# Bloob v1 implementation plan

This is the working contract for the first reliable release on the owner's LG webOS TV. It uses original Bloob code and ordinary media interfaces. The studied reference is called **Elements of A** in this repository. Its structural lessons inform this plan; its code, assets, services, and private runtime are not dependencies.

## Product boundary

- Keep Solid for browsing and the player interface. Keep the current LG-backed web video route while completing its controls.
- Keep the existing catalog, hosted source discovery, AllDebrid pairing, and authorized resolved links. Put the discovery provider behind a service interface so it can be replaced.
- Do not add an addon marketplace, a custom elementary-stream pipeline, or a second player unless the current route cannot supply a required capability.
- Never persist or log account keys or resolved URLs. Do not alter or delete TV files, packages, or accounts during inspection.

## One watch attempt

```text
remote action -> exact film/episode PlayIntent -> source search and ranking
  -> pairing if needed -> cancellable resolve -> ResolvedSource in memory
  -> loading -> one Buna session -> one playback backend
  -> backend events -> session state -> player UI
  -> ended / error / source change / exit -> resume or post-play -> browse
```

The watch coordinator owns the attempt identifier, current item, chosen source, return destination, and transition. A cancelled attempt must never accept a late search or resolve result. Screens issue actions and render state. The local service owns account and source operations. Buna owns one playback session, media clock, seeking, tracks, buffering, resume, metrics, and cleanup. A backend owns the actual media interface.

## Playback contract

- Buna's runtime accepts a backend host, not a DOM video element. The current web host creates `WebVideoBackend`; a future native host can implement the same session contract without pretending to be a video element.
- At most one active session. A replacement disposes the prior session before preparing a new source. Async work checks whether its attempt and session are still current.
- The UI distinguishes the seek preview, requested target, and media-clock landing. Once the backend acknowledges a seek, the actual landing position replaces the requested target.
- Track lists come from the current playable file, not release-name claims. A successful selection must be confirmed by the active media interface. If tracks cannot be listed or selected, the menu explains that capability honestly.
- Video, webOS media track controls, and any external subtitle path have separate capability reports. Do not infer that an LG service call works merely because another app uses it.
- On failure, retain the item and source context. Retry uses a still-valid link; expired links return to resolution. Choose another source returns to the same source list and focus.
- Back priority is menu, seek preview, player chrome, playback exit, then previous browsing layer. Exiting saves a valid resume point, stops timers and subscriptions, detaches the source, and restores focus.

## Build sequence

1. **Foundation:** make this document the single implementation contract; work from the 0.2 branch; remove the DOM element from `BunaRuntime.open()` while preserving current playback.
2. **Media controls:** inspect available browser and webOS track interfaces for the active `mediaId`; supply truthful embedded audio/subtitle lists and selection. Add an independent metadata source only if the TV does not expose track lists. Preserve the working picture/audio path.
3. **Watch coordination:** centralize loading, cancellation, source change, retry, exit, resume, and episode handoff. Keep resolved URLs transient.
4. **TV experience:** make source choice, player controls, focus, error recovery, and post-play consistent and remote-first. Use original visuals.
5. **Efficiency:** lazy-load player work, bound image/cache memory, stop idle timers, prepare only useful next-item data, and release playback resources on exit.
6. **Release candidate:** build the IPK from source, inspect package contents, then perform one integrated TV acceptance pass. Fix failures found in that pass.

## Media decision

First complete the existing web backend. If the active TV and file expose tracks through the media element or supported webOS media controls, keep that backend. If a required audio/subtitle function remains unavailable, make a focused full-screen native player proof that accepts a resolved source and returns position, selected tracks, and exit reason. Adopt it only if it fixes the gap while retaining working HDR/audio quality and acceptable entry/exit behavior.

## Acceptance on the owner's TV

- Pair once; app and service restart without another PIN.
- Choose a film or episode, take the recommended source, and reach first frame or a clear recoverable error.
- Seek with the remote; the displayed position follows the actual backend landing.
- Audio/subtitle choices shown for a file are real and selectable, or the limitation is stated accurately.
- Pause/resume, source failure, Back, resume position, and next episode behave consistently.
- Several play/exit cycles do not leave an active media session or cause a growing responsiveness problem.

The current 0.2 tests prove session and UI logic in desktop Chromium. They do not establish TV track support, output format, or native media behavior. User-reported HDR/Atmos playback is a valuable observation, not a general capability guarantee.

## Implementation status (13 October 2026)

- Foundation is in place: `BunaRuntime.open()` accepts a playback host. The web video element is confined to `WebVideoBackend`.
- The web backend now observes `sourceInfo` for its own `mediaId` when the webOS service bridge is available. It uses indexed native tracks only when they outnumber DOM tracks, waits for a successful TV response before showing a changed selection, and keeps embedded HLS subtitle switching disabled until verified safe. This path still needs the one TV acceptance run.
- Source search and resolve work reject stale results after cancel or navigation. The source list remains mounted under playback, and a numbered watch attempt blocks late playback from an old episode. The post-play panel can open the next episode's source list.
- Discovery now has a replaceable local-service provider interface; the current provider remains Torrentio. Image loading is lazy and the in-memory search cache is bounded.
- Home, Movies, and TV Shows are functional catalog sections. Unimplemented Library, Search, Trailer, and watchlist controls are not displayed. A local 1920×1080 visual pass covered those sections, Details, Sources, and the demo player.
- Local typecheck, 65 unit tests, and production build passed. The optional Chromium end-to-end check stopped before launch because this Windows host has no `ffmpeg` executable; its script also has a Linux browser default. A current IPK was built and its contents inspected. The saved TV connection timed out, so nothing was installed or validated on the TV.
