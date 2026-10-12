import type { ContentSource, ContentType, MetaDetails, MetaPreview, Row } from './types';

const CINEMETA = 'https://v3-cinemeta.strem.io';
const ART = 'https://images.metahub.space';
export const demo: MetaPreview = {
  id: 'tt1254207', type: 'movie', name: 'Big Buck Bunny',
  description: 'Playback test film', releaseInfo: '2008', runtime: '10 min',
  background: `${ART}/background/medium/tt1254207/img`,
};
export const demoRow: Row = { id: 'demo', title: 'Playback Test', items: [demo] };

// metahub backgrounds: small is 480x270, medium is 1080p, large can be 2560px
// wide (too heavy to decode on the TV for no visible gain). Tiles use small;
// full-screen surfaces use medium.
export const art = {
  tile: (id: string) => `${ART}/background/small/${id}/img`,
  backdrop: (id: string) => `${ART}/background/medium/${id}/img`,
  logo: (id: string) => `${ART}/logo/medium/${id}/img`,
};

async function get<T>(path: string): Promise<T> {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 6000);
  try {
    const res = await fetch(CINEMETA + path, { signal: controller.signal });
    if (!res.ok) throw new Error(`${path}: ${res.status}`);
    return res.json();
  } finally {
    clearTimeout(timeout);
  }
}

function toPreview(m: any): MetaPreview {
  return {
    id: m.id,
    type: m.type,
    name: m.name,
    description: m.description,
    releaseInfo: m.releaseInfo,
    runtime: m.runtime,
    genres: m.genres || m.genre,
    imdbRating: m.imdbRating,
    poster: m.poster,
    background: m.background,
    logo: m.logo,
  };
}

async function catalog(type: ContentType, id: string, extra = ''): Promise<MetaPreview[]> {
  const data = await get<{ metas: any[] }>(`/catalog/${type}/${id}${extra}.json`);
  // Items without artwork break the visual rhythm of a row; skip them.
  return data.metas.filter((m) => m.background && m.id.startsWith('tt')).map(toPreview);
}

export const cinemeta: ContentSource = {
  async homeRows(): Promise<Row[]> {
    const specs: [string, string, ContentType, string, string?][] = [
      ['top-movies', 'Popular Movies', 'movie', 'top'],
      ['top-series', 'Popular Series', 'series', 'top'],
      ['rated-movies', 'Top Rated Movies', 'movie', 'imdbRating'],
      ['rated-series', 'Top Rated Series', 'series', 'imdbRating'],
      ['scifi', 'Sci-Fi', 'movie', 'top', '/genre=Sci-Fi'],
      ['animation', 'Animation', 'movie', 'top', '/genre=Animation'],
    ];
    const rows = await Promise.all(
      specs.map(async ([id, title, type, cat, extra]) => ({
        id,
        title,
        items: (await catalog(type, cat, extra).catch(() => [])).slice(0, 20),
      })),
    );
    return [demoRow, ...rows.filter((r) => r.items.length > 0)];
  },

  async meta(type, id) {
    if (type === 'movie' && id === demo.id) return { ...demo, videos: [] };
    const { meta: m } = await get<{ meta: any }>(`/meta/${type}/${id}.json`);
    const details: MetaDetails = {
      ...toPreview(m),
      cast: m.cast,
      director: m.director,
      videos: (m.videos || []).map((v: any) => ({
        id: v.id,
        title: v.name || v.title,
        season: v.season,
        episode: v.episode ?? v.number,
        released: v.released,
        overview: v.overview || v.description,
        thumbnail: v.thumbnail,
      })),
      trailerYtId: m.trailerStreams?.[0]?.ytId,
    };
    return details;
  },
};
