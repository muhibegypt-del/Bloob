// Online subtitles (4.0): OpenSubtitles through its public, keyless Stremio
// add-on (opensubtitles-v3.strem.io), for a film or episode by IMDb ID.
//   - The file's OpenSubtitles hash (its size and the sum of its first and
//     last 64 KB, read with two small range requests) and its name go with
//     the request, so subtitles made for this very file come back marked.
//   - A chosen subtitle is downloaded, read (SubRip or WebVTT) and lined up
//     (subsync.js) with a subtitle made for this file, when there is one in
//     any language; one made for this file is used as it is.
// The file's address stays in this process (never logged, never sent to
// OpenSubtitles); downloads are allowed only from the add-on's own hosts.
// Nothing here is sent the AllDebrid key.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const https = require('https');
const http = require('http');
const { get, getJson } = require('./net');
const subsync = require('./subsync');

const ADDON = 'https://opensubtitles-v3.strem.io';
const KEEP_MS = 6 * 60 * 60 * 1000;
const lists = new Map();   // key -> { at, items }
const entries = new Map(); // id -> entry (with its url), for /file
const files = new Map();   // id -> { at, cues }

// Bloob's language names and the ISO 639-2 codes OpenSubtitles uses.
const CODES = {
  English: ['eng'], French: ['fre', 'fra'], Spanish: ['spa', 'spn', 'spl'], German: ['ger', 'deu'], Italian: ['ita'],
  Portuguese: ['por', 'pob'], Hindi: ['hin'], Urdu: ['urd'], Arabic: ['ara'], Turkish: ['tur'], Japanese: ['jpn'], Korean: ['kor'],
};
const NAMES = {};
Object.keys(CODES).forEach(n => CODES[n].forEach(c => { NAMES[c] = n; }));

function languageName(code) { return NAMES[String(code || '').toLowerCase()] || String(code || '').toUpperCase(); }

// ---------------------------------------------------------------- the file's hash

/** Bytes `start`..`end` of `url` (a range request; one redirect followed). */
function range(url, start, end, redirects) {
  return new Promise((resolve, reject) => {
    let target;
    try { target = new URL(url); } catch (_) { return reject(new Error('Invalid URL')); }
    const client = target.protocol === 'https:' ? https : http;
    const req = client.get(target, { timeout: 10000, headers: { Range: `bytes=${start}-${end}`, 'User-Agent': 'Bloob/4.0 (webOS)' } }, res => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location && (redirects || 0) < 3) {
        res.resume();
        return resolve(range(new URL(res.headers.location, target).toString(), start, end, (redirects || 0) + 1));
      }
      if (res.statusCode !== 206 && res.statusCode !== 200) {
        res.resume();
        return reject(new Error(`range read returned ${res.statusCode}`));
      }
      const size = (() => {
        const m = /\/(\d+)$/.exec(String(res.headers['content-range'] || ''));
        return m ? Number(m[1]) : Number(res.headers['content-length'] || 0);
      })();
      const chunks = [];
      let got = 0;
      const want = end - start + 1;
      res.on('data', d => {
        chunks.push(d);
        got += d.length;
        if (got >= want) req.destroy();  // a server that ignored the range: enough is enough
      });
      const done = () => resolve({ body: Buffer.concat(chunks).slice(0, want), size, partial: res.statusCode === 206 });
      res.on('end', done);
      res.on('close', done);
      res.on('error', reject);
    });
    req.on('timeout', () => req.destroy(new Error('range read timed out')));
    req.on('error', e => (e && e.message === 'aborted' ? null : reject(e)));
  });
}

/** OpenSubtitles' hash: file size plus the 64-bit little-endian sum of the first and last 64 KB, as 16 hex digits. */
function hashOf(size, head, tail) {
  // Two 32-bit halves (no BigInt on older Node).
  let lo = size % 0x100000000, hi = Math.floor(size / 0x100000000) % 0x100000000;
  const add = buf => {
    for (let i = 0; i + 8 <= buf.length; i += 8) {
      lo += buf.readUInt32LE(i);
      hi += buf.readUInt32LE(i + 4) + Math.floor(lo / 0x100000000);
      lo %= 0x100000000;
      hi %= 0x100000000;
    }
  };
  add(head);
  add(tail);
  const hex = n => ('00000000' + n.toString(16)).slice(-8);
  return hex(hi) + hex(lo);
}

async function fileHash(url) {
  const CHUNK = 65536;
  const first = await range(url, 0, CHUNK - 1);
  if (!first.partial || !first.size || first.size < CHUNK * 2) return null;
  const last = await range(url, first.size - CHUNK, first.size - 1);
  if (last.body.length < CHUNK) return null;
  return { hash: hashOf(first.size, first.body, last.body), size: first.size };
}

// ---------------------------------------------------------------- the list

function key(type, id, hash) { return `${type}:${id}:${hash || ''}`; }

/**
 * Online subtitles for `type` ('movie' | 'series') and `id` (`tt…` or `tt…:S:E`).
 * `url`: the file's address (for its hash), `filename`: its name. Returns
 * { items: [{ id, language, code, forFile, release, downloads }] }, made-for-this-file first.
 */
