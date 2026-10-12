import { createEffect, createMemo, createSignal, For, on, onCleanup, onMount, Show, type JSX } from 'solid-js';
import { buna, useBunaState, type BunaSession, type PlayIntent, type ResolvedSource, type Track } from '../buna';
import { clock, delta, timeOfDay } from '../format';
import { useKeys, type Key } from '../keys';

export type PlayerExit = 'back' | 'choose-source';

/**
 * Player view. Owns presentation only: HUD visibility, focus, and menus.
 * Everything about media comes from the Buna session's state, and every
 * media action is a session command.
 */
export function Player(props: { intent: PlayIntent; source: ResolvedSource; onExit: (reason: PlayerExit) => void }) {
  let video!: HTMLVideoElement;
  const [session, setSession] = createSignal<BunaSession | null>(null);
  onMount(() => setSession(buna.open({ intent: props.intent, source: props.source, video })));
  onCleanup(() => session()?.dispose());

  return <div class="player-screen">
    <video ref={video} playsinline />
    <Show when={session()} keyed>{active => <PlayerChrome session={active} intent={props.intent} onExit={props.onExit} />}</Show>
  </div>;
}

type Zone = 'timeline' | 'controls';
type Menu = 'audio' | 'subtitles' | null;
type Control = { id: string; label: string; icon: () => JSX.Element; run: () => void; trailing?: boolean };

const HUD_TIMEOUT_MS = 6000;
const BUFFERING_DELAY_MS = 500;

