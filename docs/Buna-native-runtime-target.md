# Buna native runtime target

This is the implementation target for an original Bloob native app. It follows
the **observed organization** of Elements of A without importing its code,
binary, private API, artwork, or UI assets. Protected-content paths are outside
this work. The installed QML experiment is retained as evidence of a failed
approach; it is not the foundation for this runtime.

## Why the QML experiment failed the architectural goal

| Boundary | Elements of A evidence | QML experiment | Buna target |
| --- | --- | --- | --- |
| Launch | 32-bit ARM executable starts `AppEngine`, its graphics engine and `NodeJsMT` (`Luna-runtime-architecture.md`, `004b8930`, `004b4340`) | webOS starts `/usr/bin/qml-runner` | Bloob executable owns startup, shutdown and runtime threads |
| Input | Native key event enters `AppEngine::keyInput`, crosses a V8 callback, then reaches app navigation (`004b616c`, `00c594a8`) | Qt delivers QML key events directly to one view | Native input adapter emits semantic events to a single app controller |
| UI | App-owned drawing tree and page deck; `Gfx::IEngine::processFrame` has a dirty-tree idle path (`002e8bc8`) | Qt owns scene graph, layout and frame loop | Original scene/page system owns focus, invalidation and render requests |
| Video composition | Player page draws a `Media::Player` through `Gfx.DrawingContext::drawVideo` (`002eb2e0`) | `VideoOutput` relies on Qt's opaque or underlay behavior | Player surface and overlays share an explicit composition contract |
| Playback | Shared session chooses progressive or HLS source; native `MsePlayer`, track pump and LG adapter manage samples, buffer and state (`Elements-of-A-system-map.md`) | one Qt `MediaPlayer` receives a URL | App-owned session owns clock, buffering, seek, tracks, errors and teardown; backend adapter owns platform calls |
| Service | Root intent router gates and opens a player session | page code calls the existing web app's service | Catalog/source resolver remains a separate adapter behind typed requests |

The reference's `Luna` is part of its shipped application executable and
JavaScript bundle. It is not the public webOS `luna://` bus and is not a library
we can link into Bloob. Recreating this organization is possible, but a QML
manifest alone does not create the native host, renderer or media adapter.

## Original runtime boundaries to implement

1. **Host:** a webOS `type: native` executable for this TV's 32-bit ARM userland.
   It initializes platform display/input/media connections, an application
   controller, graphics resources and one script/event thread if scripting is
   retained. It owns orderly teardown. A local Zig cross compiler now produces
   a matching ARM EABI executable and a simple probe ran on the TV; see
   `native-host/README.md`. A matching platform API/sysroot contract is still
   needed for graphics and media. Do not package the reference executable or
   its libraries.
2. **Event bridge:** receive remote keys, visibility, resume and close events;
   timestamp and queue them. Deliver each event once to the active page. The
   controller turns a focused card action into a typed `OpenDetails`, `Play`,
   `Back` or `Seek` intent. Pages never start a decoder directly.
3. **Scene and scheduler:** retain a small tree of page, image, text and control
   nodes. Track dirty regions and animation deadlines. Request a frame for
   input, new data, media state, decoded art or animation, then sleep while
   unchanged. Put artwork decode/cache work off the input thread. Start with
   one original visual theme; do not transplant reference assets.
4. **Page deck:** home, details, episode choice, source choice, loading, player
   and post-play are explicit states. The root coordinates transitions and
   cancels late catalog/source replies. Only the visible page receives keys.
5. **Playback session:** one owner for source, media state, position, duration,
   seekable range, buffering, selected tracks, first frame, errors and resume.
   The player page reads a state snapshot and sends commands. Back closes the
   session and releases source buffers, media handles and video geometry.
6. **Media adapters:** begin with a progressive URI backend using supported LG
   media facilities and the already bundled free test clip. Add a segmented
   source-buffer backend only after the URI path has reliable first-frame,
   pause, seek, state and cleanup. The reference's `TrackPump` and
   `LgEsPlayerPalImpl` show where buffering and sample submission belong; their
   private calls are *evidence for the design*, not callable Bloob APIs.
7. **Service adapter:** keep the existing Bloob catalog, indexer and AllDebrid
   service behind a narrow request interface initially. Store account keys in
   that service. Once the native host works, package an independent service
   with the native app and migrate the saved pairing without deleting the
   existing app or account files.

## The first native executable must prove one complete path

The first package should launch the original host, render a home card, accept
remote OK, open a details page, start the bundled free clip through its media
adapter, display a position clock driven by media callbacks, pause/seek, and
return with Back. It must release media resources on exit and still accept
remote input. Record the input-to-first-frame and exit transitions in a
URL-free log. This one path is the minimum evidence that the host, input,
rendering, media and lifecycle boundaries work together. Catalog and AllDebrid
integration follows that path rather than hiding an unproven player behind a
finished-looking browser.

## Known evidence and unknowns

The structural call paths above are supported by the local static map in
`research/Luna-runtime-architecture.md` and the watch-action map in
`research/Elements-of-A-system-map.md`. The owner's later report that QML UI
and video did not work supersedes the earlier brief clip confirmation as a
product acceptance result. The REA server identified the local reference ELF
as ARM, but its configured Windows deep-analysis providers were unavailable;
the existing Ghidra notes remain the basis for the cited native calls.

We have only a verified ARM load probe, not a Bloob native UI host or a matching platform API/sysroot,
the precise LG media interface contract for this TV, a timestamped reference
input-to-frame trace, or a resolved-title playback pass in a native host.
Those are implementation work, not evidence that the goal is impossible.