async function list(type, id, url, filename) {
  if (!/^tt\d+(:\d+:\d+)?$/.test(String(id || ''))) return { items: [] };
  let file = null;
  if (url && /^https?:\/\//.test(url)) file = await fileHash(url).catch(() => null);
  const k = key(type, id, file && file.hash);
  const kept = lists.get(k);
  if (kept && Date.now() - kept.at < KEEP_MS) return { items: kept.items, hash: !!file };
  const extra = [];
  if (file) extra.push(`videoHash=${file.hash}`, `videoSize=${file.size}`);
  if (filename) extra.push(`filename=${encodeURIComponent(filename)}`);
  const path = `${ADDON}/subtitles/${type === 'series' ? 'series' : 'movie'}/${encodeURIComponent(id)}${extra.length ? '/' + extra.join('&') : ''}.json`;
  const j = await getJson(path, { timeout: 12000, maxBytes: 3000000 });
  const items = (j && Array.isArray(j.subtitles) ? j.subtitles : []).filter(s => s && s.url && /^https:\/\/[a-z0-9.-]*strem\.io\//.test(s.url))
    .map(s => {
      const entry = {
        id: String(s.id || s.url),
        code: String(s.lang || '').toLowerCase(),
        language: languageName(s.lang),
        forFile: s.m === 'h',
        release: String(s.movieReleaseName || s.subtitleFileName || ''),
        file: String(s.subtitleFileName || ''),
        downloads: Number(s.g) || 0,
        fps: Number(s.fpsMilli) ? Number(s.fpsMilli) / 1000 : 0,
        url: s.url,
      };
      entries.set(entry.id, entry);
      return entry;
    });
  items.sort((a, b) => (b.forFile - a.forFile) || (b.downloads - a.downloads));
  const out = items.map(e => ({ id: e.id, language: e.language, code: e.code, forFile: e.forFile, release: e.release, downloads: e.downloads }));
  lists.set(k, { at: Date.now(), items: out, all: items });
  if (lists.size > 100) lists.delete(lists.keys().next().value);
  return { items: out, hash: !!file };
}

// ---------------------------------------------------------------- reading a subtitle

function clock(s) {
  const m = /(?:(\d+):)?(\d{1,2}):(\d{2})[,.](\d{1,3})/.exec(s);
  if (!m) return -1;
  const ms = Number((m[4] + '00').slice(0, 3));
  return ((Number(m[1] || 0) * 60 + Number(m[2])) * 60 + Number(m[3])) * 1000 + ms;
}

function clean(text) {
  return text.replace(/\{\\[^}]*\}/g, '').replace(/<[^>]+>/g, '').replace(/&nbsp;/g, ' ').replace(/&amp;/g, '&')
    .replace(/&lt;/g, '<').replace(/&gt;/g, '>').split('\n').map(l => l.trim()).filter(Boolean).join('\n');
}

/** SubRip or WebVTT text to cues { s, e, t } (ms), in order. */
function parse(text) {
  const body = String(text || '').replace(/^﻿/, '').replace(/\r\n?/g, '\n');
  const out = [];
  body.split(/\n\s*\n/).forEach(block => {
    const lines = block.split('\n');
    const at = lines.findIndex(l => l.indexOf('-->') >= 0);
    if (at < 0) return;
    const parts = lines[at].split('-->');
    const s = clock(parts[0]), e = clock(parts[1] || '');
    const t = clean(lines.slice(at + 1).join('\n'));
    if (s >= 0 && e > s && t) out.push({ s, e, t });
  });
  out.sort((a, b) => a.s - b.s);
  return out;
}

async function cuesOf(entry) {
  const kept = files.get(entry.id);
  if (kept && Date.now() - kept.at < KEEP_MS) return kept.cues;
  const res = await get(entry.url, { timeout: 15000, maxBytes: 4000000, allow: h => /(^|\.)strem\.io$/.test(h) });
  const cues = parse(res.body.toString('utf8'));
  files.set(entry.id, { at: Date.now(), cues });
  if (files.size > 30) files.delete(files.keys().next().value);
  return cues;
}

/**
 * One subtitle's cues, lined up: { cues, sync: { how, ratio, shiftMs, score,
 * reference } }. `how`: 'for-file' (made for this file, used as it is),
 * 'aligned' (moved onto a reference), 'kept' (no reference, or no clearly better fit).
 */
async function file(id) {
  const entry = entries.get(String(id));
  if (!entry) throw new Error('That subtitle is no longer listed. Open the list again.');
  const cues = await cuesOf(entry);
  if (entry.forFile) return { cues, sync: { how: 'for-file', ratio: 1, shiftMs: 0 } };
  // A reference made for this file, the same language first.
  let refs = [];
  lists.forEach(l => (l.all || []).forEach(e => {
    if (e.forFile && e.id !== entry.id && refs.indexOf(e) < 0) refs.push(e);
  }));
  refs.sort((a, b) => (b.code === entry.code) - (a.code === entry.code) || b.downloads - a.downloads);
  refs = refs.slice(0, 2);
  for (let i = 0; i < refs.length; i++) {
    const ref = await cuesOf(refs[i]).catch(() => null);
    if (!ref || ref.length < 20) continue;
    const a = subsync.align(cues, ref);
    return {
      cues: subsync.apply(cues, a),
      sync: { how: a.changed ? 'aligned' : 'kept', ratio: a.ratio, shiftMs: a.changed ? a.shiftMs : 0, score: a.score, before: a.before, reference: refs[i].language },
    };
  }
  return { cues, sync: { how: 'kept', ratio: 1, shiftMs: 0 } };
}

module.exports = { list, file, parse, hashOf, languageName, CODES };
