import { createSignal, For, onCleanup, onMount, Show } from 'solid-js';
import { useKeys } from '../keys';

type AudioTrackInfo = { index: number; label: string; language: string; enabled: boolean };
type TextTrackInfo = { index: number; label: string; language: string; mode: TextTrackMode };
type AudioTrackListLike = { length: number; [index: number]: { label?: string; language?: string; enabled: boolean } };
type TrackVideo = HTMLVideoElement & { audioTracks?: AudioTrackListLike };
type Menu = 'audio' | 'subtitles' | null;

export function Player(props: { url: string; title: string; onBack: () => void }) {
  let video!: TrackVideo;
  const [state, setState] = createSignal('Preparing video…');
  const [position, setPosition] = createSignal(0);
  const [duration, setDuration] = createSignal(0);
  const [buffered, setBuffered] = createSignal(0);
  const [visible, setVisible] = createSignal(true);
  const [area, setArea] = createSignal<'timeline' | 'actions'>('timeline');
  const [actionFocus, setActionFocus] = createSignal(1);
  const [menu, setMenu] = createSignal<Menu>(null);
  const [menuFocus, setMenuFocus] = createSignal(0);
  const [seekTarget, setSeekTarget] = createSignal<number | null>(null);
  const [audioTracks, setAudioTracks] = createSignal<AudioTrackInfo[]>([]);
  const [textTracks, setTextTracks] = createSignal<TextTrackInfo[]>([]);
  let hideTimer: number | undefined;
  let clockTimer: number | undefined;
  let seekTimer: number | undefined;
  const start = performance.now();

  const stamp = (seconds: number) => {
    if (!Number.isFinite(seconds) || seconds < 0) return '--:--';
    const s = Math.floor(seconds);
    const hours = Math.floor(s / 3600);
    return `${hours ? hours + ':' : ''}${hours ? String(Math.floor((s % 3600) / 60)).padStart(2, '0') : Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
  };

  function syncClock() {
    if (!video) return;
    if (Number.isFinite(video.currentTime) && video.currentTime >= 0) setPosition(video.currentTime);
    let length = video.duration;
    if ((!Number.isFinite(length) || length <= 0) && video.seekable.length) length = video.seekable.end(video.seekable.length - 1);
    if (Number.isFinite(length) && length > 0) setDuration(length);
    if (video.buffered.length) setBuffered(video.buffered.end(video.buffered.length - 1));
  }

  function syncTracks() {
    const audio = video.audioTracks;
    setAudioTracks(audio ? Array.from({ length: audio.length }, (_, index) => ({
      index, label: audio[index].label || `Audio ${index + 1}`,
      language: audio[index].language || '', enabled: !!audio[index].enabled,
    })) : []);
    setTextTracks(Array.from({ length: video.textTracks.length }, (_, index) => ({
      index, label: video.textTracks[index].label || `Subtitle ${index + 1}`,
      language: video.textTracks[index].language || '', mode: video.textTracks[index].mode,
    })));
  }

  function reveal() {
    setVisible(true);
    window.clearTimeout(hideTimer);
    if (!video.paused && !menu()) hideTimer = window.setTimeout(() => setVisible(false), 9000);
  }

  function seekTo(seconds: number) {
    syncClock();
    if (!duration() || !video.seekable.length) return setState('Seeking is unavailable for this file');
    const lower = video.seekable.start(0);
    const upper = video.seekable.end(video.seekable.length - 1);
    const target = Math.min(Math.max(seconds, lower), upper);
    video.currentTime = target;
    setPosition(target);
    setState('Seeking…');
    reveal();
  }

  function previewSeek(delta: number) {
    syncClock();
    if (!video.seekable.length) return setState('Seeking is unavailable for this file');
    const lower = video.seekable.start(0);
    const upper = video.seekable.end(video.seekable.length - 1);
    const target = Math.min(Math.max((seekTarget() ?? video.currentTime) + delta, lower), upper);
    setSeekTarget(target);
    setVisible(true);
    window.clearTimeout(hideTimer);
    window.clearTimeout(seekTimer);
    seekTimer = window.setTimeout(() => {
      video.currentTime = target;
      setPosition(target);
      setSeekTarget(null);
      reveal();
    }, 550);
  }

  function toggle() {
    if (video.paused) video.play().catch(error => setState(error.message || 'Unable to resume'));
    else video.pause();
    reveal();
  }

  function openMenu(next: Exclude<Menu, null>) {
    syncTracks(); setMenu(next); setMenuFocus(0); setVisible(true);
    window.clearTimeout(hideTimer);
  }

  function selectMenu(index: number) {
    if (menu() === 'audio') {
      const tracks = video.audioTracks;
      if (!tracks || index >= tracks.length) return;
      for (let i = 0; i < tracks.length; i++) tracks[i].enabled = i === index;
    } else if (menu() === 'subtitles') {
      for (let i = 0; i < video.textTracks.length; i++) video.textTracks[i].mode = index === i + 1 ? 'showing' : 'disabled';
    }
    syncTracks(); setMenu(null); reveal();
  }

  const actions = [
    { label: 'Back 10 seconds', glyph: () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M10 10v9h9M11 19a13 13 0 1 1 .8 10" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/><text x="20" y="26" text-anchor="middle" fill="currentColor" font-size="11" font-weight="700">10</text></svg>, run: () => previewSeek(-10) },
    { label: 'Play or pause', glyph: () => video?.paused
      ? <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M15 10l17 10-17 10z" fill="currentColor"/></svg>
      : <svg viewBox="0 0 40 40" aria-hidden="true"><rect x="12" y="10" width="5" height="20" rx="1" fill="currentColor"/><rect x="23" y="10" width="5" height="20" rx="1" fill="currentColor"/></svg>, run: toggle },
    { label: 'Forward 10 seconds', glyph: () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M30 10v9h-9M29 19a13 13 0 1 0-.8 10" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/><text x="20" y="26" text-anchor="middle" fill="currentColor" font-size="11" font-weight="700">10</text></svg>, run: () => previewSeek(10) },
    { label: 'Audio', glyph: () => <svg viewBox="0 0 40 40" aria-hidden="true"><path d="M8 16h7l9-7v22l-9-7H8zM29 15c3 3 3 7 0 10M33 11c5 5 5 13 0 18" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round"/></svg>, run: () => openMenu('audio') },
    { label: 'Subtitles', glyph: () => <svg viewBox="0 0 40 40" aria-hidden="true"><rect x="5" y="8" width="30" height="24" rx="4" fill="none" stroke="currentColor" stroke-width="2.4"/><path d="M11 21h7M11 26h5M22 21h7M22 26h6" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/></svg>, run: () => openMenu('subtitles') },
  ];

  function release() {
    window.clearTimeout(hideTimer); window.clearTimeout(seekTimer); window.clearInterval(clockTimer);
    video.pause(); video.removeAttribute('src'); video.load();
  }
  onMount(() => {
    video.src = props.url; video.load();
    clockTimer = window.setInterval(syncClock, 500);
    video.play().catch(error => setState(error.message || 'Unable to start video'));
  });
  onCleanup(release);

  useKeys(key => {
    if (menu()) {
      if (key === 'back') { setMenu(null); return reveal(); }
      const count = menu() === 'audio' ? audioTracks().length : textTracks().length + 1;
      if (key === 'up') setMenuFocus(Math.max(0, menuFocus() - 1));
      if (key === 'down' && count) setMenuFocus(Math.min(count - 1, menuFocus() + 1));
      if (key === 'enter') selectMenu(menuFocus());
      return;
    }
    if (key === 'back') {
      if (seekTarget() !== null) { window.clearTimeout(seekTimer); setSeekTarget(null); return reveal(); }
      if (visible() && state() === 'Playing') { setVisible(false); return; }
      return props.onBack();
    }
    reveal();
    if (key === 'down') return setArea('actions');
    if (key === 'up') return setArea('timeline');
    if (key === 'left' || key === 'right') {
      if (area() === 'actions') setActionFocus(Math.min(actions.length - 1, Math.max(0, actionFocus() + (key === 'right' ? 1 : -1))));
      else previewSeek(key === 'right' ? 10 : -10);
    }
    if (key === 'enter') area() === 'actions' ? actions[actionFocus()].run() : toggle();
  });

  const displayPosition = () => seekTarget() ?? position();
  const percent = () => duration() > 0 ? Math.max(0, Math.min(100, displayPosition() / duration() * 100)) : 0;
  const bufferedPercent = () => duration() > 0 ? Math.max(0, Math.min(100, buffered() / duration() * 100)) : 0;

  return <div class="player-screen">
    <video ref={video} playsinline
      onLoadedMetadata={() => { syncClock(); syncTracks(); }}
      onDurationChange={syncClock} onTimeUpdate={syncClock} onProgress={syncClock}
      onSeeking={() => { syncClock(); setState('Seeking…'); }}
      onSeeked={() => { syncClock(); setState(video.paused ? 'Paused' : 'Playing'); }}
      onPlaying={() => { syncClock(); setState('Playing'); reveal(); console.info('[playback] first frame ms', Math.round(performance.now() - start)); }}
      onWaiting={() => setState('Buffering…')}
      onPause={() => { setState('Paused'); setVisible(true); window.clearTimeout(hideTimer); }}
      onEnded={() => { setState('Ended'); setVisible(true); window.clearTimeout(hideTimer); }}
      onError={() => setState(`Playback error ${video.error?.code || ''}: ${video.error?.message || 'Unsupported or unavailable media'}`)} />
    <Show when={state() === 'Buffering…' || state() === 'Preparing video…'}>
      <div class="player-center-status"><div class="loading-mark" /><span>{state()}</span></div>
    </Show>
    <Show when={visible() || state() !== 'Playing' || menu()}>
      <div class="player-hud">
        <div class="player-heading"><div><span class="player-eyebrow">NOW PLAYING</span><div class="player-title">{props.title}</div></div>
          <Show when={state() !== 'Playing' && state() !== 'Paused'}><div class="player-state">{state()}</div></Show>
        </div>
        <Show when={seekTarget() !== null}><div class="player-seek-label">Seek to {stamp(displayPosition())}</div></Show>
        <div class="player-timeline" classList={{ focused: area() === 'timeline' }} role="slider"
          aria-label="Playback position" aria-valuemin={0} aria-valuemax={Math.round(duration())} aria-valuenow={Math.round(displayPosition())}
          onPointerDown={event => {
            const rect = event.currentTarget.getBoundingClientRect();
            seekTo(((event.clientX - rect.left) / rect.width) * duration());
          }}>
          <div class="player-timeline-buffered" style={{ width: `${bufferedPercent()}%` }} />
          <div class="player-timeline-played" style={{ width: `${percent()}%` }} />
          <div class="player-timeline-thumb" style={{ left: `${percent()}%` }} />
        </div>
        <div class="player-times"><span>{stamp(displayPosition())}</span><span>{stamp(duration())}</span></div>
        <div class="player-actions">
          <For each={actions}>{(action, index) =>
            <button classList={{ focused: area() === 'actions' && actionFocus() === index(), primary: index() === 1, secondary: index() >= 3 }}
              aria-label={action.label} title={action.label} onClick={action.run}>
              {action.glyph()}
            </button>
          }</For>
        </div>
        <div class="player-hint">← → seek 10 seconds <span>·</span> ↓ options <span>·</span> Back hide or exit</div>
      </div>
    </Show>
    <Show when={menu()}>{current => <div class="player-menu">
      <h2>{current() === 'audio' ? 'Audio' : 'Subtitles'}</h2>
      <Show when={current() === 'audio'}>
        <Show when={audioTracks().length} fallback={<p>This stream exposes no selectable audio tracks. The TV is playing its default track.</p>}>
          <For each={audioTracks()}>{(track, index) =>
            <button classList={{ focused: menuFocus() === index() }} onClick={() => selectMenu(index())}>
              <span>{track.label}{track.language ? ` · ${track.language}` : ''}</span><b>{track.enabled ? '✓' : ''}</b>
            </button>
          }</For>
        </Show>
      </Show>
      <Show when={current() === 'subtitles'}>
        <button classList={{ focused: menuFocus() === 0 }} onClick={() => selectMenu(0)}><span>Off</span><b>{textTracks().every(track => track.mode !== 'showing') ? '✓' : ''}</b></button>
        <For each={textTracks()}>{(track, index) =>
          <button classList={{ focused: menuFocus() === index() + 1 }} onClick={() => selectMenu(index() + 1)}>
            <span>{track.label}{track.language ? ` · ${track.language}` : ''}</span><b>{track.mode === 'showing' ? '✓' : ''}</b>
          </button>
        }</For>
        <Show when={!textTracks().length}><p>No selectable subtitles are exposed by this file. External WebVTT support is the next step.</p></Show>
      </Show>
    </div>}</Show>
  </div>;
}
