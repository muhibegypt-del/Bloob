# TV playback prototype

Standalone LG webOS vertical slice built from the user's Solid TV UI. It does not use the Stremio Theater bundle or Stremio streaming service.

## Architecture

Remote/UI → Play intent → source choice → AllDebrid resolution (local service) → one playback session → LG HTML5 video path. The player pauses, removes its source, and calls `load()` when leaving playback. The local service saves the AllDebrid key in its app-specific `/media/internal/com.muhib.tvprototype/account.json` on the TV, outside the IPK; it reports whether that save succeeded. Pairing once after this update is necessary. No key is stored in the app bundle.

Source discovery uses a direct HTTPS indexer adapter in the local service (currently hosted Torrentio) using Cinemeta's IMDb or episode ID. The app does not scrape sites, maintain its own index, or run a Stremio server. Results are deduplicated, grouped by reported resolution, and summarized into release cards before AllDebrid magnet or link resolution. A clear file or quality selection screen appears when needed. Provider availability and result quality remain external dependencies. The Big Buck Bunny test is a bundled 30-second H.264/AAC clip, served with HTTP byte-range support. The source clip is licensed CC BY 3.0; attribution is in `service/www/media/CREDITS.txt`.

The player samples `video.currentTime` twice a second so its progress display follows the TV media clock even when `timeupdate` events are sparse. Its remote controls provide 10-second seeking, play/pause, and menus for audio and subtitle tracks exposed through the public media element APIs. Some containers expose no selectable tracks; in that case the menu explains the limitation. External subtitle acquisition and WebVTT loading are not implemented yet. This reproduces the useful session lifecycle and interaction pattern, not the source app's private player implementation.

## Local check

Run `npm install` and `npm run typecheck && npm run build` in this folder. Start `node service/server.js`, then `npm run dev`. Vite forwards `/api` calls to the local service on port 8787.

## webOS package

After building, copy the contents of `dist/` into `service/www/` and run `ares-package app service -o .` from this folder. Install the resulting IPK as a **separate app** (`com.muhib.tvprototype`); it does not replace the existing Stremio app.

## TV verification

Use the Playback Test row for the direct video first. Confirm start, pause, 10-second seeking, Back, and decoder resource release. Then pair AllDebrid with its PIN and try a source you are authorized to use. Record click-to-first-frame and seek time, plus CPU and memory during playback and after exit.
