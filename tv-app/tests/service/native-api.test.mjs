import { describe, expect, it } from 'vitest';
import rankModule from '../../service/rank.js';
import resolveModule from '../../service/resolve.js';

const { rank } = rankModule;
const { createResolver, pickFile, videoFiles } = resolveModule;

const candidate = (over) => ({
  id: over.id, provider: 'Test', kind: 'magnet', value: 'a'.repeat(40), release: over.id,
  claims: {}, languages: [], claimsFrom: 'release-name', ...over,
});

describe('service ranking', () => {
  it('recommends the best non-excluded source with readable labels', () => {
    const result = rank([
      candidate({ id: 'cam', claims: { resolution: '4K' }, sourceType: 'CAM', seeders: 900 }),
      candidate({ id: 'dv', claims: { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['TrueHD', 'Atmos'], channels: '7.1', container: 'MP4' }, sourceType: 'WEB-DL', seeders: 120, sizeBytes: 18.4e9 }),
      candidate({ id: 'hd', claims: { resolution: '1080p', audio: ['DD+'], channels: '5.1' }, seeders: 400, sizeBytes: 4.2e9 }),
    ], { language: 'English' });
    expect(result.recommended.id).toBe('dv');
    // TrueHD Atmos is not promised as Atmos until passthrough is verified on the TV.
    expect(result.recommended.headline).toBe('4K · Dolby Vision');
    expect(result.recommended.warnings).toContain('TrueHD Atmos may play as standard sound on this TV');
    expect(result.recommended.picture).toBe('Dolby Vision + HDR');
    expect(result.recommended.audio).toBe('TrueHD Atmos 7.1');
    expect(result.recommended.size).toBe('18 GB');
    expect(result.recommended.availability).toBe('120 seeders');
    expect(result.why).toMatch(/best picture/);
    // Versions likely to fail on this TV get their own section, last.
    expect(result.groups.map(g => g.label)).toEqual(['Other versions', 'May not play well on this TV']);
    expect(result.groups[1].items[0]).toMatchObject({ id: 'cam', excluded: true });
    expect(JSON.stringify(result)).not.toContain('a'.repeat(40));
  });

  const best = (list, prefs) => rank(list, Object.assign({ language: 'English' }, prefs)).recommended;
  const all = (result) => [result.recommended].concat(...result.groups.map(g => g.items)).filter(Boolean);
  const byId = (result, id) => all(result).find(v => v.id === id);

  it('says "1 seeder" and keeps counts out of the short caution', () => {
    const result = rank([
      candidate({ id: 'one', claims: { resolution: '1080p' }, seeders: 1 }),
      candidate({ id: 'many', claims: { resolution: '720p' }, seeders: 1234 }),
    ], {});
    expect(byId(result, 'one').availability).toBe('1 seeder');
    expect(byId(result, 'many').availability).toBe('1234 seeders');
    expect(byId(result, 'one').cautions).toEqual(['Few sources · slow to prepare']);
    expect(byId(result, 'one').warnings[0]).toBe('Few seeders; may take a long time to prepare');
    for (const view of all(result)) for (const caution of view.cautions) expect(caution).not.toMatch(/\d/);
  });

  it('never prefers DTS sound, which this TV cannot play, over Dolby at the same resolution', () => {
    const dts = candidate({ id: 'dts', claims: { resolution: '4K', hdr: ['DV'], audio: ['DTS-HD MA'], channels: '7.1' }, sourceType: 'WEB-DL', seeders: 2000, sizeBytes: 20e9 });
    const ddp = candidate({ id: 'ddp', claims: { resolution: '4K', audio: ['Dolby Digital Plus'], channels: '5.1' }, sourceType: 'WEB-DL', seeders: 6, sizeBytes: 15e9 });
    for (const variant of ['DTS', 'DTS-HD MA', 'DTS:X']) {
      const result = rank([Object.assign({}, dts, { claims: Object.assign({}, dts.claims, { audio: [variant] }) }), ddp], {});
      expect(result.recommended.id).toBe('ddp');
      const silent = byId(result, 'dts');
      expect(silent.score).toBeLessThan(result.recommended.score);
      expect(silent.warnings[0]).toBe('This TV can\'t play DTS sound; you may hear nothing');
      expect(silent.cautions[0]).toBe('No sound on this TV');
    }
    // Sound at a lower resolution beats silence at a higher one.
    expect(best([dts, candidate({ id: 'aac', claims: { resolution: '1080p', audio: ['AAC'] }, seeders: 40 })]).id).toBe('aac');
    // With nothing else on offer it is still offered, with its warning.
    expect(best([dts]).id).toBe('dts');
  });

  it('gives TrueHD Atmos no Atmos or lossless bonus until passthrough is verified', () => {
    const base = { sourceType: 'WEB-DL', seeders: 100, sizeBytes: 20e9 };
    const truehd = candidate(Object.assign({ id: 'truehd', claims: { resolution: '4K', audio: ['Atmos', 'TrueHD'], channels: '7.1' } }, base));
    const ddp = candidate(Object.assign({ id: 'ddp', claims: { resolution: '4K', audio: ['Atmos', 'Dolby Digital Plus'], channels: '5.1' } }, base));
    const result = rank([truehd, ddp], {});
    expect(result.recommended.id).toBe('ddp');
    expect(result.recommended.headline).toBe('4K · Atmos');
    expect(result.recommended.notes).toContain('Dolby Atmos');
    const other = byId(result, 'truehd');
    expect(other.score).toBeLessThan(result.recommended.score);
    expect(other.notes).not.toContain('Dolby Atmos');
    expect(other.notes).not.toContain('Lossless audio');
    expect(other.warnings).toContain('TrueHD Atmos may play as standard sound on this TV');
    // A profile that has verified passthrough restores the bonus.
    const verified = { id: 'test', dts: 'none', truehdPassthrough: 'yes', dolbyVisionProfile7: 'no', hdr10plus: false, ethernetMbps: 95, comfortableMbps: 40, warnMbps: 60 };
    expect(byId(rank([truehd, ddp], { profile: verified }), 'truehd').notes).toContain('Dolby Atmos');
  });

  it('scores disc Dolby Vision as HDR10 and keeps web Dolby Vision ahead', () => {
    const same = { seeders: 100 };
    const disc = candidate({ id: 'disc', release: 'Film.2024.2160p.UHD.BluRay.REMUX.DV.HDR10.HEVC.DDP5.1', claims: { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['Dolby Digital Plus'], channels: '5.1', container: 'MP4' }, sourceType: 'REMUX', seeders: same.seeders, sizeBytes: 30e9 });
    const encode = candidate({ id: 'encode', claims: { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['Dolby Digital Plus'], channels: '5.1', container: 'MP4' }, sourceType: 'BluRay', seeders: same.seeders, sizeBytes: 20e9 });
    const web = candidate({ id: 'web', claims: { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['Dolby Digital Plus'], channels: '5.1', container: 'MP4' }, sourceType: 'WEB-DL', seeders: same.seeders, sizeBytes: 20e9 });
    const result = rank([disc, encode, web], {});
    expect(['web', 'encode']).toContain(result.recommended.id);
    expect(result.recommended.picture).toBe('Dolby Vision + HDR');
    expect(result.recommended.headline).toBe('4K · Dolby Vision');
    // A Blu-ray encode with Dolby Vision is normally profile 8: it keeps Dolby Vision.
    expect(byId(result, 'encode').picture).toBe('Dolby Vision + HDR');
    for (const id of ['disc']) {
      const view = byId(result, id);
      expect(view.picture).toBe('HDR10 (disc Dolby Vision unsupported)');
      expect(view.headline).toBe('4K · HDR10');
      expect(view.score).toBeLessThan(result.recommended.score);
      expect(view.notes).not.toContain('Dolby Vision');
    }
    // A profile 8 hint keeps Dolby Vision.
    const p8 = candidate(Object.assign({}, disc, { id: 'p8', release: 'Film.2024.2160p.BluRay.REMUX.DV.P8.HDR10' }));
    expect(byId(rank([p8], {}), 'p8').picture).toBe('Dolby Vision + HDR');
  });

  it('labels and scores HDR10+ as HDR10 on this TV', () => {
    const plus = candidate({ id: 'plus', claims: { resolution: '4K', hdr: ['HDR10+'] }, seeders: 50 });
    const ten = candidate({ id: 'ten', claims: { resolution: '4K', hdr: ['HDR10'] }, seeders: 50 });
    const result = rank([plus, ten], {});
    expect(byId(result, 'plus')).toMatchObject({ picture: 'HDR10', headline: '4K · HDR10' });
    expect(byId(result, 'plus').score).toBe(byId(result, 'ten').score);
  });

  it('penalises very high estimated bitrates and warns about the wired connection', () => {
    const film = (id, gb) => candidate({ id, claims: { resolution: '1080p' }, sourceType: 'BluRay', seeders: 100, sizeBytes: gb * 1e9 });
    const result = rank([film('huge', 70), film('normal', 12)], { type: 'movie', runtimeMinutes: 120 });
    expect(result.recommended.id).toBe('normal');
    const huge = byId(result, 'huge');
    expect(huge.bitrateMbps).toBe(78);
    expect(huge.score).toBeLessThan(result.recommended.score);
    expect(huge.warnings).toContain('Very high bitrate; may buffer on the TV\'s wired connection (about 95 Mbps)');
    expect(huge.cautions).toContain('Very high bitrate');
    expect(huge.facts).toContain('about 78 Mbit/s');
    expect(result.recommended.warnings).toEqual([]);
    // Runtime matters: 30 GB is fine for a long film and too much for an episode.
    const thirty = film('thirty', 30);
    expect(byId(rank([thirty], { type: 'movie', runtimeMinutes: 160 }), 'thirty').cautions).toEqual([]);
    expect(byId(rank([thirty], { type: 'series' }), 'thirty').cautions).toEqual(['Very high bitrate']);
    // A REMUX of ordinary size is no longer penalised for being a REMUX.
    const remux = candidate({ id: 'remux', claims: { resolution: '1080p' }, sourceType: 'REMUX', seeders: 100, sizeBytes: 25e9 });
    expect(byId(rank([remux], { runtimeMinutes: 120 }), 'remux').cautions).toEqual([]);
  });

  it('lifts releases tagged with the preferred language', () => {
    const plain = candidate({ id: 'plain', claims: { resolution: '1080p' }, seeders: 100 });
    const english = candidate({ id: 'english', claims: { resolution: '1080p' }, seeders: 100, languages: ['English'] });
    const french = candidate({ id: 'french', claims: { resolution: '1080p' }, seeders: 100, languages: ['French'] });
    expect(best([plain, english]).id).toBe('english');
    expect(best([french, plain, english], { language: 'French' }).id).toBe('french');
    const result = rank([plain, english, french], { language: 'English' });
    expect(byId(result, 'english').score).toBeGreaterThan(byId(result, 'plain').score);
    expect(byId(result, 'plain').score).toBeGreaterThan(byId(result, 'french').score);
  });

  it('never recommends 3D versions, disc images or samples', () => {
    const good = candidate({ id: 'good', claims: { resolution: '720p' }, seeders: 10 });
    const result = rank([
      candidate({ id: '3d', claims: { resolution: '4K', threeD: true }, seeders: 500 }),
      candidate({ id: 'iso', claims: { resolution: '4K', discImage: true }, seeders: 500 }),
      candidate({ id: 'sample', claims: { resolution: '4K', sample: true }, seeders: 500 }),
      good,
    ], {});
    expect(result.recommended.id).toBe('good');
    expect(byId(result, '3d')).toMatchObject({ excluded: true, cautions: ['3D version'] });
    expect(byId(result, 'iso')).toMatchObject({ excluded: true, cautions: ['Disc image'] });
    expect(byId(result, 'sample')).toMatchObject({ excluded: true, cautions: ['Sample only'] });
    for (const id of ['3d', 'iso', 'sample']) expect(byId(result, id).warnings).toHaveLength(1);
  });

  it('pairs the first caution with the first warning', () => {
    const view = rank([candidate({ id: 'x', release: 'X.2020.2160p.BluRay.REMUX.DV.TrueHD.Atmos', claims: { resolution: '4K', hdr: ['DV'], audio: ['Atmos', 'TrueHD'] }, sourceType: 'REMUX', seeders: 2, sizeBytes: 80e9 })], { runtimeMinutes: 120 }).recommended;
    expect(view.cautions[0]).toBe('Very high bitrate');
    expect(view.warnings[0]).toMatch(/^Very high bitrate/);
    expect(view.cautions).toEqual(['Very high bitrate', 'Few sources · slow to prepare']);
  });
});

