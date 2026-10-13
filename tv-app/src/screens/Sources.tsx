import { createEffect, createMemo, createSignal, For, onCleanup, onMount, Show } from 'solid-js';
import { metrics, type PlayIntent, type ResolvedSource, type SourceCandidate } from '../buna';
import { trace } from '../buna/trace';
import { api, ApiError, type AccountStatus } from '../data/api';
import { art, demo } from '../data/cinemeta';
import type { MetaPreview, Video } from '../data/types';
import { bytes } from '../format';
import { useKeys } from '../keys';
import { details, headline, rankCandidates, type RankedCandidate } from '../sources/ranker';
import {
  isAbort, resolveFile, resolveQuality, resolveSource,
  type FileOption, type QualityOption, type ResolvableSource, type ResolveOutcome, type ResolveProgress,
} from '../sources/resolver';

const TEST_SOURCE: ResolvedSource = {
  url: '/media/big-buck-bunny-480p-30sec.mp4', transport: 'progressive', fileName: 'big-buck-bunny-480p-30sec.mp4',
  claims: { resolution: '480p', videoCodec: 'H.264', audio: ['AAC'], container: 'MP4' }, resolvedAt: 0,
};
const GROUP_PREVIEW = 3;
const SEARCH_TTL_MS = 10 * 60 * 1000;
const SEARCH_CACHE_LIMIT = 40;
const searchCache = new Map<string, { at: number; candidates: SourceCandidate[] }>();

type Mode = 'list' | 'pairing' | 'resolving' | 'files' | 'qualities' | 'manual';
type Item = { key: string; run: () => void };

