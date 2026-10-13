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
      candidate({ id: 'dv', claims: { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['TrueHD', 'Atmos'], channels: '7.1' }, sourceType: 'WEB-DL', seeders: 120, sizeBytes: 18.4e9 }),
      candidate({ id: 'hd', claims: { resolution: '1080p', audio: ['DD+'], channels: '5.1' }, seeders: 400, sizeBytes: 4.2e9 }),
    ], { language: 'English' });
    expect(result.recommended.id).toBe('dv');
    expect(result.recommended.headline).toBe('4K · Dolby Vision · Atmos');
    expect(result.recommended.picture).toBe('Dolby Vision + HDR');
    expect(result.recommended.audio).toBe('TrueHD Atmos 7.1');
    expect(result.recommended.size).toBe('18 GB');
    expect(result.recommended.availability).toBe('120 seeders');
    expect(result.why).toMatch(/best picture/);
    expect(result.groups.map(g => g.label)).toEqual(['4K', '1080p']);
    expect(result.groups[0].items[0]).toMatchObject({ id: 'cam', excluded: true });
    expect(JSON.stringify(result)).not.toContain('a'.repeat(40));
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
});
