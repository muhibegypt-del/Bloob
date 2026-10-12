// End-to-end check of the real app, local service, and Buna in Chromium,
// driven only by remote keys. Requires `npm run build` and ffmpeg.
//
// Chromium builds without proprietary codecs can't decode the bundled
// H.264/AAC clip, so the test serves a VP9/Opus transcode of the same clip in
// its place (with HTTP byte ranges). Everything else is the shipping code.
import { spawn, execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, mkdtempSync, readFileSync } from 'node:fs';
import http from 'node:http';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { chromium } from 'playwright-core';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const executablePath = process.env.CHROMIUM_PATH || '/opt/pw-browsers/chromium-1194/chrome-linux/chrome';
const port = 18000 + Math.floor(Math.random() * 1000);
const origin = `http://127.0.0.1:${port}`;
const cache = join(root, 'node_modules', '.cache', 'bloob-e2e');
const webm = join(cache, 'bbb.webm');
const results = [];
let failed = false;

function check(name, condition, detail = '') {
  results.push(`${condition ? 'PASS' : 'FAIL'}  ${name}${detail ? `  (${detail})` : ''}`);
  if (!condition) failed = true;
}

if (!existsSync(webm)) {
  mkdirSync(cache, { recursive: true });
  execFileSync('ffmpeg', ['-loglevel', 'error', '-y', '-i', join(root, 'service/www/media/big-buck-bunny-480p-30sec.mp4'),
    '-c:v', 'libvpx-vp9', '-b:v', '600k', '-g', '24', '-deadline', 'realtime', '-cpu-used', '8', '-c:a', 'libopus', webm]);
}
const clip = readFileSync(webm);

const www = mkdtempSync(join(tmpdir(), 'bloob-www-'));
execFileSync('node', [join(root, 'scripts/stage.mjs'), www], { stdio: 'ignore' });
const service = spawn('node', [join(root, 'service/server.js')], {
  env: { ...process.env, TV_APP_PORT: String(port), TV_APP_UI_DIR: www, TV_APP_DATA_DIR: mkdtempSync(join(tmpdir(), 'bloob-data-')) },
  stdio: 'inherit',
});

