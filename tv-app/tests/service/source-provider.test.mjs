import { EventEmitter } from 'node:events';
import { describe, expect, it } from 'vitest';
import providerModule from '../../service/source-provider.js';

const { createTorrentioProvider } = providerModule;

describe('source provider boundary', () => {
  it('requests the exact episode and returns typed candidates', async () => {
    let options;
    const client = {
      get(requestOptions, callback) {
        options = requestOptions;
        const request = new EventEmitter();
        request.destroy = error => request.emit('error', error);
        queueMicrotask(() => {
          const response = new EventEmitter();
          response.statusCode = 200;
          callback(response);
          response.emit('data', JSON.stringify({ streams: [{
            title: 'Example.Show.S01E02.1080p.WEB-DL.mkv', infoHash: 'a'.repeat(40), fileIdx: 0,
          }] }));
          response.emit('end');
        });
        return request;
      },
    };
    const source = createTorrentioProvider(client);
    const results = await source.search('series', 'tt1234567:1:2');
    expect(source.name).toBe('Torrentio');
    expect(options.path).toBe('/stream/series/tt1234567:1:2.json');
    expect(results).toHaveLength(1);
    expect(results[0]).toMatchObject({ kind: 'magnet', provider: 'Torrentio', claimsFrom: 'release-name' });
  });
});
