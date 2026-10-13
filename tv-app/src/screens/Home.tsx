import { createEffect, createMemo, createSignal, For, Index, onCleanup, Show } from 'solid-js';
import { createStore } from 'solid-js/store';
import { Backdrop } from '../components/Backdrop';
import { MetaInfo } from '../components/MetaInfo';
import { Row } from '../components/Row';
import { Tile } from '../components/Tile';
import { PlayIcon } from '../components/icons';
import { art } from '../data/cinemeta';
import type { MetaPreview, Row as RowData } from '../data/types';
import { useKeys } from '../keys';

const TABS = ['Home', 'Movies', 'TV Shows'];
const HERO_COUNT = 6;
const HERO_INTERVAL_MS = 9000;
const ROWS_TOP = 820; // .rows top
const ROW_H = 340; // .row height
const FOCUSED_ROW_Y = 120; // where the focused row title sits on screen

type Zone = 'nav' | 'hero' | 'rows';

export function Home(props: {
  rows: RowData[];
  active: boolean;
  onOpen: (item: MetaPreview) => void;
}) {
  const [s, set] = createStore({
    zone: 'hero' as Zone,
    nav: 0,
    section: 0,
    hero: 0,
    heroSwap: false,
    row: 0,
    cols: [] as number[],
  });

  const visibleRows = createMemo(() => {
    const type = s.section === 1 ? 'movie' : s.section === 2 ? 'series' : null;
    return type ? props.rows.filter(row => row.id !== 'demo').map(row => ({ ...row, items: row.items.filter(item => item.type === type) })).filter(row => row.items.length) : props.rows;
  });
  const hero = createMemo(() => (visibleRows()[0]?.items ?? []).slice(0, HERO_COUNT));

  const col = (r: number) => s.cols[r] ?? 0;
  const focusedItem = () => visibleRows()[s.row]?.items[col(s.row)];
  let swapTimer: number | undefined;

  const showHero = (i: number) => {
    const n = hero().length;
    if (!n) return;
    window.clearTimeout(swapTimer);
    set('heroSwap', true);
    swapTimer = window.setTimeout(() => set({ hero: (i + n) % n, heroSwap: false }), 220);
  };

  const activateSection = (index: number) => {
    window.clearTimeout(swapTimer);
    set({ nav: index, section: index, hero: 0, row: 0, cols: [], heroSwap: false, zone: 'hero' });
  };

  // Warm the cache for the next hero slide so rotation never waits on network.
  createEffect(() => {
    if (!props.active) return;
    const next = hero()[(s.hero + 1) % Math.max(1, hero().length)];
    if (next) new Image().src = art.backdrop(next.id);
  });

  // A hidden screen has no active rotation timer.
  createEffect(() => {
    if (!props.active || s.zone !== 'hero' || hero().length < 2) return;
    const rotation = window.setInterval(() => showHero(s.hero + 1), HERO_INTERVAL_MS);
    onCleanup(() => window.clearInterval(rotation));
  });
  onCleanup(() => window.clearTimeout(swapTimer));

  useKeys((key) => {
    if (!props.active) return;
    const rows = visibleRows();
    switch (s.zone) {
      case 'nav':
        if (key === 'left') set('nav', Math.max(0, s.nav - 1));
        if (key === 'right') set('nav', Math.min(TABS.length - 1, s.nav + 1));
        if (key === 'down') set('zone', 'hero');
        if (key === 'enter') activateSection(s.nav);
        break;
      case 'hero':
        if (key === 'left') showHero(s.hero - 1);
        if (key === 'right') showHero(s.hero + 1);
        if (key === 'up') set('zone', 'nav');
        if (key === 'down' && rows.length) set({ zone: 'rows', row: 0 });
        if (key === 'enter' && hero()[s.hero]) props.onOpen(hero()[s.hero]);
        break;
      case 'rows': {
        const n = rows[s.row].items.length;
        if (key === 'left') set('cols', s.row, Math.max(0, col(s.row) - 1));
        if (key === 'right') set('cols', s.row, Math.min(n - 1, col(s.row) + 1));
        if (key === 'up') s.row === 0 ? set('zone', 'hero') : set('row', s.row - 1);
        if (key === 'down') set('row', Math.min(rows.length - 1, s.row + 1));
        if (key === 'enter' && focusedItem()) props.onOpen(focusedItem()!);
        break;
      }
    }
  });

  const scrollY = () => (s.zone === 'rows' ? -(ROWS_TOP + s.row * ROW_H - FOCUSED_ROW_Y) : 0);
  const backdropSrc = () => {
    const item = s.zone === 'rows' ? focusedItem() : hero()[s.hero];
    return item && art.backdrop(item.id);
  };

  return (
    <>
      <Backdrop src={backdropSrc()} dim={s.zone === 'rows' ? 0.4 : 1} debounceMs={s.zone === 'rows' ? 350 : 0} />

      <nav class="nav" classList={{ away: s.zone === 'rows' }}>
        <For each={TABS}>
          {(tab, i) => (
            <button
              class="nav-tab"
              classList={{ current: i() === s.section, focused: s.zone === 'nav' && i() === s.nav }}
              onClick={() => activateSection(i())}
            >
              {tab}
            </button>
          )}
        </For>
      </nav>

      <div class="home-scroll" style={{ transform: `translate3d(0, ${scrollY()}px, 0)` }}>
        <Show when={s.section !== 0 && !visibleRows().length}>
          <div class="section-empty">{s.section === 1 ? 'Movies' : 'TV shows'} are unavailable right now. Try Home while the catalog reconnects.</div>
        </Show>
        <div class="hero" classList={{ away: s.zone === 'rows' }}>
          <Show when={hero()[s.hero]}>
            {(item) => (
              <div class="hero-info" classList={{ swap: s.heroSwap }}>
                <MetaInfo item={item()} />
                <div class="details-buttons">
                  <div class="btn" classList={{ focused: s.zone === 'hero' }}>
                    <PlayIcon /> Play
                  </div>
                </div>
              </div>
            )}
          </Show>
          <div class="hero-dots">
            <Index each={hero()}>{(_, i) => <div class="hero-dot" classList={{ on: i === s.hero }} />}</Index>
          </div>
        </div>

        <div class="rows">
          <For each={visibleRows()}>
            {(row, r) => (
              <Row
                title={row.title}
                items={row.items}
                focusedIndex={col(r())}
                active={s.zone === 'rows' && s.row === r()}
              >
                {(item, focused) => <Tile item={item} focused={focused()} />}
              </Row>
            )}
          </For>
        </div>
      </div>
    </>
  );
}
