import { describe, expect, it } from 'vitest';
import ratingsModule from '../../service/ratings.js';

const { scores } = ratingsModule;

describe('MDBList scores', () => {
  it('shows RT, Metacritic, IMDb and Letterboxd in that order, skipping missing ones', () => {
    const data = { ratings: [
      { source: 'imdb', value: 8.1, score: 81 },
      { source: 'metacritic', value: 87, score: 87 },
      { source: 'metacriticuser', value: null, score: null },
      { source: 'tomatoes', value: 97, score: 97 },
      { source: 'letterboxd', value: 4, score: 80 },
    ] };
    expect(scores(data)).toEqual(['RT 97%', 'Metacritic 87', 'IMDb 8.1', 'Letterboxd 4.0']);
    expect(scores({ ratings: [{ source: 'tomatoes', value: null }] })).toEqual([]);
    expect(scores(null)).toEqual([]);
  });
});
