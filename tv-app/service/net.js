// Small HTTPS helpers for the service: JSON and binary GETs with a timeout,
// a size cap and a bounded number of redirects to allowed hosts.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const https = require('https');
const http = require('http');

const USER_AGENT = 'Bloob/0.4 (webOS)';

function get(url, options) {
  const opts = Object.assign({ timeout: 15000, maxBytes: 5000000, redirects: 3, allow: null, accept: '*/*' }, options || {});
  return new Promise((resolve, reject) => {
    let target;
    try { target = new URL(url); } catch (_) { return reject(new Error('Invalid URL')); }
    if (target.protocol !== 'https:' && target.protocol !== 'http:') return reject(new Error('Unsupported URL'));
    if (opts.allow && !opts.allow(target.hostname)) return reject(new Error('Host not allowed'));
    const client = target.protocol === 'https:' ? https : http;
    const request = client.get(target, { timeout: opts.timeout, headers: { Accept: opts.accept, 'User-Agent': USER_AGENT } }, response => {
      const status = response.statusCode;
      if (status >= 300 && status < 400 && response.headers.location) {
        response.resume();
        if (opts.redirects <= 0) return reject(new Error('Too many redirects'));
        const next = new URL(response.headers.location, target).toString();
        return resolve(get(next, Object.assign({}, opts, { redirects: opts.redirects - 1 })));
      }
      if (status !== 200) {
        response.resume();
        const error = new Error(`${target.hostname} returned ${status}`);
        error.statusCode = status;
        return reject(error);
      }
      const chunks = [];
      let size = 0;
      response.on('data', chunk => {
        size += chunk.length;
        if (size > opts.maxBytes) request.destroy(new Error('Response is too large'));
        else chunks.push(chunk);
      });
      response.on('end', () => resolve({ body: Buffer.concat(chunks), type: String(response.headers['content-type'] || '') }));
      response.on('error', reject);
    });
    request.on('timeout', () => request.destroy(new Error(`${target.hostname} timed out`)));
    request.on('error', reject);
  });
}

async function getJson(url, options) {
  const result = await get(url, Object.assign({ accept: 'application/json' }, options || {}));
  try { return JSON.parse(result.body.toString('utf8')); }
  catch (_) { throw new Error('Unreadable response'); }
}

module.exports = { get, getJson };
