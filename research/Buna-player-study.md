# Buna player study

This is the working reconstruction ledger for the player experience. "Buna" is our own player UI and session code; it is not an attempt to load the source app's executable into our app. This study covers ordinary playback controls and media behavior, excluding protected-content paths.

## Evidence examined

- Extracted `res/PlayerPage.js`, locally formatted as `research/PlayerPage.formatted.js` for reading. The latter is a formatting aid, not a source release.
- `res/PlayerStore.js` and the focused ARM decompilations referenced in `research/Elements-of-A-system-map.md`.
- Live read-only comparison of playback and exit process resources recorded in the system map.
- Our prototype's real webOS `HTMLVideoElement` state during AllDebrid playback and bundled test playback.
- Extracted `res/PlayerStore.js` and HLS chunk `res/6606.js`, locally formatted as `research/PlayerStore.formatted.js` and `research/HlsWorker.formatted.js`.

## Runtime boundaries established by shipped code

```text
Play intent / PlayerPage
  → PlayerStore.play (session ownership and options)
  → PlaybackSession.start (state, position, tracks, lifecycle)
  → Player Instance chooses HLS or progressive by URL / forced option
     ├─ ProgressivePlayerLuna → Media.Streaming.UriSource → Media.getPlayer("mse")
     └─ HLS bridge/worker → HLS engine → MediaSource + SourceBuffer shim
                                           → Luna Media.SourceBuffer
  → native Media::MsePlayer → TrackPump → LgEsPlayerPalImpl → LG pipeline interface
  → UnifiedPlayerWidget + TransportBar draw session state and handle remote keys
```

Static evidence: `PlayerStore.formatted.js` lines 43–136 show the progressive adapter; lines 280–565 show the HLS bridge and player selection; lines 1445–2020 show `PlaybackSession`; lines 2260–2300 show `PlayerStore.play`. `HlsWorker.formatted.js` lines 1297–1431 show the `SourceBuffer` shim; lines 2807–3820 contain the HLS player. Native call evidence is in the existing decompilation files and system map. This diagram is a code path, not a claim that every branch ran during the observed Elements of A movie.

### Position and seek

- The progressive adapter receives native state callbacks carrying position, duration, speed, buffering, and error. `getPosition()` refreshes from `lunaPlayer.getState()` when its last callback is over one second old (`PlayerStore.formatted.js` lines 43–136).
- `PlaybackSession.start()` polls position-dependent state every 100 ms (`PlayerStore.formatted.js` near line 1777). Its `seek()` accumulates a delta, clamps it, and debounces the backend position change (`PlayerStore.formatted.js` near line 1912). The HLS bridge caches requested position while the worker completes a seek (near line 370).
- HLS `setPosition()` changes the engine's `realCurrentTime`, sends a Seeking state, and supports seeking before auto-load via a start position (`HlsWorker.formatted.js` near line 3302).
- The native export `research/native-seek-decomp.txt` verifies that `MsePlayer::seek` clears per-track flags, branches on player/worker state, updates source offsets and track state, calls `seekSourceBuffers`, publishes status, and calls `updateSeeking` (function `000e9370`). `updateSeeking` checks buffer position, flushes the progress monitor, invokes an indirect player call, and updates track state (function `000e243c` in `research/native-track-decomp.txt`). The exact state enum values and indirect interface targets are not yet resolved.
- `LgEsPlayerPalImpl::seekTo` converts its time argument for the LG interface, calls two indirect LG pipeline methods, and on success resets buffered state and time bookkeeping (function `000cba08`). Ghidra's inferred signature and `this` argument are imperfect; the two LG virtual slots and time unit need validation before writing a native adapter.

### Tracks and subtitles

- `PlaybackSession` owns selected and active audio/subtitle tracks, preference policy, and subtitle cue updates (`PlayerStore.formatted.js` near lines 1547, 1710–1830).
- The progressive `UriSource` adapter's `setAudioTrack`, `setSubtitleTrack`, and `updateSubtitleTrack` methods are empty (`PlayerStore.formatted.js` near lines 110–118). Its `getThumbnail()` also returns no image. The generic UI's menu does not imply that this backend can switch tracks.
- The HLS bridge has worker messages for audio, subtitle, seek, and thumbnail operations (`PlayerStore.formatted.js` near lines 370–414). The worker selects audio by persistent track ID and subtitles by its track manager (`HlsWorker.formatted.js` near lines 3322–3350). Its `SourceBuffer` shim appends shared buffers to the Luna native source buffer with eviction/backpressure checks (near lines 1297–1343).
- Native `MsePlayer::selectVideoTrack` adds or removes a selected track from a `TrackPump` and updates track state (function `000e1c68` in `research/native-track-decomp.txt`). `setAudioFormat` passes a constructed format through an indirect ES-player interface method (`000e9cb4`); the LG adapter checks whether a codec change is permitted (`000cc4bc`). This demonstrates lower-level track and audio-format handling, but does not prove that the progressive JavaScript adapter exposes a user-selectable audio track.
- Therefore, our direct AllDebrid file path needs its own actual track-capable backend or remuxed/adapted stream before an polished track menu can function reliably. A cosmetic menu cannot solve this.

