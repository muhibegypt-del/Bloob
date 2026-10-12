// Buna: Bloob's app-owned playback runtime.
//
// The UI never touches a media element. It sends commands to a BunaSession and
// renders the PlaybackState the session publishes. A session drives exactly one
// BunaBackend; backends differ in what they can do, and say so through
// BackendCapabilities instead of pretending.

import type { ContentType } from '../data/types';

// ---------------------------------------------------------------------------
// Intent and sources

/** What the user asked to watch. Created when they press Play. */
export interface PlayIntent {
  contentId: string;
  contentType: ContentType;
  /** Episode ID (`tt…:season:episode`) for series. */
  videoId?: string;
  title: string;
  /** Secondary line, e.g. `S1 · E3 · Pilot`. */
  subtitle?: string;
  /** Explicit start position in seconds; otherwise Buna may resume. */
  startPosition?: number;
  origin: 'details' | 'sources' | 'test';
  /** `performance.now()` when the user pressed Play. */
  requestedAt: number;
}

/** Media properties a source *claims*. Every field is optional because providers omit or misreport them. */
export interface MediaClaims {
  resolution?: '4K' | '1080p' | '720p' | '480p' | 'SD';
  hdr?: ('DV' | 'HDR10+' | 'HDR10' | 'HDR' | 'HLG')[];
  videoCodec?: 'HEVC' | 'H.264' | 'AV1' | 'VP9' | 'MPEG-2';
  audio?: string[];
  channels?: string;
  container?: string;
}

/** One discovered, not yet resolved, option. */
export interface SourceCandidate {
  id: string;
  provider: string;
  kind: 'magnet' | 'link';
  /** Info hash for magnets, URL for links. Never displayed. */
  value: string;
  release: string;
  fileName?: string;
  claims: MediaClaims;
  sourceType?: 'REMUX' | 'BluRay' | 'WEB-DL' | 'WEBRip' | 'HDTV' | 'DVD' | 'CAM' | 'TS' | 'SCR';
  sizeBytes?: number;
  seeders?: number;
  languages: string[];
  multiAudio?: boolean;
  /** The release bundles several episodes or a full season. */
  pack?: boolean;
  /** Provenance of these claims: they come from release names, not from probing the file. */
  claimsFrom: 'release-name';
}

/** A playable media location. Held in memory only; never logged or persisted. */
export interface ResolvedSource {
  url: string;
  transport: 'progressive' | 'hls' | 'dash';
  /** Absolute epoch ms after which the URL is likely invalid, when known. */
  expiresAt?: number;
  fileName?: string;
  sizeBytes?: number;
  claims: MediaClaims;
  candidate?: SourceCandidate;
  resolvedAt: number;
}

// ---------------------------------------------------------------------------
// Capabilities

/** How we know a capability works. `measured` requires an observed TV test. */
export type Evidence = 'unverified' | 'user-report' | 'measured';

export interface Capability {
  /** The backend can attempt this with the current media. */
  supported: boolean;
  /** Why it is or isn't supported, in plain language. */
  reason?: string;
  /** Whether this was confirmed on the target TV. */
  tv: Evidence;
}

export interface BackendCapabilities {
  seek: Capability;
  audioTrackSelection: Capability;
  textTrackSelection: Capability;
  externalSubtitles: Capability;
  thumbnails: Capability;
  frameRateMatching: Capability;
  outputInfo: Capability;
}

// ---------------------------------------------------------------------------
// Tracks, state, errors

export interface Track {
  id: string;
  label: string;
  language: string;
  selected: boolean;
}

export type PlaybackPhase =
  | 'idle'
  | 'loading'
  | 'buffering'
  | 'playing'
  | 'paused'
  | 'seeking'
  | 'ended'
  | 'error';

export type BunaErrorCode =
  | 'network'
  | 'decode'
  | 'unsupported'
  | 'aborted'
  | 'startup-timeout'
  | 'expired'
  | 'play-rejected'
  | 'unknown';

export interface BunaError {
  code: BunaErrorCode;
  message: string;
  /** Retrying the same source might help (vs. picking another source). */
  retryable: boolean;
}

export interface TimeRange { start: number; end: number }

