import { onCleanup } from 'solid-js';

export type Key = 'up' | 'down' | 'left' | 'right' | 'enter' | 'back';

// LG Magic Remote / D-pad key codes. 461 is webOS "Back".
const CODES: Record<number, Key> = {
  37: 'left',
  38: 'up',
  39: 'right',
  40: 'down',
  13: 'enter',
  461: 'back',
  27: 'back',
  8: 'back',
};

// Only the top screen receives a key. A single key event must not reach a
// newly mounted screen after the current screen navigates away.
const handlers: ((key: Key) => void)[] = [];
window.addEventListener('keydown', (event) => {
  const key = CODES[event.keyCode];
  if (!key || !handlers.length) return;
  event.preventDefault();
  handlers[handlers.length - 1](key);
});

/** Register a remote-key handler for the lifetime of the calling component. */
export function useKeys(handler: (key: Key) => void) {
  handlers.push(handler);
  onCleanup(() => {
    const index = handlers.indexOf(handler);
    if (index !== -1) handlers.splice(index, 1);
  });
}
