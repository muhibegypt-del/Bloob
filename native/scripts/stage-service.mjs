// Stage the Bloob service (tv-app/service) as the native app's own webOS JS
// service, com.muhib.bloob.service, on its own port. The web app keeps its copy.
//   node scripts/stage-service.mjs <out-dir> <version>
import { cpSync, mkdirSync, readdirSync, rmSync, writeFileSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';

const here = dirname(fileURLToPath(import.meta.url));
const source = join(here, '../../tv-app/service');
const [out, version] = process.argv.slice(2);
if (!out || !version) throw new Error('usage: stage-service.mjs <out-dir> <version>');

const appId = 'com.muhib.bloob';
const serviceId = `${appId}.service`;
rmSync(out, { recursive: true, force: true });
mkdirSync(out, { recursive: true });
// Code only: no web UI bundle, no saved data, no tests.
for (const name of readdirSync(source)) {
  if (name.endsWith('.js')) cpSync(join(source, name), join(out, name));
}
writeFileSync(join(out, 'package.json'), JSON.stringify({ name: serviceId, version, description: 'Bloob service', main: 'server.js' }, null, 2));
writeFileSync(join(out, 'services.json'), JSON.stringify({ id: serviceId, description: 'Bloob service', services: [{ name: serviceId }] }, null, 2));
// Its own port so it can run beside the web app's service; reuse the web app's pairing.
writeFileSync(join(out, 'bloob.json'), JSON.stringify({ appId, serviceId, port: 8790, sharedAppIds: ['com.muhib.tvprototype'] }, null, 2));
console.log(`staged ${serviceId} ${version} in ${out}`);