export interface PlaybackState {
  phase: PlaybackPhase;
  /** What the user last asked for. Distinct from phase: a "play" intent can be buffering. */
  intent: 'play' | 'pause';
  /** Last observed media clock, seconds. */
  position: number;
  /** Where `position` came from. `media` = sampled from the backend clock. */
  positionSource: 'initial' | 'media' | 'seek-ack';
  duration: number | null;
  durationSource: 'media' | 'seekable' | 'none';
  seekable: TimeRange | null;
  bufferedEnd: number;
  /** Seek target the user is choosing (not yet sent to the backend). */
  previewTarget: number | null;
  /** Seek target sent to the backend and awaiting acknowledgement. */
  pendingTarget: number | null;
  audioTracks: Track[];
  textTracks: Track[];
  capabilities: BackendCapabilities;
  error: BunaError | null;
  /** ms from PlayIntent.requestedAt to the first rendered frame. */
  firstFrameMs: number | null;
  /** Position Buna resumed from, if it resumed. */
  resumedFrom: number | null;
  /** Position stopped advancing while playing (detected even without `waiting` events). */
  stalled: boolean;
}

// ---------------------------------------------------------------------------
// Commands

export type CommandResult =
  | { ok: true }
  | { ok: false; reason: 'unsupported' | 'disposed' | 'not-ready' | 'failed'; message: string };

export const ok: CommandResult = { ok: true };
export const fail = (reason: Exclude<CommandResult, { ok: true }>['reason'], message: string): CommandResult =>
  ({ ok: false, reason, message });

// ---------------------------------------------------------------------------
// Backend contract

export interface BackendSnapshot {
  currentTime: number;
  duration: number;
  seekable: TimeRange | null;
  bufferedEnd: number;
  paused: boolean;
  ended: boolean;
  /** A seek is still being processed; `currentTime` may not be a settled clock reading. */
  seeking: boolean;
  /** HTMLMediaElement.readyState semantics (0–4). */
  readyState: number;
}

export type BackendEvent =
  | { type: 'metadata' }
  | { type: 'canplay' }
  | { type: 'playing' }
  | { type: 'waiting' }
  | { type: 'pause' }
  | { type: 'seeking' }
  | { type: 'seeked' }
  | { type: 'ended' }
  | { type: 'time' }
  | { type: 'progress' }
  | { type: 'tracks' }
  | { type: 'first-frame' }
  | { type: 'error'; error: BunaError };

export interface BunaBackend {
  readonly kind: string;
  /** Start loading. Resolves when the source is attached (not when it can play). */
  prepare(source: ResolvedSource, startPosition?: number): Promise<CommandResult>;
  play(): Promise<CommandResult>;
  pause(): CommandResult;
  /** Request a position. The backend later emits `seeked`; the session waits for it. */
  seek(seconds: number): CommandResult;
  selectAudio(id: string): CommandResult;
  /** `null` turns subtitles off. */
  selectSubtitle(id: string | null): CommandResult;
  snapshot(): BackendSnapshot;
  audioTracks(): Track[];
  textTracks(): Track[];
  capabilities(): BackendCapabilities;
  subscribe(listener: (event: BackendEvent) => void): () => void;
  /** Stop playback but keep the backend reusable. */
  stop(): void;
  /** Release the decoder and every listener. Idempotent. */
  dispose(): void;
}

// ---------------------------------------------------------------------------
// Metrics (never contain URLs or account data)

export type BunaMetric =
  | { name: 'resolve'; at: number; ms: number; kind: 'magnet' | 'link' | 'test'; outcome: 'ready' | 'choice' | 'failed' | 'cancelled' }
  | { name: 'session-start'; at: number; backend: string; transport: string }
  | { name: 'metadata'; at: number; ms: number }
  | { name: 'first-frame'; at: number; ms: number }
  | { name: 'seek'; at: number; from: number; to: number; settleMs: number; landed: number }
  | { name: 'seek-timeout'; at: number; to: number }
  | { name: 'stall'; at: number; position: number }
  | { name: 'stall-end'; at: number; ms: number }
  | { name: 'error'; at: number; code: BunaErrorCode }
  | { name: 'dispose'; at: number; position: number; watchedMs: number };
