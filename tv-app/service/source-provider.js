// Source discovery adapter. The HTTP service uses this interface rather than
// knowing an indexer's URL or response format. Keep compatible with TV Node.
const https = require('https');
const { parseStreams } = require('./release');

function createTorrentioProvider(client = https) {
  return {
    name: 'Torrentio',
    search(type, id) {
      return new Promise((resolve, reject) => {
        const request = client.get({
          hostname: 'torrentio.strem.fun',
          path: `/stream/${type}/${encodeURIComponent(id).replace(/%3A/g, ':')}.json`,
          timeout: 15000,
          headers: { Accept: 'application/json', 'User-Agent': 'Bloob/0.2 (webOS)' },
        }, response => {
          if (response.statusCode !== 200) {
            response.resume();
            return reject(new Error(`Source indexer returned ${response.statusCode}`));
          }
          let body = '';
          response.on('data', chunk => {
            body += chunk;
            if (body.length > 5000000) request.destroy(new Error('Source list is too large'));
          });
          response.on('end', () => {
            try { resolve(parseStreams(JSON.parse(body).streams, 'Torrentio')); }
            catch (_) { reject(new Error('Source indexer returned an unreadable response')); }
          });
        });
        request.on('timeout', () => request.destroy(new Error('Source search timed out')));
        request.on('error', reject);
      });
    },
  };
}

module.exports = { createTorrentioProvider };
