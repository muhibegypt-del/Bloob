'use strict';
// Headphones mode's measured responses (3.2.1): WAV files the owner puts in the
// service's data folder (`headphones/` inside it, or `headphones.wav` beside it).
// Each usable one becomes a Surround room choice in the app, named after its
// file ("dolby-atmos.wav" -> "Dolby Atmos"). The app fetches the bytes through
// here, so it never needs to read the data folder itself. The files stay on the
// TV: third-party captures never go into the repository or the app package.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const fs = require('fs');
const path = require('path');
const accountStore = require('./account-store');

const MAX_BYTES = 8 * 1024 * 1024;

function dataDir() {
  const first = accountStore.defaultLocations().filter(l => !l.readOnly)[0];
  return first ? first.dir : null;
}

// Where responses are looked for, in order.
function folders() {
  const dir = dataDir();
  return dir ? [path.join(dir, 'headphones'), dir] : [];
}

// "dolby-atmos" -> "Dolby Atmos"; "headphones" (the 3.2.0 file name) -> "Your response".
function nameOf(base) {
  if (base.toLowerCase() === 'headphones') return 'Your response';
  return base.replace(/[-_]+/g, ' ').replace(/\s+/g, ' ').trim()
    .replace(/\b\w/g, c => c.toUpperCase());
}

// What the app can use: a RIFF WAVE at 48 kHz with 14 (HeSuVi) or 16 (Bloob) channels.
function inspect(buffer) {
  if (buffer.length < 44 || buffer.toString('ascii', 0, 4) !== 'RIFF' || buffer.toString('ascii', 8, 12) !== 'WAVE') return { why: 'not a WAV file' };
  let pos = 12;
  while (pos + 8 <= buffer.length) {
    const id = buffer.toString('ascii', pos, pos + 4);
    const len = buffer.readUInt32LE(pos + 4);
    if (id === 'fmt ') {
      const channels = buffer.readUInt16LE(pos + 10);
      const rate = buffer.readUInt32LE(pos + 12);
      if (rate !== 48000) return { channels: channels, rate: rate, why: 'needs 48 kHz (this one is ' + rate + ' Hz)' };
      if (channels !== 14 && channels !== 16) return { channels: channels, rate: rate, why: 'needs 14 or 16 channels, not ' + channels };
      return { channels: channels, rate: rate };
    }
    pos += 8 + len + (len & 1);
  }
  return { why: 'no format chunk' };
}

/** The responses found, usable or not (with the reason). */
function list() {
  const seen = {};
  const out = [];
  folders().forEach(folder => {
    let names = [];
    try { names = fs.readdirSync(folder); } catch (e) { return; }
    names.filter(n => /\.wav$/i.test(n)).sort().forEach(file => {
      // Beside the data folder's other files, only the 3.2.0 name counts.
      if (folder === dataDir() && file.toLowerCase() !== 'headphones.wav') return;
      const id = file;
      if (seen[id]) return;
      seen[id] = true;
      const full = path.join(folder, file);
      let stat;
      try { stat = fs.statSync(full); } catch (e) { return; }
      const entry = { id: id, name: nameOf(file.replace(/\.wav$/i, '')), bytes: stat.size };
      if (stat.size > MAX_BYTES) entry.why = 'larger than 8 MB';
      else {
        let head = Buffer.alloc(0);
        try {
          const fd = fs.openSync(full, 'r');
          head = Buffer.alloc(Math.min(stat.size, 4096));
          fs.readSync(fd, head, 0, head.length, 0);
          fs.closeSync(fd);
        } catch (e) { entry.why = 'unreadable'; }
        if (!entry.why) Object.assign(entry, inspect(head));
      }
      entry.usable = !entry.why;
      entry.path = full;
      out.push(entry);
    });
  });
  return out;
}

/** The bytes of one usable response, by id (its file name), or null. */
function read(id) {
  const entry = list().filter(e => e.id === String(id || '') && e.usable)[0];
  if (!entry) return null;
  try { return fs.readFileSync(entry.path); } catch (e) { return null; }
}

/** For the app: no paths. */
function publicList() {
  return list().map(e => {
    const out = { id: e.id, name: e.name, usable: e.usable };
    if (e.why) out.why = e.why;
    if (e.channels) out.channels = e.channels;
    return out;
  });
}

module.exports = { list, publicList, read, inspect, nameOf, folders };
