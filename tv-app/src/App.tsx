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
  const [sourcesFor, setSourcesFor] = createSignal<{ video?: Video } | null>(null);
  const [playback, setPlayback] = createSignal<Playback | null>(null);

  const closeDetails = () => { setSourcesFor(null); setTitle(null); };
  const openSources = (video?: Video) => setSourcesFor({ video });

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
      <Show when={title() && sourcesFor() ? { item: title()!, video: sourcesFor()!.video } : null} keyed>
        {(target) => (
          <div class="screen" classList={{ hidden: !!playback() }}>
            <Sources item={target.item} video={target.video} active={!playback()}
              onBack={() => setSourcesFor(null)}
              onPlay={(intent, source) => setPlayback({ intent, source })} />
          </div>
        )}
      </Show>
      <Show when={playback()} keyed>
        {(session) => <Player intent={session.intent} source={session.source} onExit={() => setPlayback(null)} />}
      </Show>
    </>
  );
}
