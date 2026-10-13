# Bloob TV app

LG webOS app: SolidJS interface, a local webOS service for sources and AllDebrid, and **Buna**, Bloob's own playback runtime. It does not use any Stremio bundle or streaming server.

## UI responsiveness in 0.2.4

- Home and episode shelves mount only tiles near the current focus. Home also mounts only nearby vertical rows while preserving their positions for Back navigation.
- Backdrop artwork is preloaded through a bounded eight-image cache, then crossfaded only after it is ready. The full-screen idle drift and Sources blur were removed.
- Remote focus scrolling on Sources is coalesced to one pending frame and only scrolls when focus is outside the useful viewport.
- Sources and Player load as separate chunks while the preceding screen is visible. Buna's Solid adapter no longer serializes the full playback state at each clock sample.
- Focus and navigation transitions settle faster. The media backend and resolution path remain the same.

## Architecture

```text
Remote keys (keys.ts: one handler owns each key press)
  → Home → Details → Sources → Player        (App.tsx; each layer stays mounted under the next)

Sources:  /api/sources/search → service/release.js (parse release names into claims)
          → src/sources/ranker.ts (recommendation + reasons)
          → src/sources/resolver.ts (AllDebrid: magnet → file → link, cancellable)
          → ResolvedSource + PlayIntent

Player:   buna.open() → BunaSession (state machine, clock, seek, stalls, tracks, resume, metrics)
          → WebVideoBackend (the only code that touches <video>)
          → PlaybackState → Player view (renders state, sends commands)
```

### Buna (`src/buna/`)

| File | Role |
| --- | --- |
| `types.ts` | Contracts: `PlayIntent`, `SourceCandidate`, `ResolvedSource`, `BackendCapabilities`, `PlaybackState`, `BunaBackend`, metrics |
| `session.ts` | One playback. Phase is derived (`loading`, `playing`, `paused`, `buffering`, `seeking`, `ended`, `error`) from backend facts plus the user's play/pause intent |
| `backends/web-video.ts` | `HTMLVideoElement` adapter: events, snapshots, track lists, capability reporting, decoder release |
| `runtime.ts` | At most one live session; pauses when webOS hides the app; picks the backend |
| `positions.ts` | Resume positions (content ID and seconds only) |
| `solid.ts` | Mirrors session state into a Solid store |
| `metrics.ts` | In-memory timing ring, readable on the TV as `window.__buna.metrics()` |
| `trace.ts` | Bounded event timeline for remote, source, session, media and webOS track-service activity; no URLs, titles, PINs, keys or raw replies |

Guarantees, each covered by tests:

- **Clock provenance.** `position` is a fresh media-clock reading (`positionSource: 'media'`) or a seek acknowledgement (`'seek-ack'`). `duration` records whether it came from the media or the seekable range. A NaN from the TV doesn't wipe a known duration.
- **Seek preview vs. commit.** Direction keys move `previewTarget` and accelerate while held (10 s, then 30, 60, 120). One seek is sent after 600 ms of quiet. `pendingTarget` holds until the backend confirms with `seeked`, or a settled clock reading if the TV drops that event. A stale clock can't move the thumb mid-seek. A seek that never settles times out back to the media clock.
- **Stalls without `waiting`.** A playing clock that stops advancing for 3 s becomes `buffering`, and clears as soon as it moves.
- **Idle cost.** The clock samples every 250 ms only while time can move. A paused session runs no timers.
- **Honest capabilities.** Track selection, external subtitles, thumbnails, frame-rate matching and output info report `supported` plus a reason, and `tv: 'unverified' | 'user-report' | 'measured'`. Menus show the reason instead of fake options.
- **Deterministic teardown.** `dispose()` is idempotent. It saves the resume point, clears every timer, unsubscribes, pauses, removes the source, and calls `load()` to release the decoder. Late events are ignored.
- **Errors.** Media errors map to plain-language messages and say whether retrying makes sense. `retry()` reloads at the last position, and expired links are refused up front.

### Service (`service/`)

- `server.js`: UI hosting, `/api/*`, and the range-enabled test clip. The API accepts only `Host: 127.0.0.1|localhost` and JSON POSTs, which blocks cross-site requests and DNS rebinding from other web content on the TV. It must stay compatible with the TV's older Node (no `?.` or `??`).
- `release.js`: Torrentio stream entries → candidates with resolution, HDR, codec, audio, channels, source type, size, seeders, language flags, pack/file hints. It accepts both `title` and `description` formats. It reports claims only, nothing verified.
- `account-store.js`: AllDebrid key persistence. It tries `/media/internal/com.muhib.tvprototype`, then `~/.bloob`, then the service folder (lost on reinstall), then the temp dir (lost on reboot). Each write is verified by reading it back. `GET /api/account` reports `paired`, `persistent`, `storage`, `durable`, and per-location failure codes, never the key. A key AllDebrid rejects (`AUTH_*`) is forgotten so the UI asks to pair again.

## Checks

```sh
npm install
npm run typecheck
npm test            # Buna session, ranker, resolver, release parser, account store (65 tests)
npm run test:e2e    # real app + service + Buna in Chromium, driven by remote keys (26 checks)
```

The end-to-end run needs `ffmpeg` and a Chromium (`CHROMIUM_PATH`, default `/opt/pw-browsers/...`). Chromium builds without proprietary codecs can't decode H.264/AAC, so the test serves a VP9/Opus transcode of the bundled clip in its place. Everything else is the shipping code. Screenshots land in `node_modules/.cache/bloob-e2e/`.

For a local UI session: `node service/server.js`, then `npm run dev`. Vite forwards `/api` to port 8787.

## Package and install

```sh
npm run package     # build → stage dist into service/www (old bundles removed) → artifacts/*.ipk
ares-install -d rootedTV artifacts/com.muhib.tvprototype_0.2.4_all.ipk
```

It installs as a separate app (`com.muhib.tvprototype`, titled Bloob).

## TV verification

1. **Pairing persistence.** Pair once. The account row on the Sources screen says where the key was saved. "Saved on this TV" is the goal. Restart the app, then run `node scripts/inspect-tv.cjs <inspector-port> account`. It should report `paired: true`, `persistent: true`, `durable: true` without a new PIN. If it says "refused to save it", the row lists each location's error code, so report those.
2. **Test clip.** Start it, pause and resume with OK and with the remote's ▶/❚❚ keys, hold → to seek, then Back. `node scripts/inspect-tv.cjs <port> buna` prints the state and timing metrics (`first-frame`, `seek` with `settleMs` and `landed`, `stall`, `dispose`).
3. **Real source.** Take the recommended option on a title you're authorized to watch. Check first frame, seek accuracy (`landed` vs `to`), the audio and subtitle menus (do they list tracks on this TV?), Back, and memory after exit.

### Live playback trace

The app records the latest 600 diagnostic events in memory automatically. It samples the media clock at most once per second. The TV inspector can read `window.__buna.trace.list()`, `window.__buna.trace.since(sequence)`, or `window.__buna.trace.clear()`. The trace contains timestamps, remote key names, navigation transitions, search/resolve timing, player commands, media state, track counts and webOS media-service call outcomes. It does not include stream links, account data, titles, media IDs, or raw service responses.

After forwarding the app inspector with `ares-inspect`, run `node scripts/inspect-tv.cjs <inspector-port> follow` to print one JSON event per line as playback happens. Use `trace` instead of `follow` for a one-time snapshot. Stop the live reader with Ctrl+C. The inspector must remain connected; the trace ring is lost when the app process exits.
