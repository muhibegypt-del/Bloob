/** Keep a small set of likely next images decoded for screen transitions. */
const MAX_IMAGES = 8;
const cache = new Map<string, { image: HTMLImageElement; ready: Promise<boolean> }>();

export function preloadImage(src: string): Promise<boolean> {
  const cached = cache.get(src);
  if (cached) {
    cache.delete(src);
    cache.set(src, cached);
    return cached.ready;
  }

  const image = new Image();
  const ready = new Promise<boolean>((resolve) => {
    image.decoding = 'async';
    image.onload = () => {
      if (image.decode) {
        let settled = false;
        const finish = (ok: boolean) => {
          if (settled) return;
          settled = true;
          window.clearTimeout(timeout);
          resolve(ok);
        };
        // Some TV WebKit builds expose decode() but never resolve it.
        const timeout = window.setTimeout(() => finish(image.naturalWidth > 0), 700);
        void Promise.resolve().then(() => image.decode()).then(
          () => finish(true), () => finish(image.naturalWidth > 0),
        );
      } else resolve(true);
    };
    image.onerror = () => resolve(false);
  });
  const entry = { image, ready };
  cache.set(src, entry);
  if (cache.size > MAX_IMAGES) cache.delete(cache.keys().next().value!);
  void ready.then(ok => { if (!ok && cache.get(src) === entry) cache.delete(src); });
  image.src = src;
  return ready;
}
