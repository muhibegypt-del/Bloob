import type { ResolvedSource, SourceCandidate } from '../buna/types';
import { api, type ResolveResult } from '../data/api';

export interface FileOption { name: string; path?: string; size: number; link: string }
export interface QualityOption { id: string; quality?: string | number; ext?: string; filesize?: number }

export type ResolveStep = 'adding' | 'caching' | 'files' | 'unlocking' | 'preparing';
export interface ResolveProgress {
  step: ResolveStep;
  label: string;
  /** 0–1 when the debrid service reports progress. */
  fraction?: number;
}

export type ResolveOutcome =
  | { kind: 'ready'; source: ResolvedSource }
  | { kind: 'choose-file'; files: FileOption[] }
  | { kind: 'choose-quality'; linkId: string; qualities: QualityOption[] };

export interface ResolveContext {
  season?: number;
  episode?: number;
  signal: AbortSignal;
  onProgress: (progress: ResolveProgress) => void;
}

/** What the resolver needs from a candidate; manual links/magnets provide only kind and value. */
export type ResolvableSource = Pick<SourceCandidate, 'kind' | 'value'> & Partial<SourceCandidate>;

const MAGNET_POLL_MS = 4000;
const MAGNET_TIMEOUT_MS = 10 * 60 * 1000;
const DELAYED_POLL_MS = 4000;
const DELAYED_TIMEOUT_MS = 4 * 60 * 1000;

/**
 * DebridResolver: candidate → playable ResolvedSource through the local
 * service's AllDebrid routes. Cancellable at every wait through `signal`.
 */
export async function resolveSource(source: ResolvableSource, ctx: ResolveContext): Promise<ResolveOutcome> {
  if (source.kind === 'link') return unlock(source.value, ctx, source);

  ctx.onProgress({ step: 'adding', label: 'Sending to AllDebrid…' });
  const magnet = await api<{ id: number; ready: boolean }>('magnet/add', { magnet: source.value }, ctx.signal);
  if (!magnet.ready) {
    const deadline = Date.now() + MAGNET_TIMEOUT_MS;
    for (;;) {
      await sleep(MAGNET_POLL_MS, ctx.signal);
      const status = await api<{ ready: boolean; failed: boolean; status: string; downloaded: number; size: number }>('magnet/status', { id: magnet.id }, ctx.signal);
      if (status.ready) break;
      if (status.failed) throw new Error(`AllDebrid could not fetch this release (${status.status || 'failed'}). Try another source.`);
      ctx.onProgress({
        step: 'caching',
        label: status.size ? 'AllDebrid is downloading this release…' : status.status || 'Waiting for AllDebrid…',
        fraction: status.size ? Math.min(1, status.downloaded / status.size) : undefined,
      });
      if (Date.now() > deadline) throw new Error('This release is still downloading on AllDebrid. Try again later or pick another source.');
    }
  }

  ctx.onProgress({ step: 'files', label: 'Finding the video file…' });
  const { files } = await api<{ files: FileOption[] }>('magnet/files', { id: magnet.id }, ctx.signal);
  if (!files.length) throw new Error('This release contains no playable video file.');
  const file = pickFile(files, { fileName: source.fileName, season: ctx.season, episode: ctx.episode });
  if (!file) return { kind: 'choose-file', files };
  return unlock(file.link, ctx, source, file);
}

/** Continue after the viewer picked a file. */
export function resolveFile(file: FileOption, ctx: ResolveContext, source?: ResolvableSource): Promise<ResolveOutcome> {
  return unlock(file.link, ctx, source, file);
}

/** Continue after the viewer picked a quality. */
export async function resolveQuality(linkId: string, quality: QualityOption, ctx: ResolveContext, source?: ResolvableSource): Promise<ResolveOutcome> {
  ctx.onProgress({ step: 'unlocking', label: 'Getting the stream…' });
  const result = await api<ResolveResult>('link/quality', { id: linkId, stream: quality.id }, ctx.signal);
  return finish(result, ctx, source);
}

async function unlock(link: string, ctx: ResolveContext, source?: ResolvableSource, file?: FileOption): Promise<ResolveOutcome> {
  ctx.onProgress({ step: 'unlocking', label: 'Getting the stream…' });
  const result = await api<ResolveResult>('link/unlock', { link }, ctx.signal);
  return finish(result, ctx, source, file);
}

