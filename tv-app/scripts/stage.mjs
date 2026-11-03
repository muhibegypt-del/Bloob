// Copy the built UI (dist/) into the service's web root for packaging.
// Replaces previously staged UI files so stale bundles never ship; leaves the
// bundled media untouched. Usage: node scripts/stage.mjs [destination]
import { cpSync, existsSync, mkdirSync, readdirSync, rmSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const dist = join(root, 'dist');
const destination = resolve(process.argv[2] || join(root, 'service', 'www'));

if (!existsSync(join(dist, 'index.html'))) {
  console.error('dist/ is missing. Run `npm run build` first.');
  process.exit(1);
}
mkdirSync(destination, { recursive: true });
for (const entry of ['index.html', 'assets']) rmSync(join(destination, entry), { recursive: true, force: true });
for (const entry of readdirSync(dist)) cpSync(join(dist, entry), join(destination, entry), { recursive: true });
const media = join(root, 'service', 'www', 'media');
if (destination !== join(root, 'service', 'www') && existsSync(media)) cpSync(media, join(destination, 'media'), { recursive: true });
console.log(`Staged UI into ${destination}`);
