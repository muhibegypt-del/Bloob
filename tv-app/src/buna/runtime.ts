import { metrics } from './metrics';
import { localPositionStore, type PositionStore } from './positions';
import { BunaSession, type SessionConfig } from './session';
import { trace } from './trace';
import type { PlaybackHost, PlayIntent, ResolvedSource } from './types';

export interface OpenOptions {
  intent: PlayIntent;
  source: ResolvedSource;
  /** The playback owner supplies a backend; the runtime does not depend on a DOM element. */
  host: PlaybackHost;
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
  ) {}

  get current(): BunaSession | null { return this.session; }

  /** Create and start a session. Any existing session is disposed first. */
  open(options: OpenOptions): BunaSession {
    this.installLifecycle();
    this.session?.dispose();
    trace.record('app', 'open-player');
    const session = new BunaSession({
      backend: options.host.createBackend(options.source),
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
    trace.record('app', 'close-player');
    this.session?.dispose();
  }

  private installLifecycle() {
    if (this.lifecycleInstalled || typeof document === 'undefined') return;
    this.lifecycleInstalled = true;
    document.addEventListener('visibilitychange', () => {
      trace.record('app', 'visibility', { hidden: document.hidden });
      // The app went to the background (Home button, input switch): pause and save.
      if (document.hidden && this.session?.state.intent === 'play') this.session.pause();
    });
    window.addEventListener('pagehide', () => { trace.record('app', 'pagehide'); this.session?.dispose(); });
  }
}

export const buna = new BunaRuntime();

// Read-only hooks for measuring on the TV through the web inspector.
// They expose timings and playback state only, never source URLs.
if (typeof window !== 'undefined') {
  (window as unknown as { __buna: object }).__buna = {
    metrics: () => metrics.list(),
    state: () => buna.current?.state ?? null,
    trace: {
      list: () => trace.list(),
      since: (seq?: number) => trace.since(seq),
      clear: () => trace.clear(),
    },
  };
}
