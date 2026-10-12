import { onCleanup } from 'solid-js';
import { createStore, reconcile } from 'solid-js/store';
import type { BunaSession } from './session';
import type { PlaybackState } from './types';

/**
 * Mirror a session's state into a Solid store. Views read this store and
 * nothing else; fine-grained reconciliation means only changed fields re-render.
 */
export function useBunaState(session: BunaSession): PlaybackState {
  const [state, setState] = createStore<PlaybackState>(structuredCloneState(session.state));
  const unsubscribe = session.subscribe(next => setState(reconcile(structuredCloneState(next))));
  onCleanup(unsubscribe);
  return state;
}

// The session's snapshots are immutable; copying keeps the store from aliasing them.
function structuredCloneState(state: PlaybackState): PlaybackState {
  return JSON.parse(JSON.stringify(state)) as PlaybackState;
}
