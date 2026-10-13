import { createSignal, onMount, Show } from 'solid-js';
import type { PlayIntent, ResolvedSource } from './buna';
import { cinemeta, demoRow } from './data/cinemeta';
import type { MetaPreview, Row, Video } from './data/types';
import { Details } from './screens/Details';
import { Home } from './screens/Home';
import { Player } from './screens/Player';
import { Sources } from './screens/Sources';

const catalog = cinemeta;

/** Navigation state. Each layer stays mounted beneath the next so focus and scroll survive Back. */
interface Playback { intent: PlayIntent; source: ResolvedSource }

export function App() {
  const [rows, setRows] = createSignal<Row[]>([demoRow]);
  onMount(() => {
    catalog.homeRows().then(setRows).catch((error) => console.warn('Catalog unavailable:', error));
  });
  const [title, setTitle] = createSignal<MetaPreview | null>(null);
  const [sourcesFor, setSourcesFor] = createSignal<{ attemptId: number; video?: Video; episodeOrder?: Video[] } | null>(null);
  const [playback, setPlayback] = createSignal<Playback | null>(null);
  let nextAttemptId = 0;

  const closeDetails = () => { setSourcesFor(null); setPlayback(null); setTitle(null); };
  const openSources = (video?: Video, episodeOrder?: Video[]) => setSourcesFor({ attemptId: ++nextAttemptId, video, episodeOrder });
  const closeSources = (attemptId: number) => {
    if (sourcesFor()?.attemptId === attemptId) setSourcesFor(null);
  };
  const acceptSource = (attemptId: number, intent: PlayIntent, source: ResolvedSource) => {
    if (sourcesFor()?.attemptId !== attemptId || playback()) return;
    setPlayback({ intent, source });
  };
  const nextEpisode = () => {
    const current = sourcesFor();
    const index = current?.episodeOrder?.findIndex(video => video.id === current.video?.id) ?? -1;
    return index >= 0 ? current?.episodeOrder?.[index + 1] : undefined;
  };
  const openNextEpisode = (video: Video) => {
    const current = sourcesFor();
    if (!current?.episodeOrder?.some(episode => episode.id === video.id)) return;
    setPlayback(null);
    setSourcesFor({ attemptId: ++nextAttemptId, video, episodeOrder: current.episodeOrder });
  };

  return (
    <>
      <div class="screen" classList={{ hidden: !!title() || !!playback() }}>
        <Home rows={rows()} active={!title()} onOpen={setTitle} />
      </div>
      <Show when={title()} keyed>
        {(item) => (
          <div class="screen" classList={{ hidden: !!sourcesFor() || !!playback() }}>
            <Details source={catalog} item={item} active={!sourcesFor() && !playback()} onBack={closeDetails} onPlay={openSources} />
          </div>
        )}
      </Show>
      <Show when={title() && sourcesFor() ? { item: title()!, ...sourcesFor()! } : null} keyed>
        {(target) => (
          <div class="screen" classList={{ hidden: !!playback() }}>
            <Sources item={target.item} video={target.video} active={!playback()}
              onBack={() => closeSources(target.attemptId)}
              onPlay={(intent, source) => acceptSource(target.attemptId, intent, source)} />
          </div>
        )}
      </Show>
      <Show when={playback()} keyed>
        {(session) => <Player intent={session.intent} source={session.source} nextEpisode={nextEpisode()}
          onNextEpisode={openNextEpisode} onExit={() => setPlayback(null)} />}
      </Show>
    </>
  );
}
