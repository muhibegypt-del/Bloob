import { describe, expect, it } from 'vitest';
import release from '../../service/release.js';

const { parseRelease, parseStream, parseStreams } = release;
const HASH = 'a'.repeat(40);

describe('parseRelease', () => {
  it('reads a 4K remux with Dolby Vision and Atmos', () => {
    expect(parseRelease('Dune.Part.Two.2024.2160p.UHD.BluRay.REMUX.DV.HDR10.HEVC.TrueHD.Atmos.7.1-GRP')).toEqual({
      resolution: '4K', hdr: ['DV', 'HDR10'], videoCodec: 'HEVC', audio: ['Atmos', 'TrueHD'],
      channels: '7.1', container: undefined, sourceType: 'REMUX',
    });
  });

  it('reads a 1080p web release', () => {
    expect(parseRelease('Show.S01E03.1080p.WEB-DL.DDP5.1.H.264-GRP.mkv')).toMatchObject({
      resolution: '1080p', videoCodec: 'H.264', audio: ['Dolby Digital Plus'], channels: '5.1', sourceType: 'WEB-DL', container: 'MKV',
    });
  });

  it('collapses generic HDR into the specific label and DTS into DTS-HD MA', () => {
    expect(parseRelease('Film 2160p HDR10+ HDR x265 DTS-HD MA 5.1')).toMatchObject({ hdr: ['HDR10+'], audio: ['DTS-HD MA'] });
  });

  it('flags cinema recordings', () => {
    expect(parseRelease('New.Film.2026.HDCAM.x264').sourceType).toBe('CAM');
    expect(parseRelease('New.Film.2026.HDTS.x264').sourceType).toBe('TS');
  });

  it('reads audio codecs stuck to their channel count', () => {
    const cases = [
      ['Film.2020.1080p.WEB.AAC2.0.x264', 'AAC', '2.0'],
      ['Film.2020.1080p.BluRay.DTS5.1.x264', 'DTS', '5.1'],
      ['Film.2020.1080p.WEB-DL.DDP5.1.H.264', 'Dolby Digital Plus', '5.1'],
      ['Film.2020.1080p.BluRay.FLAC2.0.x264', 'FLAC', '2.0'],
      ['Film.2020.2160p.BluRay.TrueHD7.1.Atmos', 'TrueHD', '7.1'],
      ['Film.2020.1080p.WEB.Opus2.0.x265', 'Opus', '2.0'],
      ['Film.2020.720p.HDTV.AC3.5.1.x264', 'Dolby Digital', '5.1'],
      ['Film.2020.1080p.BluRay.DTS-HD.MA5.1.x264', 'DTS-HD MA', '5.1'],
    ];
    for (const [name, codec, channels] of cases) {
      const parsed = parseRelease(name);
      expect(parsed.audio, name).toContain(codec);
      expect(parsed.channels, name).toBe(channels);
    }
  });

  it('lets an explicit 1080p or 720p win over UHD', () => {
    expect(parseRelease('Movie.2020.1080p.UHD.BluRay.x264-GRP').resolution).toBe('1080p');
    expect(parseRelease('Movie.2020.UHD.720p.WEB').resolution).toBe('720p');
    expect(parseRelease('Movie.2020.UHD.BluRay.x265').resolution).toBe('4K');
    expect(parseRelease('Movie.2020.4K.HDR.WEB').resolution).toBe('4K');
  });

  it('reads REMUX wherever it appears', () => {
    expect(parseRelease('Film.2019.1080p.BDRemux.AVC.DTS-HD.MA.5.1').sourceType).toBe('REMUX');
    expect(parseRelease('Film.2019.1080p.BD-Remux.AVC').sourceType).toBe('REMUX');
    expect(parseRelease('Film 2019 2160p UHD BluRay REMUX HEVC').sourceType).toBe('REMUX');
  });

  it('does not take the source from a title word', () => {
    expect(parseRelease('Cam.2018.1080p.NF.WEB-DL.DDP5.1.H.264-GRP').sourceType).toBe('WEB-DL');
    expect(parseRelease('Charlottes.Web.1973.DVDRip.x264').sourceType).toBe('DVD');
    expect(parseRelease('Web.Therapy.S01E01.720p.HDTV.x264').sourceType).toBe('HDTV');
    expect(parseRelease('Cam.2018').sourceType).toBeUndefined();
    // Words no title uses still count anywhere.
    expect(parseRelease('[HDCAM] New Film 2026').sourceType).toBe('CAM');
  });

  it('flags 3D versions, disc images and samples', () => {
    expect(parseRelease('Avatar.2009.1080p.3D.HSBS.BluRay.x264').threeD).toBe(true);
    expect(parseRelease('Film.2012.1080p.BluRay.Half-SBS.x264').threeD).toBe(true);
    expect(parseRelease('Film.2012.1080p.BluRay.H-OU.x264').threeD).toBe(true);
    expect(parseRelease('Film.2012.1080p.3D.BluRay.SBS').threeD).toBe(true);
    // "3D" in the title of a flat release is not a claim.
    expect(parseRelease('Step.Up.3D.2010.1080p.BluRay.x264').threeD).toBeUndefined();
    expect(parseRelease('Film.2020.COMPLETE.UHD.BLURAY-GRP').discImage).toBe(true);
    expect(parseRelease('Film.2020.COMPLETE.BLURAY-GRP').discImage).toBe(true);
    expect(parseRelease('Film.2020.1080p.BluRay.iso').discImage).toBe(true);
    expect(parseRelease('Film 2020 BDMV').discImage).toBe(true);
    expect(parseRelease('Film.2020.1080p.WEB-DL.sample.mkv').sample).toBe(true);
    expect(parseRelease('Film.2020.1080p.WEB-DL.mkv')).toMatchObject({ threeD: undefined, discImage: undefined, sample: undefined });
  });

  it('does not invent properties', () => {
    expect(parseRelease('Some Movie')).toEqual({
      resolution: undefined, hdr: undefined, videoCodec: undefined, audio: undefined, channels: undefined, container: undefined, sourceType: undefined,
    });
  });
});