const browser = await chromium.launch({ executablePath });
try {
  for (let i = 0; i < 50; i++) {
    try { if ((await fetch(`${origin}/api/account`)).ok) break; } catch { /* starting */ }
    await new Promise(r => setTimeout(r, 100));
  }
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  // Offline and deterministic: no catalog, artwork, or indexer traffic.
  await page.route(/cinemeta|metahub|strem\.fun/, route => route.abort());
  await page.route(/\/api\/sources\/search$/, route => route.fulfill({ status: 502, contentType: 'application/json', body: JSON.stringify({ error: 'Indexer unavailable in test' }) }));
  await page.route(/\/media\/big-buck-bunny-480p-30sec\.mp4$/, route => {
    const range = /bytes=(\d+)-(\d*)/.exec(route.request().headers().range || '');
    const start = range ? Number(range[1]) : 0;
    const end = range && range[2] ? Math.min(Number(range[2]), clip.length - 1) : clip.length - 1;
    route.fulfill({
      status: range ? 206 : 200,
      headers: { 'Content-Type': 'video/webm', 'Accept-Ranges': 'bytes', 'Content-Length': String(end - start + 1), ...(range ? { 'Content-Range': `bytes ${start}-${end}/${clip.length}` } : {}) },
      body: clip.subarray(start, end + 1),
    });
  });

  const press = async (key, wait = 120) => { await page.keyboard.press(key); await page.waitForTimeout(wait); };
  // Keys Playwright has no name for (LG transport keys) go through the DevTools protocol.
  const cdp = await page.context().newCDPSession(page);
  const remoteKey = async (code, wait = 120) => {
    for (const type of ['rawKeyDown', 'keyUp']) await cdp.send('Input.dispatchKeyEvent', { type, windowsVirtualKeyCode: code, nativeVirtualKeyCode: code });
    await page.waitForTimeout(wait);
  };
  const state = () => page.evaluate(() => window.__buna.state());
  const until = async (description, predicate, timeout = 10000) => {
    const end = Date.now() + timeout;
    let last;
    while (Date.now() < end) {
      last = await state();
      if (last && predicate(last)) return last;
      await page.waitForTimeout(100);
    }
    throw new Error(`Timed out waiting for ${description}; last state ${JSON.stringify(last && { phase: last.phase, position: last.position, pending: last.pendingTarget, preview: last.previewTarget })}`);
  };

  // --- API hardening
  await page.goto(`${origin}/`);
  const csrf = await page.evaluate(() => fetch('/api/pin/start', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: '{}' }).then(r => r.status));
  check('service rejects non-JSON POSTs (CSRF guard)', csrf === 403, `status ${csrf}`);
  const rebinding = await new Promise(done => http.get({ host: '127.0.0.1', port, path: '/api/account', headers: { Host: 'attacker.example' } }, res => { res.resume(); done(res.statusCode); }).on('error', () => done('error')));
  check('service rejects foreign Host headers (DNS rebinding guard)', rebinding === 403, `status ${rebinding}`);

  // --- Home → Details → Sources → Player, by remote only
  await page.waitForSelector('.hero');
  await press('Enter', 600);
  await page.waitForSelector('.details-body');
  await press('Enter', 600);
  await page.waitForSelector('.sources-screen [data-key="test"].focused');
  const sourcesText = await page.textContent('.sources-screen');
  check('sources screen reports the indexer failure honestly', /Couldn't search for sources/.test(sourcesText || ''));
  await page.screenshot({ path: join(cache, 'sources-offline.png') });
  await press('Enter', 200);

  const started = await until('playback with a presented frame', s => s.phase === 'playing' && s.firstFrameMs !== null, 15000);
  check('playback starts', started.phase === 'playing');
  check('first frame is measured', typeof started.firstFrameMs === 'number' && started.firstFrameMs > 0, `${started.firstFrameMs} ms`);
  check('duration comes from media', started.durationSource === 'media' && Math.abs(started.duration - 30) < 1, `${started.duration}`);
  const advancing = await until('clock to advance', s => s.position > 1.5);
  check('clock follows media time', advancing.positionSource === 'media');
  await page.waitForTimeout(6500);
  check('HUD hides during uninterrupted playback', !(await page.$('.player-hud.visible')));
  // Restart near the beginning so a +20 s seek lands well before the 30 s clip ends.
  await page.evaluate(() => { document.querySelector('video').currentTime = 0; });
  await until('rewind', s => s.position < 1.5 && s.phase === 'playing');

  // --- seek preview and commit (two presses → +20s, one seek)
  const seekCount = () => page.evaluate(() => window.__buna.metrics().filter(m => m.name === 'seek').length);
  const seeksBefore = await seekCount();
  const before = (await state()).position;
  await press('ArrowRight', 60);
  await press('ArrowRight', 60);
  const preview = await state();
  check('seek preview accumulates without seeking', preview.previewTarget !== null && Math.abs(preview.previewTarget - (before + 20)) < 1.5 && preview.pendingTarget === null,
    `preview ${preview.previewTarget?.toFixed(1)} from ${before.toFixed(1)}`);
  const hudClass = await page.getAttribute('.player-hud', 'class');
  check('seeking from hidden uses the reduced HUD', /\bvisible\b/.test(hudClass) && /\bscrub\b/.test(hudClass), hudClass);
  const landed = await until('seek acknowledgement', s => s.previewTarget === null && s.pendingTarget === null && s.position >= preview.previewTarget - 1, 5000);
  const videoTime = await page.evaluate(() => document.querySelector('video').currentTime);
  check('displayed position matches the media clock after the seek', Math.abs(landed.position - videoTime) < 1, `state ${landed.position.toFixed(2)} video ${videoTime.toFixed(2)}`);
  const seekMetric = await page.evaluate(() => window.__buna.metrics().filter(m => m.name === 'seek').pop());
  check('one seek was sent for two presses', (await seekCount()) - seeksBefore === 1, `${(await seekCount()) - seeksBefore} seeks`);
  check('seek settle time is recorded', seekMetric && seekMetric.settleMs >= 0, `${seekMetric?.settleMs} ms, landed ${seekMetric?.landed}`);

  // --- pause / play through the HUD; icon follows state
  await press('ArrowDown', 200);
  await press('Enter', 300);
  const paused = await until('pause', s => s.phase === 'paused');
  const toggleLabel = await page.getAttribute('.player-controls button.primary', 'aria-label');
  check('pause works and the button offers Play', paused.intent === 'pause' && toggleLabel === 'Play', `label ${toggleLabel}`);
  const frozen = (await state()).position;
  await page.waitForTimeout(1200);
  check('paused clock is stable', Math.abs((await state()).position - frozen) < 0.01);
  check('HUD stays visible while paused', !!(await page.$('.player-hud.visible')));
  await remoteKey(415, 300); // LG remote Play
  await until('resume via the remote Play key', s => s.phase === 'playing');
  await remoteKey(19, 300); // LG remote Pause
  await until('pause via the remote Pause key', s => s.phase === 'paused');
  await remoteKey(415, 300);
  await until('resume', s => s.phase === 'playing');
  check('remote Play and Pause keys control playback', true);

  // --- audio menu explains capability honestly
  for (let i = 0; i < 6; i++) await press('ArrowRight', 60);
  const focusedLabel = await page.getAttribute('.player-controls button.focused', 'aria-label');
  await press('ArrowLeft', 60);
  const audioLabel = await page.getAttribute('.player-controls button.focused', 'aria-label');
  if (audioLabel === 'Audio') await press('Enter', 300);
  const menuText = await page.textContent('.player-menu').catch(() => '');
  check('audio menu opens from the controls', audioLabel === 'Audio' && !!menuText, `focused ${focusedLabel} → ${audioLabel}`);
  await page.screenshot({ path: join(cache, 'player-menu.png') });
  await press('Escape', 300);
  check('Back closes the menu first', !(await page.$('.player-menu')) && !!(await page.$('video')));

  // --- seek to the end
  await press('ArrowUp', 100);
  for (let i = 0; i < 4; i++) await press('ArrowRight', 40);
  await until('end of media', s => s.phase === 'ended', 15000);
  check('playback reaches the ended state', true);
  await page.screenshot({ path: join(cache, 'player-ended.png') });

  // --- exit releases the session and returns to Sources with focus intact
  await press('Escape', 600);
  const after = await page.evaluate(() => ({
    video: !!document.querySelector('video'),
    sources: !!document.querySelector('.screen:not(.hidden) .sources-screen'),
    focused: document.querySelector('.sources-screen .focused')?.getAttribute('data-key'),
    session: window.__buna.state(),
    disposed: window.__buna.metrics().some(m => m.name === 'dispose'),
  }));
  check('Back from the ended player returns to Sources', after.sources && !after.video, JSON.stringify({ sources: after.sources, video: after.video }));
  check('Sources focus is preserved', after.focused === 'test', `focused ${after.focused}`);
  check('session disposed with no live session left', after.disposed && after.session === null);
  check('no uncaught page errors', errors.length === 0, errors.join(' | '));

  // --- a second play starts a clean session
  await press('Enter', 200);
  const again = await until('second playback', s => s.phase === 'playing', 15000);
  check('second session plays from the start', again.position < 5 && again.resumedFrom === null, `position ${again.position.toFixed(1)}`);
  await press('Escape', 200);
  await press('Escape', 600);
  check('Back twice exits from playing', !(await page.$('video')));
} catch (error) {
  check('e2e run completed', false, error.message);
} finally {
  await browser.close();
  service.kill();
}

console.log(results.join('\n'));
console.log(failed ? '\nE2E FAILED' : `\nE2E PASSED (${results.length} checks). Screenshots in ${cache}`);
process.exit(failed ? 1 : 0);