function PlayerChrome(props: { session: BunaSession; intent: PlayIntent; onExit: (reason: PlayerExit) => void }) {
  const session = props.session;
  const state = useBunaState(session);
  const [hud, setHud] = createSignal<'hidden' | 'scrub' | 'full'>('full');
  const [zone, setZone] = createSignal<Zone>('timeline');
  const [controlIndex, setControlIndex] = createSignal(0);
  const [menu, setMenu] = createSignal<Menu>(null);
  const [menuIndex, setMenuIndex] = createSignal(0);
  const [errorIndex, setErrorIndex] = createSignal(0);
  const [showSpinner, setShowSpinner] = createSignal(false);
  const [showResume, setShowResume] = createSignal(false);
  const [now, setNow] = createSignal(Date.now());
  let hideTimer: number | undefined;
  let spinnerTimer: number | undefined;
  let resumeTimer: number | undefined;

  const display = () => state.previewTarget ?? state.pendingTarget ?? state.position;
  const isPlaying = () => state.intent === 'play' && state.phase !== 'ended';
  const remaining = () => (state.duration ? Math.max(0, state.duration - display()) : null);
  const fraction = (seconds: number) => (state.duration ? Math.min(1, Math.max(0, seconds / state.duration)) : 0);

  // ---------------------------------------------------------------- HUD visibility

  /** Paused, ended, loading, menus, and errors keep the full HUD; uninterrupted playback hides it. */
  const pinned = () => !!menu() || ['paused', 'ended', 'error', 'loading'].includes(state.phase);
  const seekActive = () => state.previewTarget !== null || state.pendingTarget !== null;

  /** Show the HUD. `scrub` is the reduced timeline-only form used for seeking from hidden. */
  function reveal(mode: 'scrub' | 'full' = 'full') {
    setHud(current => (current === 'full' || mode === 'full' ? 'full' : 'scrub'));
    armHide();
  }
  function armHide(ms = HUD_TIMEOUT_MS) {
    window.clearTimeout(hideTimer);
    hideTimer = window.setTimeout(() => {
      if (pinned() || seekActive()) armHide();
      else setHud('hidden');
    }, ms);
  }
  // A pause, error, or end the user didn't trigger from the HUD still brings it up.
  createEffect(on(pinned, isPinned => {
    if (isPinned && hud() === 'hidden') setHud('full');
    armHide();
  }, { defer: true }));
  createEffect(on(() => state.phase, phase => {
    // The reduced scrub HUD leaves quickly once the seek lands.
    if (phase === 'playing' && hud() === 'scrub') armHide(2500);
    if (phase === 'error') setErrorIndex(0);
  }));

  // Delay the spinner so short buffering blips don't flash.
  createEffect(on(() => state.phase === 'buffering' || state.phase === 'loading', waiting => {
    window.clearTimeout(spinnerTimer);
    if (!waiting) return setShowSpinner(false);
    spinnerTimer = window.setTimeout(() => setShowSpinner(true), BUFFERING_DELAY_MS);
  }));

  createEffect(on(() => state.resumedFrom, from => {
    window.clearTimeout(resumeTimer);
    setShowResume(!!from);
    if (from) resumeTimer = window.setTimeout(() => setShowResume(false), 8000);
  }));

  const clockTimer = window.setInterval(() => setNow(Date.now()), 15000);
  onMount(() => reveal());
  onCleanup(() => {
    window.clearTimeout(hideTimer); window.clearTimeout(spinnerTimer);
    window.clearTimeout(resumeTimer); window.clearInterval(clockTimer);
  });

  // ---------------------------------------------------------------- controls

  const controls = createMemo<Control[]>(() => [
    ...(state.resumedFrom ? [{ id: 'restart', label: 'Start over', icon: RestartIcon, run: () => { session.restart(); reveal(); } }] : []),
    { id: 'back10', label: 'Back 10 seconds', icon: Back10Icon, run: () => { session.nudge(-1); reveal(); } },
    { id: 'toggle', label: isPlaying() ? 'Pause' : 'Play', icon: () => (isPlaying() ? <PauseIcon /> : <PlayGlyph />), run: () => { void session.toggle(); reveal(); } },
    { id: 'fwd10', label: 'Forward 10 seconds', icon: Forward10Icon, run: () => { session.nudge(1); reveal(); } },
    { id: 'audio', label: 'Audio', icon: AudioIcon, run: () => openMenu('audio'), trailing: true },
    { id: 'subtitles', label: 'Subtitles', icon: SubtitleIcon, run: () => openMenu('subtitles'), trailing: true },
  ]);
  // Keep focus on the same control when the list changes (e.g. Start over disappears).
  createEffect(on(() => controls().length, (length, previous) => {
    if (previous !== undefined && length < previous) setControlIndex(i => Math.max(0, i - (previous - length)));
  }, { defer: true }));
  const focusToggle = () => setControlIndex(controls().findIndex(c => c.id === 'toggle'));

  const menuTracks = (): Track[] => (menu() === 'audio' ? state.audioTracks : state.textTracks);
  const menuCapability = () => (menu() === 'audio' ? state.capabilities.audioTrackSelection : state.capabilities.textTrackSelection);
  /** Rows: audio tracks, or "Off" + subtitle tracks. */
  const menuRows = () => (menu() === 'subtitles' && menuCapability().supported ? [null, ...menuTracks()] : menuCapability().supported ? menuTracks() : []);

  function openMenu(next: Exclude<Menu, null>) {
    setMenu(next);
    const rows = next === 'audio' ? state.audioTracks : [null, ...state.textTracks];
    setMenuIndex(Math.max(0, rows.findIndex(row => (row ? row.selected : !state.textTracks.some(t => t.selected)))));
  }

  function chooseMenuRow(index: number) {
    const row = menuRows()[index];
    if (menu() === 'audio' && row) session.selectAudio(row.id);
    if (menu() === 'subtitles') session.selectSubtitle(row ? row.id : null);
    setMenu(null);
    reveal();
  }

  const errorActions = () => [
    ...(state.error?.retryable ? [{ label: 'Try again', run: () => void session.retry() }] : []),
    { label: 'Choose another source', run: () => props.onExit('choose-source') },
    { label: 'Exit player', run: () => props.onExit('back') },
  ];

  // ---------------------------------------------------------------- remote input

  function onTransportKey(key: Key): boolean {
    switch (key) {
      case 'play': void session.play(); break;
      case 'pause': session.pause(); break;
      case 'playpause': void session.toggle(); break;
      case 'ff': session.nudge(1); break;
      case 'rw': session.nudge(-1); break;
      case 'stop': props.onExit('back'); return true;
      default: return false;
    }
    reveal(hud() === 'hidden' && (key === 'ff' || key === 'rw') ? 'scrub' : 'full');
    return true;
  }

  useKeys(key => {
    if (state.phase === 'error') {
      const actions = errorActions();
      if (key === 'left' || key === 'up') setErrorIndex(i => Math.max(0, i - 1));
      if (key === 'right' || key === 'down') setErrorIndex(i => Math.min(actions.length - 1, i + 1));
      if (key === 'enter') actions[Math.min(errorIndex(), actions.length - 1)].run();
      if (key === 'back' || key === 'stop') props.onExit('back');
      return;
    }
    if (menu()) {
      const count = menuRows().length;
      if (key === 'up') setMenuIndex(i => Math.max(0, i - 1));
      if (key === 'down') setMenuIndex(i => Math.min(Math.max(0, count - 1), i + 1));
      if (key === 'enter' && count) chooseMenuRow(menuIndex());
      if (key === 'back' || key === 'left') { setMenu(null); reveal(); }
      return;
    }
    if (onTransportKey(key)) return;

    if (key === 'back') {
      if (session.cancelPreview()) return reveal();
      if (hud() !== 'hidden' && isPlaying()) { window.clearTimeout(hideTimer); return setHud('hidden'); }
      return props.onExit('back');
    }

    if (hud() === 'hidden') {
      // First press wakes the HUD; left/right and OK also act immediately.
      if (key === 'left' || key === 'right') { session.nudge(key === 'right' ? 1 : -1); return reveal('scrub'); }
      if (key === 'enter') { void session.toggle(); setZone('controls'); focusToggle(); return reveal(); }
      setZone(key === 'down' ? 'controls' : 'timeline');
      if (key === 'down') focusToggle();
      return reveal();
    }

    // Repeated seeking keeps the reduced HUD; anything else brings up the full controls.
    reveal(hud() === 'scrub' && zone() === 'timeline' && (key === 'left' || key === 'right') ? 'scrub' : 'full');
    if (zone() === 'timeline') {
      if (key === 'left' || key === 'right') return void session.nudge(key === 'right' ? 1 : -1);
      if (key === 'enter') return void (state.previewTarget !== null ? session.commitPreview() : session.toggle());
      if (key === 'down') { setZone('controls'); if (hud() === 'scrub') setHud('full'); return focusToggle(); }
      return;
    }
    if (key === 'up') return setZone('timeline');
    if (key === 'left') return setControlIndex(i => Math.max(0, i - 1));
    if (key === 'right') return setControlIndex(i => Math.min(controls().length - 1, i + 1));
    if (key === 'enter') controls()[controlIndex()]?.run();
  });

  // ---------------------------------------------------------------- view

  const statusLabel = () => {
    if (state.phase === 'seeking') return 'Seeking…';
    if (state.phase === 'buffering') return 'Buffering…';
    if (state.phase === 'loading') return 'Starting…';
    if (state.phase === 'ended') return 'Finished';
    return '';
  };

  return <>
    <Show when={state.phase === 'loading' && state.firstFrameMs === null}>
      <div class="player-startup">
        <div class="player-startup-title">{props.intent.title}</div>
        <Show when={props.intent.subtitle}><div class="player-startup-subtitle">{props.intent.subtitle}</div></Show>
        <div class="player-spinner" />
        <Show when={state.resumedFrom}>{from => <div class="player-startup-note">Resuming from {clock(from())}</div>}</Show>
      </div>
    </Show>

    <Show when={showSpinner() && state.phase === 'buffering'}>
      <div class="player-buffering"><div class="player-spinner" /></div>
    </Show>

    <Show when={state.phase === 'error' && state.error}>{error => <div class="player-error" role="alert">
      <h2>Playback stopped</h2>
      <p>{error().message}</p>
      <div class="player-error-actions">
        <For each={errorActions()}>{(action, index) =>
          <button classList={{ focused: errorIndex() === index() }} onClick={action.run}>{action.label}</button>
        }</For>
      </div>
    </div>}</Show>

    <div class="player-hud" classList={{ visible: hud() !== 'hidden' && state.phase !== 'error' && !(state.phase === 'loading' && state.firstFrameMs === null), scrub: hud() === 'scrub' }}>
      <div class="player-top">
        <div>
          <div class="player-title">{props.intent.title}</div>
          <Show when={props.intent.subtitle}><div class="player-subtitle">{props.intent.subtitle}</div></Show>
        </div>
        <div class="player-clock">
          <span>{timeOfDay(now())}</span>
          <Show when={remaining() !== null && state.phase !== 'ended'}><small>Ends {timeOfDay(now() + (remaining() || 0) * 1000)}</small></Show>
        </div>
      </div>

      <div class="player-bottom">
        <div class="player-status-row">
          <Show when={statusLabel()}><span class="player-status">{statusLabel()}</span></Show>
          <Show when={showResume() && state.resumedFrom}>{from => <span class="player-status quiet">Resumed from {clock(from())} · Start over is in the controls</span>}</Show>
        </div>

        <div class="player-timeline" classList={{ focused: zone() === 'timeline' && hud() !== 'hidden', previewing: state.previewTarget !== null, seeking: state.pendingTarget !== null }}
          role="slider" aria-label="Playback position" aria-valuemin={0} aria-valuemax={Math.round(state.duration || 0)} aria-valuenow={Math.round(display())}
          onClick={event => {
            if (!state.duration) return;
            const rect = event.currentTarget.getBoundingClientRect();
            session.seekTo(((event.clientX - rect.left) / rect.width) * state.duration);
            reveal();
          }}>
          <div class="player-track">
            <div class="player-buffered" style={{ left: `${fraction(state.position) * 100}%`, width: `${Math.max(0, fraction(state.bufferedEnd) - fraction(state.position)) * 100}%` }} />
            <div class="player-played" style={{ transform: `scaleX(${fraction(display())})` }} />
            <Show when={state.previewTarget !== null || state.pendingTarget !== null}>
              <div class="player-origin" style={{ left: `${fraction(state.position) * 100}%` }} />
            </Show>
          </div>
          <div class="player-thumb" style={{ left: `${fraction(display()) * 100}%` }} />
          <Show when={state.previewTarget !== null || state.pendingTarget !== null}>
            <div class="player-bubble" style={{ left: `clamp(80px, ${fraction(display()) * 100}%, calc(100% - 80px))` }}>
              <b>{clock(display())}</b><small>{delta(display() - state.position)}</small>
            </div>
          </Show>
        </div>
        <div class="player-times">
          <span>{clock(display())}</span>
          <span>{state.duration ? `−${clock(remaining())}` : '--:--'}</span>
        </div>

        <div class="player-controls">
          <For each={controls()}>{(control, index) =>
            <button classList={{ focused: zone() === 'controls' && controlIndex() === index(), primary: control.id === 'toggle', trailing: !!control.trailing && !controls()[index() - 1]?.trailing }}
              aria-label={control.label} onClick={control.run}>
              {control.icon()}
              <span class="player-control-label">{control.label}</span>
            </button>
          }</For>
        </div>
      </div>
    </div>

    <Show when={menu()}>{current => <div class="player-menu" role="dialog" aria-label={current() === 'audio' ? 'Audio' : 'Subtitles'}>
      <h2>{current() === 'audio' ? 'Audio' : 'Subtitles'}</h2>
      <Show when={menuCapability().supported} fallback={<p class="player-menu-note">{menuCapability().reason}</p>}>
        <For each={menuRows()}>{(row, index) =>
          <button classList={{ focused: menuIndex() === index() }} onClick={() => chooseMenuRow(index())}>
            <span>{row ? trackName(row, index()) : 'Off'}</span>
            <b>{(row ? row.selected : !state.textTracks.some(t => t.selected)) ? '✓' : ''}</b>
          </button>
        }</For>
      </Show>
      <p class="player-menu-hint">Back to close</p>
    </div>}</Show>
  </>;
}