describe('service resolver', () => {
  const files = [
    { n: 'Show.S01E01.mkv', s: 2e9, l: 'https://host/1' },
    { n: 'Show.S01E02.mkv', s: 2.1e9, l: 'https://host/2' },
    { n: 'Extras', e: [{ n: 'sample.mkv', s: 1e7, l: 'https://host/s' }] },
  ];

  it('picks the episode file inside a pack', () => {
    expect(pickFile(videoFiles(files), { season: 1, episode: 2 }).link).toBe('https://host/2');
    expect(pickFile(videoFiles(files), {})).toBeNull();
  });

  function fakeDebrid(log) {
    let polls = 0;
    return async (endpoint, fields) => {
      log.push(endpoint);
      if (endpoint === '/v4/magnet/upload') return { magnets: [{ id: 7, ready: false }] };
      if (endpoint === '/v4.1/magnet/status') return { magnets: { statusCode: ++polls > 1 ? 4 : 1, status: 'Downloading', size: 100, downloaded: 50 } };
      if (endpoint === '/v4/magnet/files') return { magnets: [{ files }] };
      if (endpoint === '/v4/link/unlock') return { link: `${fields.link}?token=secret`, filename: 'Show.S01E02.mkv', filesize: 2.1e9 };
      throw new Error('unexpected ' + endpoint);
    };
  }

  async function until(resolver, id, predicate) {
    for (let i = 0; i < 400; i++) {
      const job = resolver.status(id);
      if (predicate(job)) return job;
      await new Promise(r => setTimeout(r, 25));
    }
    throw new Error('timeout');
  }

  it('runs a magnet job to a ready URL, reporting progress', async () => {
    const log = [];
    const resolver = createResolver(fakeDebrid(log));
    const job = resolver.start({ kind: 'magnet', value: 'a'.repeat(40) }, { season: 1, episode: 2 });
    const caching = await until(resolver, job.id, j => j.step === 'caching');
    expect(caching.fraction).toBe(0.5);
    const done = await until(resolver, job.id, j => j.state === 'ready');
    expect(done.url).toBe('https://host/2?token=secret');
    expect(log).toContain('/v4/magnet/files');
  }, 20000);

  it('asks for a file when none is clearly right, then continues', async () => {
    const resolver = createResolver(fakeDebrid([]));
    const job = resolver.start({ kind: 'magnet', value: 'a'.repeat(40) }, {});
    const question = await until(resolver, job.id, j => j.state === 'choose-file');
    expect(question.files.map(f => f.name)).toEqual(['Show.S01E02.mkv', 'Show.S01E01.mkv', 'sample.mkv']);
    resolver.choose(job.id, { file: 1 });
    const done = await until(resolver, job.id, j => j.state === 'ready');
    expect(done.url).toBe('https://host/1?token=secret');
  }, 20000);

  it('cancels while waiting', async () => {
    const resolver = createResolver(fakeDebrid([]));
    const job = resolver.start({ kind: 'magnet', value: 'a'.repeat(40) }, {});
    await until(resolver, job.id, j => j.step === 'adding' || j.step === 'caching' || j.step === 'starting');
    expect(resolver.cancel(job.id)).toBe(true);
    expect(resolver.status(job.id)).toBeNull();
  });
});

