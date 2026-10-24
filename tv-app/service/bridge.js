// The audio bridge (3.0). This TV's player decodes neither DTS nor TrueHD, so a version whose sound
// is only those plays through the bridge: the service runs ffmpeg (Bloob's own
// minimal build, bin/ffmpeg), which copies the video and subtitles untouched and
// turns the DTS track into Dolby Digital Plus 5.1, written as Matroska to the
// player over localhost. Seeking restarts the conversion at the new time
// (?t=seconds); the app adds that time to the player's clock.
//
// ffmpeg reads the file through /api/bridge/src/<token> on this service, which
// forwards byte ranges to AllDebrid: the resolved link (a bearer-like secret)
// stays in this process's memory and never reaches ffmpeg's arguments or a log.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { spawn } = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const http = require('http');
const https = require('https');
const path = require('path');

const TTL_MS = 12 * 60 * 60 * 1000;
const sessions = new Map();  // token -> { url, at, probe, speed }

function binary(name) {
  const own = path.join(__dirname, 'bin', name);
  if (process.env.BLOOB_FFMPEG_DIR) return path.join(process.env.BLOOB_FFMPEG_DIR, name);
  try { fs.accessSync(own, fs.constants.X_OK); return own; } catch (_) { return name; }  // desktop: from PATH
}

function prune() {
  const now = Date.now();
  sessions.forEach((s, k) => { if (now - s.at > TTL_MS) sessions.delete(k); });
}

/** Byte-range proxy from ffmpeg (localhost) to the resolved link. */
function serveSource(token, req, res) {
  const s = sessions.get(token);
  if (!s) { res.writeHead(404); return res.end(); }
  let target;
  try { target = new URL(s.url); } catch (_) { res.writeHead(400); return res.end(); }
  const lib = target.protocol === 'https:' ? https : http;
  const headers = {};
  if (req.headers.range) headers.Range = req.headers.range;
  const upstream = lib.request(target, { method: req.method === 'HEAD' ? 'HEAD' : 'GET', headers, timeout: 30000 }, up => {
    if (up.statusCode >= 300 && up.statusCode < 400 && up.headers.location) {
      // Follow a redirect once, keeping the new link in memory only.
      up.resume();
      s.url = new URL(up.headers.location, target).toString();
      return serveSource(token, req, res);
    }
    const pass = {};
    ['content-length', 'content-range', 'accept-ranges', 'content-type'].forEach(h => { if (up.headers[h]) pass[h] = up.headers[h]; });
    res.writeHead(up.statusCode, pass);
    up.pipe(res);
  });
  upstream.on('timeout', () => upstream.destroy(new Error('timeout')));
  upstream.on('error', () => { if (!res.headersSent) res.writeHead(502); res.end(); });
  req.on('close', () => upstream.destroy());
  upstream.end();
}

function srcUrl(port, token) {
  return `http://127.0.0.1:${port}/api/bridge/src/${token}`;
}

// Sound the TV's player can't decode: DTS, and TrueHD (TV apps can't decode or pass it through).
const DTS = /^(dts|dca|truehd|mlp)/i;

/** ffprobe's view of the file: duration, audio tracks, chapters, picture. */
function probe(port, token) {
  return new Promise(resolve => {
    const args = ['-v', 'error', '-show_entries',
      'format=duration:stream=index,codec_type,codec_name,profile,channels,width,height,disposition:stream_tags=language,title:stream_side_data=dv_profile:chapter=start_time,end_time:chapter_tags=title',
      '-of', 'json', srcUrl(port, token)];
    let out = '';
    let child;
    try { child = spawn(binary('ffprobe'), args, { stdio: ['ignore', 'pipe', 'ignore'] }); } catch (_) { return resolve(null); }
    const timer = setTimeout(() => child.kill('SIGKILL'), 20000);
    child.stdout.on('data', d => { out += d; });
    child.on('error', () => { clearTimeout(timer); resolve(null); });
    child.on('close', () => {
      clearTimeout(timer);
      try { resolve(summarise(JSON.parse(out))); } catch (_) { resolve(null); }
    });
  });
}

function summarise(j) {
  const streams = j.streams || [];
  let audioN = 0, textN = 0;
  const audio = [], text = [];
  let video = null;
  streams.forEach(s => {
    const tags = s.tags || {};
    const disp = s.disposition || {};
    if (s.codec_type === 'audio') {
      audio.push({ index: audioN++, codec: s.codec_name || '', profile: s.profile || '', channels: s.channels || 0,
        language: tags.language || '', title: tags.title || '', default: !!disp.default, commentary: !!disp.comment || /commentary/i.test(tags.title || '') });
    } else if (s.codec_type === 'subtitle') {
      text.push({ index: textN++, codec: s.codec_name || '', language: tags.language || '', title: tags.title || '', forced: !!disp.forced, sdh: !!disp.hearing_impaired || /sdh/i.test(tags.title || '') });
    } else if (s.codec_type === 'video' && !video && !(disp.attached_pic)) {
      const dv = (s.side_data_list || []).find(d => d.dv_profile !== undefined);
      video = { codec: s.codec_name || '', width: s.width || 0, height: s.height || 0, dvProfile: dv ? dv.dv_profile : null };
    }
  });
  const chapters = (j.chapters || []).map(c => ({ start: Number(c.start_time) || 0, end: Number(c.end_time) || 0, title: (c.tags || {}).title || '' }));
  return { durationMs: Math.round((Number((j.format || {}).duration) || 0) * 1000), audio, text, video, chapters };
}

