import { describe, expect, it } from 'vitest';
import torznabModule from '../../service/torznab.js';

const { parseFeed, searchUrl, publicFeed, invalidFeed } = torznabModule;

const FEED = `<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0" xmlns:torznab="http://torznab.com/schemas/2015/feed">
<channel><title>Tracker</title>
<item>
  <title>Film.2024.2160p.UHD.BluRay.REMUX.DV.HDR10.HEVC.TrueHD.7.1.Atmos-GRP</title>
  <guid>https://tracker.example/torrents/1</guid>
  <link>http://192.168.1.20:9696/12/download?apikey=SECRET&amp;link=abc</link>
  <size>74200000000</size>
  <enclosure url="http://192.168.1.20:9696/12/download?apikey=SECRET&amp;link=abc" length="74200000000" type="application/x-bittorrent" />
  <torznab:attr name="seeders" value="41" />
  <torznab:attr name="infohash" value="ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCD" />
  <torznab:attr name="imdb" value="0133093" />
</item>
<item>
  <title><![CDATA[Film 2024 1080p & more]]></title>
  <link>magnet:?xt=urn:btih:1111111111111111111111111111111111111111</link>
  <size>9000000000</size>
</item>
</channel></rss>`;

describe('Torznab feeds', () => {
  it('reads items, attributes, entities and CDATA', () => {
    const items = parseFeed(FEED);
    expect(items).toHaveLength(2);
    expect(items[0]).toMatchObject({
      title: 'Film.2024.2160p.UHD.BluRay.REMUX.DV.HDR10.HEVC.TrueHD.7.1.Atmos-GRP',
      link: 'http://192.168.1.20:9696/12/download?apikey=SECRET&link=abc',
      size: 74200000000,
      seeders: 41,
      infohash: 'abcdefabcdefabcdefabcdefabcdefabcdefabcd',
      imdb: '0133093',
    });
    expect(items[1].title).toBe('Film 2024 1080p & more');
    expect(items[1].magnet).toMatch(/^magnet:/);
    expect(items[1].seeders).toBeUndefined();
  });

  it('reports an indexer error instead of an empty list', () => {
    expect(() => parseFeed('<error code="100" description="Incorrect user credentials" />')).toThrow(/Incorrect user credentials/);
  });

  it('builds film and episode searches by IMDb number', () => {
    const feed = { url: 'http://192.168.1.20:9696/12/api', apikey: 'k' };
    const film = new URL(searchUrl(feed, 'movie', 'tt0133093'));
    expect(film.searchParams.get('t')).toBe('movie');
    expect(film.searchParams.get('imdbid')).toBe('0133093');
    expect(film.searchParams.get('apikey')).toBe('k');
    const ep = new URL(searchUrl(feed, 'series', 'tt0944947:1:10'));
    expect(ep.searchParams.get('t')).toBe('tvsearch');
    expect(ep.searchParams.get('season')).toBe('1');
    expect(ep.searchParams.get('ep')).toBe('10');
  });

  it('shows only a name and host, and validates what is saved', () => {
    expect(publicFeed({ name: 'Mine', url: 'http://192.168.1.20:9696/12/api', apikey: 'secret' })).toEqual({ name: 'Mine', host: '192.168.1.20:9696' });
    expect(invalidFeed({ url: 'ftp://x', apikey: 'a' })).toMatch(/http/);
    expect(invalidFeed({ url: 'http://x/api', apikey: '' })).toMatch(/API key/);
    expect(invalidFeed({ name: 'Mine', url: 'http://x/api', apikey: 'a' })).toBe('');
  });
});
