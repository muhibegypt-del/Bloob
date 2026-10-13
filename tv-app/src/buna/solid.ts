import { onCleanup } from 'solid-js';
import { createStore } from 'solid-js/store';
import type { BunaSession } from './session';
import type { PlaybackState } from './types';

/**
 * Mirror a session's state into a Solid store. Views read this store and
 * nothing else. Buna publishes immutable snapshots and reuses unchanged track
 * arrays, so Solid can patch changed fields without serializing the whole state.
 */
export function useBunaState(session: BunaSession): PlaybackState {
  const [state, setState] = createStore<PlaybackState>({ ...session.state });
  const unsubscribe = session.subscribe(next => setState(next));
  onCleanup(unsubscribe);
  return state;
}
