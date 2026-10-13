// Which app this copy of the service belongs to. The web app and the native app
// each bundle this service under their own ID and port; `bloob.json`, written
// next to this file when the native package is staged, says which one it is.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const fs = require('fs');
const path = require('path');

const defaults = {
  appId: 'com.muhib.tvprototype',
  serviceId: 'com.muhib.tvprototype.service',
  port: 8787,
  // Other Bloob apps whose saved AllDebrid pairing this copy may reuse.
  sharedAppIds: [],
};

function load() {
  let file = {};
  try { file = JSON.parse(fs.readFileSync(path.join(__dirname, 'bloob.json'), 'utf8')); }
  catch (_) { /* web app package: defaults */ }
  const config = Object.assign({}, defaults, file);
  if (process.env.TV_APP_PORT) config.port = Number(process.env.TV_APP_PORT);
  return config;
}

module.exports = load();
