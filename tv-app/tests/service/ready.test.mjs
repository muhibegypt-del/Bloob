import { describe, expect, it } from 'vitest';
import readyModule from '../../service/ready.js';

const { createReadyCheck } = readyModule;
const h = ch => ch.repeat(40);

describe('Ready now through the owner’s AllDebrid', () => {
  it('marks ready versions and removes only the not-ready magnets it added', async () => {
    const calls = [];
    const alldebrid = async (endpoint, fields) => {
      calls.push([endpoint, fields]);
      if (endpoint === '/v4.1/magnet/status') return { magnets: [{ id: 9, hash: h('a'), statusCode: 1 }] };  // already in the account, downloading
      if (endpoint === '/v4/magnet/upload') return { magnets: [{ id: 10, hash: h('b'), ready: true }, { id: 11, hash: h('c'), ready: false }] };
      return {};
    };
    const check = createReadyCheck(alldebrid);
    const candidates = ['a', 'b', 'c', 'd'].map(x => ({ id: `magnet:${h(x)}#0` }));
    const out = await check.annotate(candidates, [h('a'), h('b'), h('c')], 1000);
    expect(out.map(c => c.ready)).toEqual([false, true, false, undefined]);
    const upload = calls.find(c => c[0] === '/v4/magnet/upload');
    expect(upload[1]).toEqual([['magnets[]', h('b')], ['magnets[]', h('c')]]);  // the account's own magnet is not re-added
    const deletes = calls.filter(c => c[0] === '/v4/magnet/delete').map(c => c[1].id);
    expect(deletes).toEqual(['11']);  // never 9 (the owner's) or 10 (ready)
    calls.length = 0;
    await check.annotate(candidates, [h('a'), h('b'), h('c')], 1000);
    expect(calls).toEqual([]);  // remembered for six hours
  });
});