describe('parseStream', () => {
  const movie = {
    name: 'Torrentio\n4k DV | HDR',
    title: 'Dune.Part.Two.2024.2160p.WEB-DL.DV.HDR.HEVC.DDP5.1.Atmos-GRP\n👤 245 💾 18.4 GB ⚙️ ThePirateBay\nMulti Audio / 🇬🇧 / 🇫🇷 / 🇮🇹',
    infoHash: HASH.toUpperCase(),
    fileIdx: 0,
    behaviorHints: { bingeGroup: 'torrentio|4k|WEB-DL|DV|HDR', filename: 'Dune.Part.Two.2024.2160p.WEB-DL.DV.HDR.HEVC.DDP5.1.Atmos-GRP.mkv' },
  };

  it('builds a structured candidate from the classic title format', () => {
    expect(parseStream(movie, 'Torrentio')).toEqual({
      id: `magnet:${HASH}#0`, provider: 'Torrentio', kind: 'magnet', value: HASH,
      release: 'Dune.Part.Two.2024.2160p.WEB-DL.DV.HDR.HEVC.DDP5.1.Atmos-GRP',
      fileName: 'Dune.Part.Two.2024.2160p.WEB-DL.DV.HDR.HEVC.DDP5.1.Atmos-GRP.mkv',
      fileIdx: 0,
      claims: { resolution: '4K', hdr: ['DV', 'HDR'], videoCodec: 'HEVC', audio: ['Atmos', 'Dolby Digital Plus'], channels: '5.1', container: 'MKV' },
      sourceType: 'WEB-DL', sizeBytes: 18_400_000_000, seeders: 245,
      languages: ['English', 'French', 'Italian'], multiAudio: true, claimsFrom: 'release-name',
    });
  });

  it('accepts the newer description field and a pack file line', () => {
    const candidate = parseStream({
      name: 'Torrentio\n1080p',
      description: 'Show.S01.COMPLETE.1080p.BluRay.x264-GRP\nShow.S01E03.1080p.BluRay.x264-GRP.mkv\n👤 31 💾 2.1 GB ⚙️ 1337x',
      infoHash: HASH, fileIdx: 2,
    }, 'Torrentio');
    expect(candidate).toMatchObject({ pack: true, fileName: 'Show.S01E03.1080p.BluRay.x264-GRP.mkv', fileIdx: 2, seeders: 31, sizeBytes: 2_100_000_000, languages: [] });
  });

  it('reads the source from the release name, not from a title word in the file name', () => {
    const candidate = parseStream({
      title: 'Charlottes.Web.1973.DVDRip.x264-GRP\n👤 12 💾 1.4 GB ⚙️ X',
      infoHash: HASH,
      behaviorHints: { filename: 'Charlottes.Web.1973.DVDRip.x264-GRP.mkv' },
    }, 'X');
    expect(candidate.sourceType).toBe('DVD');
  });

  it('carries 3D, disc image and sample claims', () => {
    const claims = (title) => parseStream({ title, infoHash: HASH }, 'X').claims;
    expect(claims('Avatar.2009.1080p.3D.HSBS.BluRay.x264')).toMatchObject({ threeD: true });
    expect(claims('Film.2020.COMPLETE.UHD.BLURAY-GRP')).toMatchObject({ discImage: true });
    expect(claims('Film.2020.1080p.WEB-DL.sample')).toMatchObject({ sample: true });
    expect(claims('Film.2020.1080p.WEB-DL')).not.toHaveProperty('threeD');
  });

  it('prefers the indexer-reported byte size', () => {
    expect(parseStream({ ...movie, behaviorHints: { videoSize: 123456789 } }, 'X').sizeBytes).toBe(123456789);
  });

  it('accepts direct links and rejects entries with nothing playable', () => {
    expect(parseStream({ url: 'https://example.com/a.mkv', title: 'A 720p' }, 'X')).toMatchObject({ kind: 'link', value: 'https://example.com/a.mkv' });
    expect(parseStream({ url: 'javascript:alert(1)', title: 'x' }, 'X')).toBeNull();
    expect(parseStream({ infoHash: 'nothex', title: 'x' }, 'X')).toBeNull();
    expect(parseStream(null, 'X')).toBeNull();
  });

  it('de-duplicates by magnet and file, keeping different episodes of one pack', () => {
    const list = parseStreams([
      { infoHash: HASH, fileIdx: 1, title: 'Pack' },
      { infoHash: HASH, fileIdx: 1, title: 'Pack again' },
      { infoHash: HASH, fileIdx: 2, title: 'Pack' },
      { title: 'junk' },
    ], 'X');
    expect(list.map(c => c.id)).toEqual([`magnet:${HASH}#1`, `magnet:${HASH}#2`]);
  });

  it('survives a non-array stream list', () => {
    expect(parseStreams(undefined, 'X')).toEqual([]);
  });
});
