import { WebVideoBackend } from './backends/web-video';
import { metrics } from './metrics';
import { localPositionStore, type PositionStore } from './positions';
import { BunaSession, type SessionConfig } from './session';
import type { BunaBackend, PlayIntent, ResolvedSource } from './types';

export interface OpenOptions {
  intent: PlayIntent;
  source: ResolvedSource;
  /** The element the view renders video into. */
  video: HTMLVideoElement;
  config?: Partial<SessionConfig>;
}

/**
 * Process-wide coordinator. Guarantees at most one live playback session, picks
 * the backend for a source, and reacts to app lifecycle (webOS suspends apps by
 * hiding the document).
 */
export class BunaRuntime {
  private session: BunaSession | null = null;
  private lifecycleInstalled = false;

  constructor(
    private readonly positions: PositionStore = localPositionStore(),
    private readonly createBackend: (source: ResolvedSource, video: HTMLVideoElement) => BunaBackend = defaultBackend,
  ) {}

  get current(): BunaSession | null { return this.session; }

  /** Create and start a session. Any existing session is disposed first. */
  open(options: OpenOptions): BunaSession {
    this.installLifecycle();
    this.session?.dispose();
    const session = new BunaSession({
      backend: this.createBackend(options.source, options.video),
      source: options.source,
      intent: options.intent,
      positions: this.positions,
      config: options.config,
    });
    this.session = session;
    session.onDispose(() => { if (this.session === session) this.session = null; });
    void session.start();
    return session;
  }

  /** Dispose the active session, if any. */
  close(): void {
    this.session?.dispose();
  }

  private installLifecycle() {
    if (this.lifecycleInstalled || typeof document === 'undefined') return;
    this.lifecycleInstalled = true;
    document.addEventListener('visibilitychange', () => {
      // The app went to the background (Home button, input switch): pause and save.
      if (document.hidden && this.session?.state.intent === 'play') this.session.pause();
    });
    window.addEventListener('pagehide', () => this.session?.dispose());
  }
}

/**
 * Backend selection. The web video element is the only backend today; an
 * HLS/MSE or native backend plugs in here when a measured gap justifies it.
 */
function defaultBackend(_source: ResolvedSource, video: HTMLVideoElement): BunaBackend {
  return new WebVideoBackend(video);
}

export const buna = new BunaRuntime();

// Read-only hooks for measuring on the TV through the web inspector.
// They expose timings and playback state only, never source URLs.
if (typeof window !== 'undefined') {
  (window as unknown as { __buna: object }).__buna = {
    metrics: () => metrics.list(),
    state: () => buna.current?.state ?? null,
  };
}