/**
 * Which track the bridge converts, or -1 when none is needed: a main track this
 * TV can play exists (the player picks it, 2.8.1). Else the first DTS main
 * track in the viewer's language, else the first DTS main track.
 */
function bridgeTrack(info, language) {
  if (!info || !info.audio.length) return -1;
  const main = info.audio.filter(a => !a.commentary);
  if (main.some(a => !DTS.test(a.codec))) return -1;
  const lang = String(language || '').toLowerCase().slice(0, 2);
  const mine = main.find(a => DTS.test(a.codec) && lang && String(a.language).toLowerCase().indexOf(lang) === 0);
  const pick = mine || main.find(a => DTS.test(a.codec));
  return pick ? pick.index : -1;
}

function describe(track) {
  const name = /^(truehd|mlp)/i.test(track.codec || '') ? 'TrueHD'
    : /ma/i.test(track.profile || '') ? 'DTS-HD MA' : /x/i.test(track.profile || '') ? 'DTS:X' : 'DTS';
  const ch = track.channels === 8 ? '7.1' : track.channels === 6 ? '5.1' : track.channels === 2 ? 'Stereo' : `${track.channels} ch`;
  return `${name} ${ch} → Dolby Digital Plus 5.1 (Bloob)`;
}

/** Called by the app after resolving: decide, and if needed, hand back the bridge's stream. */
async function open(port, url, language) {
  prune();
  const token = crypto.randomBytes(12).toString('hex');
  sessions.set(token, { url: String(url), at: Date.now(), probe: null, speed: null });
  const info = await probe(port, token);
  const s = sessions.get(token);
  s.probe = info;
  const track = bridgeTrack(info, language);
  if (track < 0) {
    return { bridge: false, probe: info };
  }
  const t = info.audio.find(a => a.index === track);
  return {
    bridge: true,
    play: `http://127.0.0.1:${port}/api/bridge/play/${token}.mkv?a=${track}`,
    durationMs: info.durationMs,
    note: describe(t),
    probe: info,
  };
}

/** The converted stream, from ?t= seconds. One ffmpeg per request; it stops when the player hangs up. */
function servePlay(port, token, query, req, res) {
  const s = sessions.get(token);
  if (!s) { res.writeHead(404); return res.end(); }
  const t = Math.max(0, Number(query.get('t')) || 0);
  const a = Math.max(0, Math.floor(Number(query.get('a')) || 0));
  const args = ['-hide_banner', '-loglevel', 'error', '-stats_period', '2', '-stats',
    '-ss', String(t), '-i', srcUrl(port, token),
    '-map', '0:v:0', '-map', `0:a:${a}`, '-map', '0:s?',
    '-c:v', 'copy', '-c:s', 'copy', '-c:a', 'eac3', '-ac', '6', '-b:a', '1024k',
    '-map_metadata', '-1', '-map_chapters', '-1',
    '-f', 'matroska', '-live', '1', 'pipe:1'];
  let child;
  try { child = spawn(binary('ffmpeg'), args, { stdio: ['ignore', 'pipe', 'pipe'] }); } catch (e) {
    res.writeHead(500); return res.end();
  }
  res.writeHead(200, { 'Content-Type': 'video/x-matroska', 'Cache-Control': 'no-store' });
  child.stdout.pipe(res);
  let tail = '';
  child.stderr.on('data', d => {
    tail = (tail + d).slice(-400);
    const m = /speed=\s*([\d.]+)x/.exec(tail);
    if (m) s.speed = Number(m[1]);
  });
  const stop = () => { if (child.exitCode === null) child.kill('SIGKILL'); };
  req.on('close', stop);
  res.on('close', stop);
  child.on('close', code => {
    if (code && code !== 255) console.log(`[bridge] ffmpeg ended (${code}) at ${t}s: ${tail.split('\n').filter(l => !/^frame=|speed=/.test(l)).slice(-2).join(' ').slice(0, 200)}`);
    res.end();
  });
  console.log(`[bridge] converting audio track ${a} from ${t}s`);
}

function status(token) {
  const s = sessions.get(token);
  return s ? { speed: s.speed } : null;
}

module.exports = { open, serveSource, servePlay, status, bridgeTrack, summarise, describe };