async function finish(result: ResolveResult, ctx: ResolveContext, source?: ResolvableSource, file?: FileOption): Promise<ResolveOutcome> {
  if (result.state === 'qualities' && result.streams?.length) {
    return { kind: 'choose-quality', linkId: String(result.id), qualities: result.streams };
  }
  if (result.state === 'waiting' && result.delayed) {
    const deadline = Date.now() + DELAYED_TIMEOUT_MS;
    ctx.onProgress({ step: 'preparing', label: 'AllDebrid is preparing the stream…' });
    for (;;) {
      await sleep(DELAYED_POLL_MS, ctx.signal);
      const next = await api<ResolveResult>('link/delayed', { id: result.delayed }, ctx.signal);
      if (next.state === 'ready' && next.url) return ready(next.url, result.filename || file?.name, file?.size, source);
      if (next.state === 'error') throw new Error('AllDebrid could not prepare this link.');
      if (Date.now() > deadline) throw new Error('AllDebrid took too long to prepare this link.');
    }
  }
  if (result.state === 'ready' && result.url) return ready(result.url, result.filename || file?.name, result.filesize || file?.size, source);
  throw new Error('AllDebrid returned no playable link.');
}

function ready(url: string, fileName: string | undefined, sizeBytes: number | undefined, source?: ResolvableSource): ResolveOutcome {
  return {
    kind: 'ready',
    source: {
      url,
      transport: transportFor(fileName || url),
      fileName,
      sizeBytes: sizeBytes || source?.sizeBytes,
      claims: source?.claims ?? {},
      candidate: source && 'id' in source ? (source as SourceCandidate) : undefined,
      resolvedAt: Date.now(),
    },
  };
}

export function transportFor(nameOrUrl: string): ResolvedSource['transport'] {
  const path = nameOrUrl.split('?')[0];
  if (/\.m3u8$/i.test(path)) return 'hls';
  if (/\.mpd$/i.test(path)) return 'dash';
  return 'progressive';
}

const EXTRAS = /(^|[\s._\-/[(])(sample|trailer|teaser|extras?|featurettes?|behind[\s._-]the[\s._-]scenes|deleted[\s._-]scenes)([\s._\-)\]]|$)/i;

/**
 * Pick the file to play inside a release, or null when the viewer should
 * choose. Order: the exact file the indexer named, then the only real video,
 * then the one matching the episode, then a clearly dominant largest file.
 */
export function pickFile(files: FileOption[], hint: { fileName?: string; season?: number; episode?: number }): FileOption | null {
  if (hint.fileName) {
    const wanted = hint.fileName.split('/').pop()!.toLowerCase();
    const exact = files.filter(f => f.name.toLowerCase() === wanted);
    if (exact.length === 1) return exact[0];
  }
  const clean = files.filter(f => !EXTRAS.test(f.path || f.name));
  const candidates = clean.length ? clean : files;
  if (candidates.length === 1) return candidates[0];
  if (hint.season !== undefined && hint.episode !== undefined) {
    const s = hint.season;
    const e = hint.episode;
    const pattern = new RegExp(`s0*${s}[\\s._-]*e0*${e}(?!\\d)|(?:^|\\D)${s}x0*${e}(?!\\d)`, 'i');
    const matches = candidates.filter(f => pattern.test(f.name));
    if (!matches.length) return null;
    return matches.sort((a, b) => b.size - a.size)[0];
  }
  const ranked = [...candidates].sort((a, b) => b.size - a.size);
  return ranked[0].size > 0 && ranked[0].size >= ranked[1].size * 1.5 ? ranked[0] : null;
}

/** Wait, rejecting immediately with an AbortError if the signal fires. */
export function sleep(ms: number, signal: AbortSignal): Promise<void> {
  return new Promise((resolve, reject) => {
    if (signal.aborted) return reject(abortError());
    const timer = setTimeout(() => { signal.removeEventListener('abort', onAbort); resolve(); }, ms);
    function onAbort() { clearTimeout(timer); reject(abortError()); }
    signal.addEventListener('abort', onAbort, { once: true });
  });
}

function abortError() {
  try { return new DOMException('Cancelled', 'AbortError'); }
  catch { const error = new Error('Cancelled'); error.name = 'AbortError'; return error; }
}

export const isAbort = (error: unknown) => (error as Error)?.name === 'AbortError';
