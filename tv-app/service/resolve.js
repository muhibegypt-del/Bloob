// Resolution jobs for the native app: candidate (magnet or link) → playable
// URL through AllDebrid. Mirrors the web app's src/sources/resolver.ts, but runs
// in the service so the app only starts a job, polls it, answers a file or
// quality question, or cancels. The URL stays on this machine.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const crypto = require('crypto');
const { sizeText } = require('./rank');

const MAGNET_POLL_MS = 4000;
const MAGNET_TIMEOUT_MS = 10 * 60 * 1000;
const DELAYED_POLL_MS = 4000;
const DELAYED_TIMEOUT_MS = 4 * 60 * 1000;
const JOB_TTL_MS = 15 * 60 * 1000;

const VIDEO_FILE = /\.(mp4|mkv|m4v|mov|webm|ts|m2ts|avi)$/i;
const EXTRAS = /(^|[\s._\-/[(])(sample|trailer|teaser|extras?|featurettes?|behind[\s._-]the[\s._-]scenes|deleted[\s._-]scenes)([\s._\-)\]]|$)/i;

function videoFiles(nodes, result, folder) {
  result = result || [];
  folder = folder || '';
  for (const node of nodes || []) {
    if (node.e) videoFiles(node.e, result, folder ? `${folder}/${node.n}` : node.n);
    else if (node.l && VIDEO_FILE.test(node.n || '')) {
      result.push({ name: node.n, path: folder ? `${folder}/${node.n}` : node.n, size: node.s || 0, link: node.l });
    }
  }
  return result.sort((a, b) => b.size - a.size);
}

/** Same order as the web resolver: named file, only video, episode match, dominant largest. */
function pickFile(files, hint) {
  if (hint.fileName) {
    const wanted = String(hint.fileName).split('/').pop().toLowerCase();
    const exact = files.filter(f => f.name.toLowerCase() === wanted);
    if (exact.length === 1) return exact[0];
  }
  const clean = files.filter(f => !EXTRAS.test(f.path || f.name));
  const candidates = clean.length ? clean : files;
  if (candidates.length === 1) return candidates[0];
  if (hint.season !== undefined && hint.episode !== undefined) {
    const s = hint.season;
    const e = hint.episode;
    const pattern = new RegExp(`s0*${s}[\\s._-]*e0*${e}(?!\\d)|(?:^|\\D)${s}x0*${e}(?!\\d)`, 'i');
    const matches = candidates.filter(f => pattern.test(f.name));
    if (!matches.length) return null;
    return matches.sort((a, b) => b.size - a.size)[0];
  }
  const ranked = candidates.slice().sort((a, b) => b.size - a.size);
  return ranked[0].size > 0 && ranked[0].size >= ranked[1].size * 1.5 ? ranked[0] : null;
}

class Cancelled extends Error {
  constructor() { super('Cancelled'); this.cancelled = true; }
}

