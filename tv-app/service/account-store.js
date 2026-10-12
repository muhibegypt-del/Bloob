const fs = require('fs').promises;
const path = require('path');

// webOS keeps JS service data under /media/internal. Keep this app's token
// outside the packaged service so an IPK update or service restart preserves it.
const directory = process.env.TV_APP_DATA_DIR ||
  (process.platform === 'win32'
    ? path.join(process.env.LOCALAPPDATA || process.cwd(), 'TVPrototype')
    : '/media/internal/com.muhib.tvprototype');
const file = path.join(directory, 'account.json');

async function load() {
  try {
    const saved = JSON.parse(await fs.readFile(file, 'utf8'));
    return typeof saved.apiKey === 'string' ? saved.apiKey : '';
  } catch (error) {
    if (error.code === 'ENOENT') return '';
    throw error;
  }
}

async function save(apiKey) {
  await fs.mkdir(directory, { recursive: true, mode: 0o700 });
  const temporary = file + '.' + process.pid + '.tmp';
  await fs.writeFile(temporary, JSON.stringify({ apiKey }), { mode: 0o600 });
  await fs.rename(temporary, file);
}

module.exports = { load, save };
