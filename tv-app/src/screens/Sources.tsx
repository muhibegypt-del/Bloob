import { createEffect, createSignal, For, onCleanup, onMount, Show } from 'solid-js';
import { api, type ResolveResult } from '../data/api';
import { art } from '../data/cinemeta';
import type { MetaPreview, Video } from '../data/types';
import { useKeys } from '../keys';

const DEMO_ID = 'tt1254207';
const DEMO_VIDEO = '/media/big-buck-bunny-480p-30sec.mp4';
type FileOption = { name: string; size: number; link: string };
type DiscoveredSource = {
  kind: 'magnet' | 'link'; value: string; release: string;
  quality: string; size: string; peers: number | null; codec: string; provider: string;
};
type View = 'sources' | 'files' | 'qualities';

export function Sources(props: { item: MetaPreview; video?: Video; onBack: () => void; onPlay: (url: string, title: string) => void }) {
  const [paired, setPaired] = createSignal(false);
  const [persistent, setPersistent] = createSignal(false);
  const [pin, setPin] = createSignal<{ pin: string; url: string } | null>(null);
  const [value, setValue] = createSignal('');
  const [busy, setBusy] = createSignal(false);
  const [message, setMessage] = createSignal('');
  const [qualities, setQualities] = createSignal<{ id: string; quality?: string | number; ext?: string }[]>([]);
  const [qualityId, setQualityId] = createSignal('');
  const [files, setFiles] = createSignal<FileOption[]>([]);
  const [sources, setSources] = createSignal<DiscoveredSource[]>([]);
  const [searching, setSearching] = createSignal(true);
  const [view, setView] = createSignal<View>('sources');
  const [focus, setFocus] = createSignal(0);
  let pairingTimer: number | undefined;
  let active = true;

  const demoOffset = () => props.item.id === DEMO_ID ? 1 : 0;
  const pairIndex = () => demoOffset();
  const sourceIndex = () => pairIndex() + 1;
  const inputIndex = () => sourceIndex() + sources().length;
  const submitIndex = () => inputIndex() + 1;
  createEffect(() => {
    focus();
    window.requestAnimationFrame(() => document.querySelector('.sources-screen .focused')?.scrollIntoView({ block: 'nearest' }));
  });

  onMount(() => {
    api<{ paired: boolean; persistent: boolean }>('account').then(x => { setPaired(x.paired); setPersistent(x.persistent); }).catch(() => setMessage('Local service is unavailable'));
    api<{ sources: DiscoveredSource[] }>('sources/search', { type: props.item.type, id: props.item.id, videoId: props.video?.id })
      .then(result => { if (active) setSources(result.sources); })
      .catch(error => { if (active) setMessage(`Source search: ${(error as Error).message}`); })
      .finally(() => { if (active) setSearching(false); });
  });
  onCleanup(() => { active = false; window.clearInterval(pairingTimer); });

  async function pair() {
    try {
      setBusy(true);
      const next = await api<{ pin: string; url: string }>('pin/start', {});
      setPin(next);
      setMessage('Open the address on your phone and enter this code.');
      pairingTimer = window.setInterval(async () => {
        try {
          const status = await api<{ paired: boolean; persistent: boolean }>('pin/check', {});
          if (status.paired && active) {
            setPaired(true); setPersistent(status.persistent); setPin(null);
            setMessage(status.persistent ? 'AllDebrid connected and saved on this TV' : 'Connected for this session; TV storage was unavailable');
            window.clearInterval(pairingTimer);
          }
        } catch (error) { if (active) setMessage((error as Error).message); window.clearInterval(pairingTimer); }
      }, 5000);
    } catch (error) { setMessage((error as Error).message); }
    finally { setBusy(false); }
  }

  async function finish(result: ResolveResult) {
    if (result.state === 'ready' && result.url) return props.onPlay(result.url, props.video?.title || props.item.name);
    if (result.state === 'qualities' && result.streams?.length) {
      setQualityId(String(result.id)); setQualities(result.streams);
      setView('qualities'); setFocus(0);
      setMessage('Choose a playback quality'); return;
    }
    if (result.state === 'waiting' && result.delayed) {
      setMessage('Preparing the link…');
      for (let attempt = 0; active && attempt < 60; attempt++) {
        await new Promise(resolve => window.setTimeout(resolve, 5000));
        if (!active) return;
        const next = await api<ResolveResult>('link/delayed', { id: result.delayed });
        if (next.state === 'ready') return finish(next);
        if (next.state === 'error') throw new Error('AllDebrid could not prepare this link');
      }
      throw new Error('Link preparation timed out');
    }
    throw new Error('No playable link was returned');
  }

  async function unlock(link: string) {
    setBusy(true); setMessage('Resolving source…');
    try { await finish(await api<ResolveResult>('link/unlock', { link })); }
    catch (error) { if (active) setMessage((error as Error).message); }
    finally { if (active) setBusy(false); }
  }

  function likelyFile(all: FileOption[]): FileOption | null {
    const clean = all.filter(file => !/(^|[.\s_-])(sample|trailer|extras?)([.\s_-]|$)/i.test(file.name));
    const candidates = clean.length ? clean : all;
    if (candidates.length === 1) return candidates[0];
    const season = props.video?.season;
    const episode = props.video?.episode;
    if (season !== undefined && episode !== undefined) {
      const matches = candidates.filter(file =>
        new RegExp(`s0?${season}e0?${episode}(?!\\d)|(?:^|\\D)${season}x0?${episode}(?!\\d)`, 'i').test(file.name));
      return matches.length === 1 ? matches[0] : null;
    }
    const ranked = [...candidates].sort((a, b) => b.size - a.size);
    return ranked[0].size > 0 && ranked[0].size >= ranked[1].size * 1.5 ? ranked[0] : null;
  }

  async function resolveSource(source: string) {
    source = source.trim();
    if (!source) return setMessage('Enter a link or magnet');
    if (!paired()) return setMessage('Connect AllDebrid first');
    if (!source.startsWith('magnet:') && !/^[a-f0-9]{40}$/i.test(source)) return unlock(source);
    setBusy(true); setMessage('Checking torrent availability…');
    try {
      const magnet = await api<{ id: number; ready: boolean }>('magnet/add', { magnet: source });
      if (!magnet.ready) {
        for (let attempt = 0; active && attempt < 120; attempt++) {
          await new Promise(resolve => window.setTimeout(resolve, 5000));
          if (!active) return;
          const status = await api<{ ready: boolean; status: string }>('magnet/status', { id: magnet.id });
          setMessage(status.status || 'Preparing torrent…');
          if (status.ready) break;
          if (attempt === 119) throw new Error('Torrent is not ready yet. Try again later.');
        }
      }
      const result = await api<{ files: FileOption[] }>('magnet/files', { id: magnet.id });
      if (!result.files.length) throw new Error('No video file found in this torrent');
      const likely = likelyFile(result.files);
      if (likely) { await unlock(likely.link); return; }
      setFiles(result.files);
      setView('files'); setFocus(0);
      setMessage('Choose a video file');
    } catch (error) { if (active) setMessage((error as Error).message); }
    finally { if (active) setBusy(false); }
  }

  function submit() { return resolveSource(value()); }
  function chooseSource(source: DiscoveredSource) { return resolveSource(source.value); }

  async function chooseQuality(stream: { id: string }) {
    try {
      setBusy(true);
      await finish(await api<ResolveResult>('link/quality', { id: qualityId(), stream: stream.id }));
    } catch (error) { setMessage((error as Error).message); }
    finally { setBusy(false); }
  }

  useKeys(key => {
    if (key === 'back') {
      if (view() !== 'sources') { setView('sources'); setFocus(0); return; }
      return props.onBack();
    }
    if (view() === 'files' || view() === 'qualities') {
      const count = view() === 'files' ? files().length : qualities().length;
      if (key === 'up') setFocus(Math.max(0, focus() - 1));
      if (key === 'down') setFocus(Math.min(count - 1, focus() + 1));
      if (key === 'enter') {
        if (view() === 'files' && files()[focus()]) unlock(files()[focus()].link);
        if (view() === 'qualities' && qualities()[focus()]) chooseQuality(qualities()[focus()]);
      }
      return;
    }
    if (key === 'up') setFocus(Math.max(0, focus() - 1));
    if (key === 'down') setFocus(Math.min(submitIndex(), focus() + 1));
    if (key === 'enter') {
      if (focus() === 0 && props.item.id === DEMO_ID) props.onPlay(DEMO_VIDEO, props.item.name);
      if (focus() === pairIndex() && !paired()) pair();
      if (focus() >= sourceIndex() && focus() < inputIndex()) chooseSource(sources()[focus() - sourceIndex()]);
      if (focus() === inputIndex()) document.getElementById('source-input')?.focus();
      if (focus() === submitIndex()) submit();
    }
  });

  return <div class="sources-screen">
    <div class="sources-backdrop" style={{ 'background-image': `url("${art.backdrop(props.item.id)}")` }} />
    <div class="sources-content">
      <div class="sources-kicker">{view() === 'sources' ? 'PLAYBACK SOURCES' : view() === 'files' ? 'CHOOSE A FILE' : 'CHOOSE QUALITY'}</div>
      <h1>{props.video?.title || props.item.name}</h1>
      <p class="sources-subtitle">{view() === 'sources' ? 'Choose a release. The TV will play one resolved file.' : 'Back returns to the source list.'}</p>
      <Show when={message()}><p class="sources-alert" role="status">{message()}</p></Show>

      <Show when={view() === 'sources'} fallback={<div class="source-choice-panel">
        <h2>{view() === 'files' ? 'Select the video file' : 'Select playback quality'}</h2>
        <p>{view() === 'files' ? 'The release contains several videos, so choose the one you want.' : 'This link offers several versions.'}</p>
        <Show when={view() === 'files'} fallback={<For each={qualities()}>{(quality, index) =>
          <button class="source-card" classList={{ focused: focus() === index() }} onClick={() => chooseQuality(quality)}>
            <strong class="source-badge">{quality.quality || quality.id}</strong><span>{quality.ext || 'Video stream'}</span><b>›</b>
          </button>
        }</For>}>
          <For each={files()}>{(file, index) =>
            <button class="source-card" classList={{ focused: focus() === index() }} onClick={() => unlock(file.link)}>
              <strong class="source-badge">FILE</strong>
              <span class="source-copy"><b>{file.name}</b><small>{(file.size / 1e9).toFixed(2)} GB</small></span><b>›</b>
            </button>
          }</For>
        </Show>
      </div>}>
        <Show when={props.item.id === DEMO_ID}>
          <button class="source-card" classList={{ focused: focus() === 0 }} onClick={() => props.onPlay(DEMO_VIDEO, props.item.name)}>
            <strong class="source-badge">TEST</strong><span class="source-copy"><b>Big Buck Bunny</b><small>Bundled 480p · 30 seconds</small></span><b>›</b>
          </button>
        </Show>
        <button class="source-account" classList={{ focused: focus() === pairIndex() }} onClick={pair} disabled={paired() || busy()}>
          <span>{paired() ? 'AllDebrid connected' : 'Connect AllDebrid with PIN'}</span>
          <small>{paired() ? (persistent() ? 'Saved on this TV' : 'This session only') : 'Needed to resolve indexed releases'}</small>
        </button>
        <Show when={pin()}>{p => <div class="pair-code"><b>{p().pin}</b><span>Open {p().url} on another device</span></div>}</Show>

        <div class="source-section-heading"><h2>Releases</h2><span>{searching() ? 'Searching…' : `${sources().length} shown · external indexer`}</span></div>
        <Show when={!searching() && !sources().length}><p class="source-message">No indexed release was found. You can still enter a link below.</p></Show>
        <div class="discovered-list"><For each={sources()}>{(source, index) =>
          <button class="source-card" classList={{ focused: focus() === sourceIndex() + index() }} disabled={busy()} onClick={() => chooseSource(source)}>
            <strong class="source-badge">{source.quality || 'VIDEO'}</strong>
            <span class="source-copy"><b>{source.release}</b><small>{[source.size, source.codec, source.peers !== null ? `${source.peers} peers` : '', source.provider].filter(Boolean).join(' · ')}</small></span>
            <b class="source-chevron">›</b>
          </button>
        }</For></div>
        <div class="source-manual-label">Have a direct link or magnet?</div>
        <div class="source-entry">
          <input id="source-input" classList={{ focused: focus() === inputIndex() }} value={value()} onInput={e => setValue(e.currentTarget.value)} placeholder="Paste a link or magnet" />
          <button class="source-submit" classList={{ focused: focus() === submitIndex() }} disabled={busy()} onClick={submit}>Resolve</button>
        </div>
      </Show>
      <Show when={busy()}><div class="sources-busy"><div class="loading-mark" /><span>{message() || 'Preparing source…'}</span></div></Show>
    </div>
  </div>;
}