describe('service catalogue', () => {
  it('quietens shouting episode titles but keeps acronyms and mixed case', async () => {
    const { calmTitle } = (await import('../../service/catalog.js')).default;
    expect(calmTitle('THE ONE WHERE MONICA GETS A ROOMMATE')).toBe('The One Where Monica Gets a Roommate');
    expect(calmTitle('OUT OF THE PAST')).toBe('Out of the Past');
    expect(calmTitle('NCIS')).toBe('NCIS');
    expect(calmTitle('iCarly Saves TV')).toBe('iCarly Saves TV');
  });

  it('reads runtimes for the bitrate estimate and never fetches for one', async () => {
    const { runtimeMinutes, cachedRuntime } = (await import('../../service/catalog.js')).default;
    expect(runtimeMinutes('148 min')).toBe(148);
    expect(runtimeMinutes('2h 28min')).toBe(148);
    expect(runtimeMinutes('')).toBe(0);
    expect(await cachedRuntime('movie', 'tt0000001')).toBe(0);
  });
});

describe('service memory policy (R5b)', () => {
  it('trims the image byte cache to the app policy and reports it', async () => {
    const imagesModule = await import('../../service/images.js');
    const images = imagesModule.default || imagesModule;
    expect(images.setMemoryPolicy('playback')).toBe(12 * 1024 * 1024);
    expect(images.setMemoryPolicy('background')).toBe(4 * 1024 * 1024);
    expect(images.setMemoryPolicy('normal')).toBe(40 * 1024 * 1024);
    expect(images.setMemoryPolicy('anything else')).toBe(0);
    expect(typeof images.cacheBytes()).toBe('number');
  });
});
