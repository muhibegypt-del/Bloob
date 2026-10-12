import { createSignal, onMount, Show } from 'solid-js';
import { cinemeta, demoRow } from './data/cinemeta';
import type { MetaPreview, Row, Video } from './data/types';
import { Details } from './screens/Details';
import { Home } from './screens/Home';
import { Sources } from './screens/Sources';
import { Player } from './screens/Player';

const source = cinemeta;

export function App() {
  const [rows, setRows] = createSignal<Row[]>([demoRow]);
  onMount(() => {
    source.homeRows().then(setRows).catch((error) => console.warn('Catalog unavailable:', error));
  });
  const [open, setOpen] = createSignal<MetaPreview | null>(null);
  const [selectedVideo, setSelectedVideo] = createSignal<Video | undefined>();
  const [showSources, setShowSources] = createSignal(false);
  const [playback, setPlayback] = createSignal<{ url: string; title: string } | null>(null);

  const closeDetails = () => { setOpen(null); setSelectedVideo(undefined); };
  const chooseSource = (video?: Video) => { setSelectedVideo(video); setShowSources(true); };

  return (
    <>
      {/* Home stays mounted underneath Details so its focus and scroll survive. */}
      <div class="screen" classList={{ hidden: !!open() || !!playback() }}>
        <Home rows={rows()} active={!open()} onOpen={setOpen} />
      </div>
      <Show when={open() && !showSources() && !playback() ? open() : null} keyed>
        {(item) => (
          <div class="screen">
            <Details source={source} item={item} active onBack={closeDetails} onPlay={chooseSource} />
          </div>
        )}
      </Show>
      <Show when={open() && showSources() && !playback() ? open() : null} keyed>
        {(item) => <Sources item={item} video={selectedVideo()} onBack={() => setShowSources(false)} onPlay={(url, title) => setPlayback({ url, title })} />}
      </Show>
      <Show when={playback()} keyed>
        {(session) => <Player url={session.url} title={session.title} onBack={() => setPlayback(null)} />}
      </Show>
    </>
  );
}
