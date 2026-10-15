// Development fixtures for the service (BLOOB_DEV_FIXTURES), used only by the
// desktop UI run: realistic indexer results and a fake AllDebrid that "resolves"
// to the bundled test clip. Never shipped.
const PORT = Number(process.env.TV_APP_PORT || 8790);

function stream(title, hash, size, seeders, extra) {
  return Object.assign({
    name: 'Torrentio\n' + (extra && extra.res || '4k'),
    title: `${title}\n👤 ${seeders} 💾 ${size} ⚙️ Fixture`,
    infoHash: hash.repeat(40).slice(0, 40),
    behaviorHints: { filename: title.split('\n')[0] + '.mkv' },
  }, extra || {});
}

async function streams(type, id) {
  if (type === 'series') return episodeStreams(id);
  return [
    stream('Film.2024.2160p.WEB-DL.DV.HDR10.DDP5.1.Atmos.H265', 'a', '18.4 GB', 245),
    stream('Film.2024.2160p.UHD.BluRay.REMUX.HDR10.TrueHD.7.1.Atmos', 'b', '61 GB', 40),
    stream('Film.2024.2160p.WEB.HDR.DTS-HD.MA.5.1.x265', 'c', '14 GB', 3),
    stream('Film.2024.1080p.WEB-DL.AAC2.0.H264', 'd', '1.9 GB', 1200, { name: 'Torrentio\n1080p' }),
    stream('Film.2024.1080p.BluRay.DTS.5.1.x264', 'e', '9.8 GB', 410, { name: 'Torrentio\n1080p' }),
    stream('Film.2024.1080p.WEB-DL.DDP5.1.ITA.ENG.H264 🇮🇹 / 🇬🇧', 'f', '4.2 GB', 60, { name: 'Torrentio\n1080p' }),
    stream('Film.2024.720p.WEB-DL.AAC2.0', '1', '1.1 GB', 300, { name: 'Torrentio\n720p' }),
    stream('Film.2024.HDCAM.x264', '2', '1.4 GB', 900, { name: 'Torrentio\nCAM' }),
  ];
}

// One episode as indexers list it: single files from several groups (a new
// hash per episode) and the whole season as one pack (the same hash throughout).
function episodeStreams(id) {
  const parts = String(id).split(':');
  const pad = (n) => String(n).padStart(2, '0');
  const code = `S${pad(parts[1] || 1)}E${pad(parts[2] || 1)}`;
  const e = Number(parts[2] || 1);
  const own = (c) => c + (e % 10);  // a different file (hash) each episode
  return [
    stream(`Show.${code}.2160p.WEB-DL.DV.HDR10.DDP5.1.Atmos.H265-FLUX`, own('a'), '6.1 GB', 140),
    stream(`Show.S${pad(parts[1] || 1)}.2160p.WEB-DL.DV.DDP5.1.Atmos.H265-NTb`, '3', '58 GB', 35,
      { fileIdx: e - 1, title: `Show.S${pad(parts[1] || 1)}.2160p.WEB-DL.DV.DDP5.1.Atmos.H265-NTb\nShow.${code}.2160p.WEB-DL.DV.DDP5.1.Atmos.H265-NTb.mkv\n👤 35 💾 5.8 GB ⚙️ Fixture` }),
    stream(`Show.${code}.2160p.WEB.HDR.DTS-HD.MA.5.1.x265-GRP`, own('c'), '7 GB', 4),
    stream(`Show.${code}.1080p.WEB-DL.DDP5.1.H264-FLUX`, own('d'), '2.2 GB', 2400, { name: 'Torrentio\n1080p' }),
    stream(`Show.${code}.720p.WEB-DL.AAC2.0-GRP`, own('e'), '0.9 GB', 600, { name: 'Torrentio\n720p' }),
  ];
}

let polls = 0;
async function alldebrid(endpoint, fields) {
  await new Promise(r => setTimeout(r, 150));
  if (endpoint === '/v4.1/pin/get') return { pin: 'K7QF', check: 'x', user_url: 'https://alldebrid.com/pin/?pin=K7QF', expires_in: 600 };
  if (endpoint === '/v4/pin/check') return { activated: false, expires_in: 590 };
  if (endpoint === '/v4/magnet/upload') { polls = 0; return { magnets: [{ id: 1, ready: false }] }; }
  if (endpoint === '/v4.1/magnet/status') return { magnets: { statusCode: ++polls > 2 ? 4 : 1, status: 'Downloading', size: 100, downloaded: 40 * polls } };
  if (endpoint === '/v4/magnet/files') return { magnets: [{ files: [{ n: 'Film.2024.2160p.mkv', s: 18e9, l: 'https://fixture/file' }, { n: 'Sample.mkv', s: 1e7, l: 'https://fixture/sample' }] }] };
  if (endpoint === '/v4/link/unlock') return { link: `http://127.0.0.1:${PORT}/media/big-buck-bunny-480p-30sec.mp4`, filename: 'Film.2024.2160p.mkv', filesize: 18e9 };
  throw new Error('Fixture has no answer for ' + endpoint);
}

module.exports = { streams, alldebrid };
