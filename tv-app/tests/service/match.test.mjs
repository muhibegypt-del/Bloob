import { describe, expect, it } from 'vitest';
import matchModule from '../../service/match.js';

const { titleOf, titleMatches, strict } = matchModule;

describe('strict title matching (4.1.1)', () => {
  it('cuts the title out of release names', () => {
    expect(titleOf('Dark.S01E05.Truths.1080p.NF.WEB-DL.DDP5.1.x264-NTb')).toBe('dark');
    expect(titleOf('Dark.Matter.2024.S01E05.2160p.ATVP.WEB-DL')).toBe('dark matter');
    expect(titleOf('www.Torrenting.com - Dark S01E05 720p')).toBe('dark');
    expect(titleOf('[SubsPlease] Sousou no Frieren - 05 (1080p)')).toBe('sousou no frieren');
    expect(titleOf('1917.2019.2160p.UHD.BluRay.REMUX')).toBe('1917');
  });
  it('keeps the show and drops other shows that share a word', () => {
    expect(titleMatches('Dark.2017.S01E05.GERMAN.1080p', ['Dark'])).toBe(true);
    expect(titleMatches('Dark.Season.1.Complete.1080p', ['Dark'])).toBe(true);
    expect(titleMatches('Dark.Matter.2024.S01E05', ['Dark'])).toBe(false);
    expect(titleMatches('The.Terminal.List.S01E05.1080p', ['Dark'])).toBe(false);
    expect(titleMatches('Darkness.2023.S01E01', ['Dark'])).toBe(false);
  });
  it('allows accents, "the", country suffixes and aliases', () => {
    expect(titleMatches('Shogun.2024.S01E01', ['Shōgun'])).toBe(true);
    expect(titleMatches('The.Office.US.S01E01', ['The Office'])).toBe(true);
    expect(titleMatches('Office.S01E01', ['The Office'])).toBe(true);
    expect(titleMatches('Sousou.no.Frieren.S01E05', ['Frieren: Beyond Journey’s End', 'Sousou no Frieren'])).toBe(true);
  });
  it('holds films to their year', () => {
    const list = [{ release: 'Heat.1995.2160p.UHD.BluRay' }, { release: 'Heat.2013.1080p.WEB-DL' }, { release: 'The.Heat.2013.1080p' }];
    expect(strict(list, { type: 'movie', titles: ['Heat'], year: 1995 }).map(c => c.release)).toEqual(['Heat.1995.2160p.UHD.BluRay']);
  });
  it('drops nothing when the title is unknown', () => {
    const list = [{ release: 'Anything.S01E01' }];
    expect(strict(list, null)).toEqual(list);
  });
});