## Player UI behavior found in the shipped Elements of A code

| Behavior | Evidence in formatted PlayerPage | Buna translation |
| --- | --- | --- |
| Separate `Full`, `Reduced`, and `Deck` UI modes | `PlayerUiState` near line 4521; `UnifiedPlayerWidget` near 4535 | One transport-first HUD, reduced controls during seeking, separate track menu. |
| Transport bar binds current time, duration, seekable range, buffering and state | Bindings near lines 2085–2130; `TransportBar` near 3904 | Read the TV media clock and seekable range; keep seek preview separate from actual position until committed. |
| Controls auto-dismiss, remain visible when paused or interacting, and can reappear on buffering | `UnifiedPlayerWidget.show`, `hide`, `restartTimer`, and configuration near lines 2150, 4880–4930 | Hide chrome during uninterrupted playback; show status only when it matters. |
| Seek controls reduce other UI while scrubbing; short direction input seeks 10 seconds | `updatePlayerUiWhileScrubbing` near 4867; `keyHandlingSeekOrTrickplay` near 3300 | Direction keys show a short seek preview and commit after the last press. |
| Audio and subtitle menus use selected/available track collections | Menu setup near lines 2010–2080 | Show actual browser-exposed tracks; do not invent selectable streams. |
| Back dismisses visible controls before ending the session | `PlayerPage` Back handler near line 3255 | First Back hides the visible HUD; the next Back exits playback. |

These are static code observations. The exact visual output, pixel dimensions, animation timing on the user's TV, and every runtime branch have not been observed. Our UI implements the interaction ideas in original code and styling rather than copying Elements of A assets or code.

## What remains to study

| Area | Current evidence | Remaining work |
| --- | --- | --- |
| Player page and controls | Static `PlayerPage` widget tree, UI modes, remote handling, menus, transport bar | Capture visible Elements of A controls and remote behavior for pause, repeated seek, tracks, and exit on this TV. |
| Session and policy | `PlayerStore`/`PlaybackSession` start, state, seek, tracks, preloading, stop | Follow all relevant state callbacks through actual playback; identify which settings and capabilities are active. |
| HLS and progressive source paths | Factory selection, HLS worker message bridge, MSE shim, progressive `UriSource` | Trace variant/segment fetch, stream switches, buffered ranges, and teardown on one ordinary unprotected session. |
| Native media core | `MsePlayer`, `TrackPump`, LG adapter setup, sample submission, seek and cleanup functions decompiled | Resolve LG indirect interface methods, data formats, and exact event ordering; validate decompiler types. |
| Hardware output | `/proc` shows media threads and LG device descriptors during Elements of A playback | Match first frame, output mode, buffer behavior, resource use and cleanup to a timestamped runtime trace. |
| Buna backend choice | Our HTML video path already plays HDR/Atmos; direct file controls have gaps | Measure first; decide whether public webOS APIs plus a source adapter suffice or whether a native backend is justified. |

Do not record account values, full stream URLs, or protected-content traffic in live traces. The source module code and native decompilation establish structure; they do not replace a runtime trace of the specific film the user watched.

## Current prototype boundary

**Design proposal, now implemented (v0.2, 12 October 2026):** Buna lives in `tv-app/src/buna/`. It translates the session ideas above into original code. One session owns clock, seek, buffering, tracks, resume, metrics, and teardown. The UI only renders `PlaybackState`. The seek preview stays separate from the committed position until the backend acknowledges it. Capabilities state whether a feature is supported and whether it was verified on the TV. The only backend is the public `HTMLVideoElement` path, which the user reported delivering HDR and Atmos. These behaviors are verified by unit tests and a key-driven run in desktop Chromium. **They have not yet been observed on the TV.** Still missing: preview thumbnails, external subtitles, frame-rate matching, output-mode reporting, and any HLS/MSE or native LG ES adapter. The track-switching gap described above remains the main reason a second backend might become justified; measure it on the TV first.