function trackName(track: Track, index: number) {
  const language = track.language ? languageName(track.language) : '';
  if (track.label && language && !track.label.toLowerCase().includes(language.toLowerCase())) return `${track.label} · ${language}`;
  return track.label || language || `Track ${index + 1}`;
}

function languageName(code: string) {
  try {
    const names = new (Intl as unknown as { DisplayNames: new (l: string[], o: object) => { of(c: string): string } }).DisplayNames(['en'], { type: 'language' });
    return names.of(code) || code;
  } catch { return code; }
}

// ---------------------------------------------------------------- icons (original artwork)

const PlayGlyph = () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M14 9.5v21a1 1 0 0 0 1.5.86l17-10.5a1 1 0 0 0 0-1.72l-17-10.5A1 1 0 0 0 14 9.5z" fill="currentColor" /></svg>;
const PauseIcon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><rect x="11" y="9" width="6" height="22" rx="1.5" fill="currentColor" /><rect x="23" y="9" width="6" height="22" rx="1.5" fill="currentColor" /></svg>;
const Back10Icon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M20 9a12 12 0 1 1-11.3 8" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" /><path d="M20 4.5 15.5 9l4.5 4.5" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" /><text x="20" y="25" text-anchor="middle" fill="currentColor" font-size="12.5" font-weight="700">10</text></svg>;
const Forward10Icon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M20 9a12 12 0 1 0 11.3 8" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" /><path d="M20 4.5 24.5 9 20 13.5" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" /><text x="20" y="25" text-anchor="middle" fill="currentColor" font-size="12.5" font-weight="700">10</text></svg>;
const RestartIcon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M10 10v20M30 11v18L15 20z" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round" /></svg>;
const AudioIcon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M8 16h6l8-6v20l-8-6H8zM27 15c2.5 2.8 2.5 7.2 0 10M31 11c5 5 5 13 0 18" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round" /></svg>;
const SubtitleIcon = () => <svg viewBox="0 0 40 40" aria-hidden="true"><rect x="6" y="9" width="28" height="22" rx="4" fill="none" stroke="currentColor" stroke-width="2.4" /><path d="M11 20h7M11 25h5M21 20h8M21 25h6" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" /></svg>;
