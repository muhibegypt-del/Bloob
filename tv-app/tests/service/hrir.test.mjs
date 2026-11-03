import { describe, expect, it } from 'vitest';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import hrirModule from '../../service/hrir.js';

const { publicList, read } = hrirModule;

function wav(channels, rate) {
  const b = Buffer.alloc(44 + channels * 4 * 8);
  b.write('RIFF', 0); b.writeUInt32LE(b.length - 8, 4); b.write('WAVE', 8);
  b.write('fmt ', 12); b.writeUInt32LE(16, 16); b.writeUInt16LE(3, 20); b.writeUInt16LE(channels, 22);
  b.writeUInt32LE(rate, 24); b.writeUInt32LE(rate * channels * 4, 28); b.writeUInt16LE(channels * 4, 32); b.writeUInt16LE(32, 34);
  b.write('data', 36); b.writeUInt32LE(channels * 4 * 8, 40);
  return b;
}

describe('measured headphone responses', () => {
  it('are found, named, checked and read by name only', () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'bloob-hrir-'));
    const before = process.env.TV_APP_DATA_DIR;
    process.env.TV_APP_DATA_DIR = dir;
    try {
      fs.mkdirSync(path.join(dir, 'headphones'));
      fs.writeFileSync(path.join(dir, 'headphones', 'dolby-atmos.wav'), wav(14, 48000));
      fs.writeFileSync(path.join(dir, 'headphones', 'old_44k.wav'), wav(14, 44100));
      fs.writeFileSync(path.join(dir, 'headphones.wav'), wav(16, 48000));
      fs.writeFileSync(path.join(dir, 'other.wav'), wav(14, 48000));  // beside the data: ignored
      const list = publicList();
      expect(list.map(e => e.id)).toEqual(['dolby-atmos.wav', 'old_44k.wav', 'headphones.wav']);
      expect(list[0]).toMatchObject({ name: 'Dolby Atmos', usable: true, channels: 14 });
      expect(list[1].usable).toBe(false);
      expect(list[1].why).toMatch(/48 kHz/);
      expect(list[2].name).toBe('Your response');
      expect(read('dolby-atmos.wav').length).toBeGreaterThan(44);
      expect(read('old_44k.wav')).toBeNull();
      expect(read('../account.json')).toBeNull();
      expect(JSON.stringify(list)).not.toContain(dir);  // no paths for the app
    } finally {
      if (before === undefined) delete process.env.TV_APP_DATA_DIR; else process.env.TV_APP_DATA_DIR = before;
    }
  });
});