export function Sources(props: {
  item: MetaPreview;
  video?: Video;
  active: boolean;
  onBack: () => void;
  onPlay: (intent: PlayIntent, source: ResolvedSource) => void;
}) {
  const [account, setAccount] = createSignal<AccountStatus | null>(null);
  const [searching, setSearching] = createSignal(true);
  const [candidates, setCandidates] = createSignal<SourceCandidate[]>([]);
  const [searchError, setSearchError] = createSignal('');
  const [mode, setMode] = createSignal<Mode>('list');
  const [focusKey, setFocusKey] = createSignal('');
  const [expanded, setExpanded] = createSignal<Set<string>>(new Set());
  const [banner, setBanner] = createSignal<{ tone: 'info' | 'error'; text: string } | null>(null);
  const [pin, setPin] = createSignal<{ code: string; url: string; expiresAt: number } | null>(null);
  const [pinExpired, setPinExpired] = createSignal(false);
  const [now, setNow] = createSignal(Date.now());
  const [progress, setProgress] = createSignal<ResolveProgress | null>(null);
  const [resolving, setResolving] = createSignal<ResolvableSource | null>(null);
  const [files, setFiles] = createSignal<FileOption[]>([]);
  const [qualities, setQualities] = createSignal<{ linkId: string; options: QualityOption[] }>({ linkId: '', options: [] });
  const [manualValue, setManualValue] = createSignal('');
  let operation: AbortController | null = null;
  let searchOperation: AbortController | null = null;
  let pairGeneration = 0;
  let alive = true;
  let pairTimer: number | undefined;
  let tickTimer: number | undefined;
  let afterPairing: ResolvableSource | null = null;
  let operationStarted = 0;
  let manualInput: HTMLInputElement | undefined;

  const isDemo = () => props.item.id === demo.id;
  const searchKey = () => `${props.item.type}:${props.video?.id || props.item.id}`;
  const preferredLanguage = (() => {
    try {
      const code = (navigator.language || 'en').split('-')[0];
      const names = new (Intl as unknown as { DisplayNames: new (l: string[], o: object) => { of(c: string): string } }).DisplayNames(['en'], { type: 'language' });
      return names.of(code) || 'English';
    } catch { return 'English'; }
  })();
  const ranking = createMemo(() => rankCandidates(candidates(), { language: preferredLanguage }));
  const subtitle = () => {
    const v = props.video;
    if (!v) return [props.item.releaseInfo, props.item.runtime].filter(Boolean).join(' · ');
    return [v.season !== undefined ? `Season ${v.season}` : '', v.episode !== undefined ? `Episode ${v.episode}` : '', v.title].filter(Boolean).join(' · ');
  };

  // ---------------------------------------------------------------- data

  async function refreshAccount() {
    try {
      const status = await api<AccountStatus>('account');
      if (alive) setAccount(status);
    } catch {
      if (alive) setBanner({ tone: 'error', text: "Bloob's local service isn't responding. Restart the app." });
    }
  }

  async function search() {
    searchOperation?.abort();
    const controller = new AbortController();
    searchOperation = controller;
    const key = searchKey();
    const searchStarted = performance.now();
    trace.record('sources', 'search-start');
    const cached = searchCache.get(key);
    if (cached && Date.now() - cached.at < SEARCH_TTL_MS) {
      searchOperation = null;
      searchCache.delete(key);
      searchCache.set(key, cached);
      setCandidates(cached.candidates);
      trace.record('sources', 'search-cache', { count: cached.candidates.length });
      setSearching(false);
      return;
    }
    setSearching(true);
    setSearchError('');
    try {
      const result = await api<{ sources: SourceCandidate[] }>('sources/search', { type: props.item.type, id: props.item.id, videoId: props.video?.id }, controller.signal);
      if (controller.signal.aborted || searchOperation !== controller) return;
      searchCache.set(key, { at: Date.now(), candidates: result.sources });
      if (searchCache.size > SEARCH_CACHE_LIMIT) searchCache.delete(searchCache.keys().next().value!);
      setCandidates(result.sources);
      trace.record('sources', 'search-ready', { count: result.sources.length, ms: performance.now() - searchStarted });
    } catch (error) {
      if (controller.signal.aborted || searchOperation !== controller) return;
      setSearchError((error as Error).message);
      trace.record('sources', 'search-failed', { ms: performance.now() - searchStarted });
    } finally {
      if (searchOperation === controller) { searchOperation = null; setSearching(false); }
    }
  }

  onMount(() => { void refreshAccount(); void search(); });
  onCleanup(() => {
    alive = false;
    operation?.abort();
    operation = null;
    searchOperation?.abort();
    searchOperation = null;
    pairGeneration++;
    window.clearInterval(pairTimer);
    window.clearInterval(tickTimer);
  });

  // ---------------------------------------------------------------- focus model

  const visibleItems = (group: { label: string; items: RankedCandidate[] }) =>
    expanded().has(group.label) ? group.items : group.items.slice(0, GROUP_PREVIEW);

  const items = createMemo<Item[]>(() => {
    switch (mode()) {
      case 'files': return files().map((file, i) => ({ key: `file:${i}`, run: () => continueWith(ctx => resolveFile(file, ctx, resolving() ?? undefined)) }));
      case 'qualities': return qualities().options.map((option, i) => ({ key: `quality:${i}`, run: () => continueWith(ctx => resolveQuality(qualities().linkId, option, ctx, resolving() ?? undefined)) }));
      case 'pairing': return pinExpired() ? [{ key: 'pin-renew', run: () => void startPairing() }] : [];
      case 'resolving': return [];
      case 'manual': return [
        { key: 'manual-input', run: () => manualInput?.focus() },
        { key: 'manual-go', run: submitManual },
      ];
    }
    const list: Item[] = [];
    const recommended = ranking().recommended;
    if (isDemo()) list.push({ key: 'test', run: playTest });
    if (recommended) list.push({ key: 'rec', run: () => choose(recommended.candidate) });
    for (const group of ranking().groups) {
      for (const ranked of visibleItems(group)) list.push({ key: `cand:${ranked.candidate.id}`, run: () => choose(ranked.candidate) });
      if (group.items.length > GROUP_PREVIEW) list.push({ key: `more:${group.label}`, run: () => toggleGroup(group.label) });
    }
    list.push({ key: 'manual', run: () => { setMode('manual'); setFocusKey('manual-input'); queueMicrotask(() => manualInput?.focus()); } });
    list.push({ key: 'account', run: () => { if (!account()?.paired) void startPairing(); } });
    return list;
  });

  // Keep focus on a real item; default to the first one.
  createEffect(() => {
    const list = items();
    if (!list.length) return;
    if (!list.some(item => item.key === focusKey())) setFocusKey(list[0].key);
  });
  createEffect(() => {
    const key = focusKey();
    if (!props.active) return;
    window.requestAnimationFrame(() => document.querySelector(`.sources-screen [data-key="${CSS.escape(key)}"]`)?.scrollIntoView({ block: 'nearest' }));
  });

  function moveFocus(step: number) {
    const list = items();
    const index = list.findIndex(item => item.key === focusKey());
    const next = list[Math.min(list.length - 1, Math.max(0, index + step))];
    if (next) setFocusKey(next.key);
  }

  function toggleGroup(label: string) {
    const next = new Set(expanded());
    if (next.has(label)) next.delete(label); else next.add(label);
    setExpanded(next);
  }

  const focused = (key: string) => focusKey() === key;

  // ---------------------------------------------------------------- pairing

  async function startPairing(then?: ResolvableSource) {
    if (!alive) return;
    const generation = ++pairGeneration;
    if (then) afterPairing = then;
    window.clearInterval(pairTimer);
    setPinExpired(false);
    setBanner(null);
    setMode('pairing');
    try {
      const started = await api<{ pin: string; url: string; expiresIn: number }>('pin/start', {});
      if (generation !== pairGeneration) return;
      setPin({ code: started.pin, url: started.url, expiresAt: Date.now() + (started.expiresIn || 600) * 1000 });
    } catch (error) {
      if (generation !== pairGeneration) return;
      setMode('list');
      setBanner({ tone: 'error', text: `Couldn't start pairing: ${(error as Error).message}` });
      return;
    }
    window.clearInterval(tickTimer);
    tickTimer = window.setInterval(() => {
      setNow(Date.now());
      if ((pin()?.expiresAt ?? 0) <= Date.now()) { setPinExpired(true); window.clearInterval(pairTimer); window.clearInterval(tickTimer); }
    }, 1000);
    pairTimer = window.setInterval(async () => {
      try {
        const status = await api<AccountStatus>('pin/check', {});
        if (generation !== pairGeneration) return;
        if (!status.paired) return;
        window.clearInterval(pairTimer);
        window.clearInterval(tickTimer);
        setAccount(status);
        setPin(null);
        setMode('list');
        setBanner({ tone: 'info', text: pairedMessage(status) });
        const next = afterPairing;
        afterPairing = null;
        if (next) void choose(next as SourceCandidate);
      } catch (error) {
        if (generation !== pairGeneration) return;
        if (error instanceof ApiError && error.status === 400) return; // not started yet
        setBanner({ tone: 'error', text: (error as Error).message });
      }
    }, 5000);
  }

  function cancelPairing() {
    pairGeneration++;
    window.clearInterval(pairTimer);
    window.clearInterval(tickTimer);
    afterPairing = null;
    setPin(null);
    setMode('list');
  }

  // ---------------------------------------------------------------- resolving

  function intent(): PlayIntent {
    return {
      contentId: props.item.id,
      contentType: props.item.type,
      videoId: props.video?.id,
      title: props.item.name,
      subtitle: props.video ? subtitle() : undefined,
      origin: isDemo() && !props.video ? 'test' : 'sources',
      requestedAt: operationStarted || performance.now(),
    };
  }

  function playTest() {
    operationStarted = performance.now();
    trace.record('sources', 'test-source');
    metrics.record({ name: 'resolve', at: Date.now(), ms: 0, kind: 'test', outcome: 'ready' });
    props.onPlay({ ...intent(), origin: 'test' }, { ...TEST_SOURCE, resolvedAt: Date.now() });
  }

  async function choose(source: ResolvableSource) {
    if (mode() === 'resolving') return;
    if (!account()) await refreshAccount();
    if (!alive) return;
    if (!account()?.paired) return void startPairing(source);
    trace.record('sources', `resolve-${source.kind}`);
    operationStarted = performance.now();
    setResolving(source);
    await run(ctx => resolveSource(source, ctx));
  }

  function continueWith(step: (ctx: Parameters<typeof resolveSource>[1]) => Promise<ResolveOutcome>) {
    void run(step);
  }

  /** Run one cancellable resolve step and route its outcome. */
  async function run(step: (ctx: Parameters<typeof resolveSource>[1]) => Promise<ResolveOutcome>) {
    operation?.abort();
    const controller = new AbortController();
    operation = controller;
    const kind = resolving()?.kind ?? 'link';
    setBanner(null);
    setProgress({ step: 'adding', label: 'Starting…' });
    setMode('resolving');
    try {
      const outcome = await step({
        season: props.video?.season, episode: props.video?.episode, signal: controller.signal,
        onProgress: progress => { if (operation === controller && !controller.signal.aborted) setProgress(progress); },
      });
      if (controller.signal.aborted || operation !== controller) return;
      const ms = Math.round(performance.now() - operationStarted);
      if (outcome.kind === 'ready') {
        metrics.record({ name: 'resolve', at: Date.now(), ms, kind, outcome: 'ready' });
        setMode('list');
        props.onPlay(intent(), outcome.source);
      } else if (outcome.kind === 'choose-file') {
        metrics.record({ name: 'resolve', at: Date.now(), ms, kind, outcome: 'choice' });
        setFiles(outcome.files);
        setMode('files');
        setFocusKey('file:0');
      } else {
        metrics.record({ name: 'resolve', at: Date.now(), ms, kind, outcome: 'choice' });
        setQualities({ linkId: outcome.linkId, options: outcome.qualities });
        setMode('qualities');
        setFocusKey('quality:0');
      }
    } catch (error) {
      if (operation !== controller) return;
      if (isAbort(error)) {
        metrics.record({ name: 'resolve', at: Date.now(), ms: Math.round(performance.now() - operationStarted), kind, outcome: 'cancelled' });
        return;
      }
      metrics.record({ name: 'resolve', at: Date.now(), ms: Math.round(performance.now() - operationStarted), kind, outcome: 'failed' });
      setMode('list');
      if (error instanceof ApiError && error.status === 401) {
        await refreshAccount();
        setBanner({ tone: 'error', text: (error as Error).message });
        return;
      }
      setBanner({ tone: 'error', text: `${(error as Error).message} Pick another source below.` });
    } finally {
      if (operation === controller) { operation = null; setProgress(null); }
    }
  }

  function cancelResolve() {
    operation?.abort();
    operation = null;
    setProgress(null);
    setMode('list');
    setBanner({ tone: 'info', text: 'Cancelled.' });
  }

  function submitManual() {
    const value = manualValue().trim();
    if (!value) return manualInput?.focus();
    const kind = value.startsWith('magnet:') || /^[a-f0-9]{40}$/i.test(value) ? 'magnet' : 'link';
    if (kind === 'link' && !/^https?:\/\//i.test(value)) return setBanner({ tone: 'error', text: 'Enter an http(s) link, a magnet link, or a 40-character info hash.' });
    manualInput?.blur();
    setMode('list');
    void choose({ kind, value });
  }

  // ---------------------------------------------------------------- input

  useKeys(key => {
    if (!props.active) return;
    if (key === 'back') {
      switch (mode()) {
        case 'resolving': return cancelResolve();
        case 'pairing': return cancelPairing();
        case 'files': case 'qualities': case 'manual':
          manualInput?.blur();
          setMode('list');
          return;
        default: return props.onBack();
      }
    }
    if (mode() === 'resolving') return;
    if (key === 'up') return moveFocus(-1);
    if (key === 'down') return moveFocus(1);
    if (key === 'enter') {
      if (mode() === 'manual' && focusKey() === 'manual-input' && document.activeElement === manualInput) return submitManual();
      items().find(item => item.key === focusKey())?.run();
    }
  });

  // ---------------------------------------------------------------- view

  return <div class="sources-screen">
    <div class="sources-backdrop" style={{ 'background-image': `url("${art.backdrop(props.item.id)}")` }} />
    <div class="sources-layout">
      <header class="sources-header">
        <div>
          <div class="sources-kicker">Choose how to watch</div>
          <h1>{props.item.name}</h1>
          <Show when={subtitle()}><p class="sources-subtitle">{subtitle()}</p></Show>
        </div>
        <AccountChip account={account()} />
      </header>

      <Show when={banner()}>{b => <div class="sources-banner" classList={{ error: b().tone === 'error' }} role="status">{b().text}</div>}</Show>

      <Show when={mode() === 'list' || mode() === 'manual'}>
        <main class="sources-main">
          <Show when={isDemo()}>
            <button class="source-row test" data-key="test" classList={{ focused: focused('test') }} onClick={playTest}>
              <div class="source-badges"><span class="badge">TEST</span></div>
              <div class="source-text"><b>Big Buck Bunny · bundled clip</b><small>480p · H.264 · AAC · 30 seconds · plays without AllDebrid</small></div>
              <span class="source-chevron">›</span>
            </button>
          </Show>

          <Show when={searching()}>
            <div class="source-skeleton"><div class="loading-mark small" /><span>Finding sources…</span></div>
          </Show>

          <Show when={!searching() && ranking().recommended}>{best =>
            <button class="source-hero" data-key="rec" classList={{ focused: focused('rec') }} onClick={() => choose(best().candidate)}>
              <div class="source-hero-label">Recommended</div>
              <div class="source-hero-main">
                <div>
                  <div class="source-hero-title">{headline(best().candidate)}</div>
                  <div class="source-hero-details">{details(best().candidate, bytes)}</div>
                </div>
                <span class="source-hero-action">{account()?.paired === false ? 'Connect & play' : 'Play'}</span>
              </div>
              <div class="source-hero-why">{ranking().why}</div>
              <Show when={best().warnings.length}><div class="source-warning">{best().warnings.join(' · ')}</div></Show>
              <div class="source-release">{best().candidate.release}</div>
            </button>
          }</Show>

          <Show when={!searching() && !ranking().recommended}>
            <div class="source-empty">
              <b>{searchError() ? "Couldn't search for sources" : candidates().length ? 'No recommended source' : 'No sources found'}</b>
              <span>{searchError() || (candidates().length ? 'The releases found are cinema recordings or unavailable. They are listed below if you want to try one.' : 'You can still paste a link or magnet below.')}</span>
            </div>
          </Show>

          <For each={searching() ? [] : ranking().groups}>{group =>
            <section class="source-group">
              <h2>{group.label === 'Other' ? 'Other quality' : group.label}<span>{group.items.length}</span></h2>
              <For each={visibleItems(group)}>{ranked => <CandidateRow ranked={ranked} focused={focused(`cand:${ranked.candidate.id}`)} onChoose={() => choose(ranked.candidate)} />}</For>
              <Show when={group.items.length > GROUP_PREVIEW}>
                <button class="source-more" data-key={`more:${group.label}`} classList={{ focused: focused(`more:${group.label}`) }} onClick={() => toggleGroup(group.label)}>
                  {expanded().has(group.label) ? 'Show fewer' : `Show all ${group.items.length}`}
                </button>
              </Show>
            </section>
          }</For>

          <section class="source-group">
            <Show when={mode() === 'manual'} fallback={
              <button class="source-row quiet" data-key="manual" classList={{ focused: focused('manual') }} onClick={() => setMode('manual')}>
                <div class="source-text"><b>Paste a link or magnet</b><small>For a source you already have</small></div>
                <span class="source-chevron">›</span>
              </button>
            }>
              <div class="source-manual">
                <input ref={manualInput} data-key="manual-input" classList={{ focused: focused('manual-input') }} value={manualValue()}
                  onInput={event => setManualValue(event.currentTarget.value)} placeholder="https://… or magnet:?xt=…" />
                <button data-key="manual-go" classList={{ focused: focused('manual-go') }} onClick={submitManual}>Resolve</button>
              </div>
            </Show>
            <button class="source-row quiet" data-key="account" classList={{ focused: focused('account') }} onClick={() => { if (!account()?.paired) void startPairing(); }}>
              <div class="source-text">
                <b>{account()?.paired ? 'AllDebrid connected' : 'Connect AllDebrid'}</b>
                <small>{account() ? (account()!.paired ? pairedMessage(account()!) : 'Needed to play indexed releases. Takes a minute, once.') : 'Checking…'}</small>
              </div>
            </button>
          </section>
          <p class="sources-footnote">Quality details come from release names and are confirmed only when playback starts.</p>
        </main>
      </Show>

      <Show when={mode() === 'pairing'}>
        <div class="sources-panel pairing">
          <h2>Connect AllDebrid</h2>
          <Show when={pin()} fallback={<div class="loading-mark small" />}>{p => <>
            <ol>
              <li>On your phone or computer, open <b>{p().url.replace(/^https?:\/\//, '')}</b></li>
              <li>Enter this code</li>
            </ol>
            <div class="pin-code" aria-label="Pairing code">{p().code}</div>
            <Show when={!pinExpired()} fallback={
              <button class="source-more" data-key="pin-renew" classList={{ focused: focused('pin-renew') }} onClick={() => startPairing()}>The code expired · get a new one</button>
            }>
              <p class="pin-wait"><span class="pulse" /> Waiting for you to confirm… code expires in {countdown(p().expiresAt - now())}</p>
            </Show>
            <p class="pin-note">This TV remembers the connection, so you only do this once.</p>
          </>}</Show>
          <p class="panel-hint">Back to cancel</p>
        </div>
      </Show>

      <Show when={mode() === 'resolving'}>
        <div class="sources-panel resolving" role="status">
          <h2>{resolving()?.claims ? headline(resolving() as SourceCandidate) : 'Preparing your source'}</h2>
          <Steps progress={progress()} magnet={resolving()?.kind === 'magnet'} />
          <p class="panel-hint">Back to cancel</p>
        </div>
      </Show>

      <Show when={mode() === 'files'}>
        <div class="sources-panel choice">
          <h2>Which file?</h2>
          <p>This release has several videos and Bloob couldn't tell which one you want.</p>
          <For each={files()}>{(file, i) =>
            <button class="source-row" data-key={`file:${i()}`} classList={{ focused: focused(`file:${i()}`) }} onClick={() => continueWith(ctx => resolveFile(file, ctx, resolving() ?? undefined))}>
              <div class="source-text"><b>{file.name}</b><small>{[bytes(file.size), file.path && file.path !== file.name ? file.path.split('/').slice(0, -1).join(' / ') : ''].filter(Boolean).join(' · ')}</small></div>
              <span class="source-chevron">›</span>
            </button>
          }</For>
          <p class="panel-hint">Back to sources</p>
        </div>
      </Show>

      <Show when={mode() === 'qualities'}>
        <div class="sources-panel choice">
          <h2>Which version?</h2>
          <p>This link offers several versions.</p>
          <For each={qualities().options}>{(option, i) =>
            <button class="source-row" data-key={`quality:${i()}`} classList={{ focused: focused(`quality:${i()}`) }} onClick={() => continueWith(ctx => resolveQuality(qualities().linkId, option, ctx, resolving() ?? undefined))}>
              <div class="source-badges"><span class="badge">{String(option.quality || 'Video')}</span></div>
              <div class="source-text"><b>{option.ext ? option.ext.toUpperCase() : 'Video stream'}</b><small>{bytes(option.filesize)}</small></div>
              <span class="source-chevron">›</span>
            </button>
          }</For>
          <p class="panel-hint">Back to sources</p>
        </div>
      </Show>
    </div>
  </div>;
}

function CandidateRow(props: { ranked: RankedCandidate; focused: boolean; onChoose: () => void }) {
  const c = () => props.ranked.candidate;
  return <button class="source-row" data-key={`cand:${c().id}`} classList={{ focused: props.focused, dim: props.ranked.excluded }} onClick={props.onChoose}>
    <div class="source-badges">
      <span class="badge strong">{c().claims.resolution ?? '?'}</span>
      <Show when={c().claims.hdr?.length}><span class="badge">{c().claims.hdr!.includes('DV') ? 'DV' : c().claims.hdr![0]}</span></Show>
      <Show when={c().claims.audio?.includes('Atmos')}><span class="badge">Atmos</span></Show>
    </div>
    <div class="source-text">
      <b>{details(c(), bytes) || 'No details reported'}</b>
      <small>
        <Show when={props.ranked.warnings.length}><em>{props.ranked.warnings[0]} · </em></Show>
        {c().release}
      </small>
    </div>
    <span class="source-chevron">›</span>
  </button>;
}

function AccountChip(props: { account: AccountStatus | null }) {
  return <div class="account-chip" classList={{ ok: !!props.account?.paired, warn: !!props.account?.paired && !props.account.persistent }}>
    <span class="dot" />
    {!props.account ? 'AllDebrid…' : props.account.paired ? (props.account.persistent ? 'AllDebrid connected' : 'AllDebrid · not saved') : 'AllDebrid not connected'}
  </div>;
}

const STEPS: { step: ResolveProgress['step']; label: string; magnetOnly?: boolean }[] = [
  { step: 'adding', label: 'Send to AllDebrid', magnetOnly: true },
  { step: 'caching', label: 'Download on AllDebrid', magnetOnly: true },
  { step: 'files', label: 'Find the video file', magnetOnly: true },
  { step: 'unlocking', label: 'Get the stream' },
];

function Steps(props: { progress: ResolveProgress | null; magnet: boolean }) {
  const steps = () => STEPS.filter(s => props.magnet || !s.magnetOnly);
  const current = () => {
    const step = props.progress?.step === 'preparing' ? 'unlocking' : props.progress?.step;
    return Math.max(0, steps().findIndex(s => s.step === step));
  };
  return <ol class="resolve-steps">
    <For each={steps()}>{(s, i) =>
      <li classList={{ done: i() < current(), active: i() === current() }}>
        <span class="marker" />
        <span>{i() === current() && props.progress ? props.progress.label : s.label}</span>
        <Show when={i() === current() && props.progress?.fraction !== undefined}>
          <span class="resolve-meter"><span style={{ transform: `scaleX(${props.progress!.fraction})` }} /></span>
          <small>{Math.round((props.progress!.fraction || 0) * 100)}%</small>
        </Show>
      </li>
    }</For>
  </ol>;
}

function pairedMessage(status: AccountStatus) {
  if (!status.paired) return 'Not connected';
  switch (status.storage) {
    case 'app-data': case 'home': case 'configured': return 'Saved on this TV. You will not need the code again.';
    case 'service': return 'Saved on this TV until the app is reinstalled.';
    case 'temporary': return 'Saved until the TV restarts. Longer-term storage was unavailable.';
    default: return `Connected for now, but this TV refused to save it (${status.attempts.map(a => `${a.location}: ${a.result}`).join(', ') || 'no storage'}).`;
  }
}

function countdown(ms: number) {
  const s = Math.max(0, Math.floor(ms / 1000));
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
}
