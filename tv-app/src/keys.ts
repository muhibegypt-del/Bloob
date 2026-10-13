import { onCleanup } from 'solid-js';
import { trace } from './buna/trace';

export type Key =
  | 'up' | 'down' | 'left' | 'right' | 'enter' | 'back'
  | 'play' | 'pause' | 'playpause' | 'stop' | 'ff' | 'rw';

// LG Magic Remote / D-pad key codes. 461 is webOS "Back"; 412–417 and 19 are
// the remote's transport keys. 8/27 and 179/178 cover desktop testing.
const CODES: Record<number, Key> = {
  37: 'left',
  38: 'up',
  39: 'right',
  40: 'down',
  13: 'enter',
  461: 'back',
  27: 'back',
  8: 'back',
  415: 'play',
  19: 'pause',
  179: 'playpause',
  413: 'stop',
  178: 'stop',
  417: 'ff',
  412: 'rw',
};

// While typing in a text field, these keys keep their editing meaning.
const EDITING_CODES = new Set([8, 37, 39]);

function isEditable(target: EventTarget | null) {
  const element = target as HTMLElement | null;
  return !!element && (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA' || element.isContentEditable);
}

// Only the top screen receives a key. A single key event must not reach a
// newly mounted screen after the current screen navigates away.
const handlers: ((key: Key, event: KeyboardEvent) => void)[] = [];
window.addEventListener('keydown', (event) => {
  const key = CODES[event.keyCode];
  if (!key || !handlers.length) return;
  if (isEditable(event.target) && EDITING_CODES.has(event.keyCode)) return;
  event.preventDefault();
  trace.record('remote', key);
  handlers[handlers.length - 1](key, event);
});

/** Register a remote-key handler for the lifetime of the calling component. */
export function useKeys(handler: (key: Key, event: KeyboardEvent) => void) {
  handlers.push(handler);
  onCleanup(() => {
    const index = handlers.indexOf(handler);
    if (index !== -1) handlers.splice(index, 1);
  });
}