function createResolver(alldebrid) {
  const jobs = new Map();

  function sweep() {
    const now = Date.now();
    jobs.forEach((job, id) => { if (now - job.updated > JOB_TTL_MS) jobs.delete(id); });
  }

  function update(job, fields) {
    if (job.cancelled) throw new Cancelled();
    Object.assign(job.public, fields);
    job.updated = Date.now();
  }

  function wait(job, ms) {
    return new Promise((resolve, reject) => {
      if (job.cancelled) return reject(new Cancelled());
      const timer = setTimeout(() => { job.wake = null; resolve(); }, ms);
      job.wake = () => { clearTimeout(timer); reject(new Cancelled()); };
    });
  }

  function ready(job, url, fileName, size) {
    update(job, { state: 'ready', step: 'ready', label: 'Ready', fraction: 1, url, fileName: fileName || '', size: size || 0 });
  }

  async function finish(job, result, file) {
    if (result.streams && result.streams.length && !result.delayed && !result.link) {
      job.linkId = String(result.id);
      job.qualities = result.streams;
      return update(job, {
        state: 'choose-quality',
        label: 'Choose a quality',
        qualities: result.streams.map(s => ({ id: String(s.id), label: [s.quality ? `${s.quality}p` : '', s.ext, sizeText(s.filesize)].filter(Boolean).join(' · ') })),
      });
    }
    if (result.delayed) {
      update(job, { state: 'working', step: 'preparing', label: 'AllDebrid is preparing the stream…', fraction: null });
      const deadline = Date.now() + DELAYED_TIMEOUT_MS;
      for (;;) {
        await wait(job, DELAYED_POLL_MS);
        const next = await alldebrid('/v4/link/delayed', { id: String(result.delayed) });
        if (next.status === 2 && next.link) return ready(job, next.link, result.filename || (file && file.name), file && file.size);
        if (next.status === 3) throw new Error('AllDebrid could not prepare this link.');
        if (Date.now() > deadline) throw new Error('AllDebrid took too long to prepare this link.');
      }
    }
    if (result.link) return ready(job, result.link, result.filename || (file && file.name), result.filesize || (file && file.size));
    throw new Error('AllDebrid returned no playable link.');
  }

  async function unlock(job, link, file) {
    update(job, { state: 'working', step: 'unlocking', label: 'Getting the stream…', fraction: null });
    return finish(job, await alldebrid('/v4/link/unlock', { link }), file);
  }

  async function run(job, source, hint) {
    if (source.kind === 'link') return unlock(job, source.value, null);
    update(job, { state: 'working', step: 'adding', label: 'Sending to AllDebrid…', fraction: null });
    const upload = await alldebrid('/v4/magnet/upload', { 'magnets[]': source.value });
    const magnet = upload.magnets && upload.magnets[0];
    if (!magnet) throw new Error('AllDebrid did not accept this release.');
    if (magnet.error) throw new Error(magnet.error.message || magnet.error.code);
    if (!magnet.ready) {
      const deadline = Date.now() + MAGNET_TIMEOUT_MS;
      for (;;) {
        await wait(job, MAGNET_POLL_MS);
        const data = await alldebrid('/v4.1/magnet/status', { id: String(magnet.id) });
        const status = data.magnets && (Array.isArray(data.magnets) ? data.magnets[0] : data.magnets);
        if (!status) throw new Error('AllDebrid lost track of this release.');
        if (status.statusCode === 4) break;
        if (status.statusCode >= 5) throw new Error(`AllDebrid could not fetch this release (${status.status || 'failed'}). Try another source.`);
        update(job, {
          step: 'caching',
          label: status.size ? 'AllDebrid is downloading this release…' : (status.status || 'Waiting for AllDebrid…'),
          fraction: status.size ? Math.min(1, (status.downloaded || 0) / status.size) : null,
        });
        if (Date.now() > deadline) throw new Error('This release is still downloading on AllDebrid. Try again later or pick another source.');
      }
    }
    update(job, { step: 'files', label: 'Finding the video file…', fraction: null });
    const data = await alldebrid('/v4/magnet/files', { 'id[]': String(magnet.id) });
    const entry = data.magnets && data.magnets[0];
    const files = videoFiles(entry && entry.files);
    if (!files.length) throw new Error('This release contains no playable video file.');
    const file = pickFile(files, hint);
    if (file) return unlock(job, file.link, file);
    job.files = files;
    update(job, {
      state: 'choose-file',
      label: 'Choose the file to play',
      files: files.map((f, index) => ({ index, name: f.name, size: sizeText(f.size) })),
    });
  }

  function settle(job, promise) {
    promise.catch(error => {
      if (job.cancelled || error.cancelled) return;
      Object.assign(job.public, { state: 'error', label: error.message || 'Could not get this source.', error: error.message || 'Could not get this source.', status: error.status || 0 });
      job.updated = Date.now();
    });
  }

  function start(source, hint) {
    sweep();
    const id = crypto.randomBytes(8).toString('hex');
    const job = { id, cancelled: false, wake: null, updated: Date.now(), public: { id, state: 'working', step: 'starting', label: 'Starting…', fraction: null } };
    jobs.set(id, job);
    settle(job, run(job, source, hint || {}));
    return job.public;
  }

  function status(id) {
    const job = jobs.get(String(id));
    return job ? job.public : null;
  }

  function choose(id, answer) {
    const job = jobs.get(String(id));
    if (!job) return null;
    if (job.public.state === 'choose-file' && job.files && job.files[Number(answer.file)]) {
      const file = job.files[Number(answer.file)];
      update(job, { state: 'working', files: undefined });
      settle(job, unlock(job, file.link, file));
    } else if (job.public.state === 'choose-quality' && answer.quality !== undefined) {
      const linkId = job.linkId;
      update(job, { state: 'working', step: 'unlocking', label: 'Getting the stream…', qualities: undefined });
      settle(job, alldebrid('/v4/link/streaming', { id: linkId, stream: String(answer.quality) }).then(result => finish(job, result, null)));
    }
    return job.public;
  }

  function cancel(id) {
    const job = jobs.get(String(id));
    if (!job) return false;
    job.cancelled = true;
    if (job.wake) job.wake();
    jobs.delete(job.id);
    return true;
  }

  return { start, status, choose, cancel };
}

module.exports = { createResolver, pickFile, videoFiles };
