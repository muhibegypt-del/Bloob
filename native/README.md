# Bloob native (Buna N1)

A native webOS app (`com.muhib.bloob`, "Bloob Native") written in C++ against the community webOS NDK. It replaces the browser with our own host. Bloob controls the window, remote input, drawing and the playback session. Video goes through the TV's own media pipeline in a hardware plane under our UI.

It installs alongside the web app (`com.muhib.tvprototype`) and does not replace it.

## What N1 contains

| Part | File | How |
| --- | --- | --- |
| Window, remote | `src/main.cpp`, `src/keys.h` | SDL2 as shipped on the TV (LG's webOS SDL). Back/Exit are claimed with `SDL_WEBOS_ACCESS_POLICY_KEYS_*` hints; LG scancodes 482 (Back), 505 (Exit), 486–489 (colour keys) |
| Drawing | `src/gfx.cpp` | Own renderer. Inter text from stb_truetype glyph atlases, anti-aliased rounded shapes, every frame drawn at 1920×1080 to an offscreen canvas and scaled once. Redraws only on input, media events or a visible moving clock (no busy loop) |
| Video under UI | `src/main.cpp` (`open_player`, `place_video`) | `SDL_webOSCreateExportedWindow` → window ID passed to the media pipeline; UI pixels left transparent where video shows |
| Media | `src/media.cpp` | LG uMediaServer over the system bus (`luna://com.webos.media/load|subscribe|play|pause|seek|selectTrack|setSubtitleEnable|unload`). Real track lists, codec, resolution, frame rate, HDR type from `sourceInfo`/`videoInfo` |
| System bus | `src/luna.cpp` | LG `libhelpers` (`HLunaServiceCall`), replies marshalled to the UI thread |
| Session (native Buna) | `src/session.cpp` | Same rules as the web Buna: clock interpolated between media reports (max 2 s ahead), seek preview with 10/30/60/120 s acceleration, one seek 600 ms after the last press, committed position only after `seekDone` or a settled report, stall watchdog, startup timeout, error state, end/replay |
| Diagnostics | Home → Diagnostics | TV model/firmware, SDL driver/renderer, display and panel modes, LG SDL extensions present, last remote key with scancode, recent log lines |

Compatibility, verified on the built binary: 32-bit ARM EABI5 soft-float (`e_flags 0x5000200`, the same as the TV's own executables). It needs only `GLIBC_2.4`, no `GLIBCXX`, since the C++ runtime is linked statically. It uses only SDL 2.0.0-era functions plus the LG extensions, and those are resolved at runtime. Its libraries are `libSDL2-2.0.so.0`, `libglib-2.0.so.0`, `libhelpers.so.2` and libc.

## Build

```sh
# once: webOS NDK (openlgtv buildroot-nc4, as used by Moonlight TV)
curl -L -o /tmp/sdk.tgz https://github.com/openlgtv/buildroot-nc4/releases/download/webos-a38c582/arm-webos-linux-gnueabi_sdk-buildroot-x86_64.tar.gz
tar xzf /tmp/sdk.tgz -C /opt && /opt/arm-webos-linux-gnueabi_sdk-buildroot/relocate-sdk.sh

native/scripts/build.sh          # host unit tests → cross build → ABI guard → artifacts/com.muhib.bloob_<v>_arm.ipk
native/scripts/ui-snapshots.sh   # desktop build, scripted remote + simulated media events → 1920×1080 screenshots
```

`ares-package` comes from the web app's dev dependencies (`tv-app/node_modules`). The desktop build (`bloob_host`) runs the same UI and session code against a stand-in bus (`src/luna_host.cpp`). It drives scripted key presses and injects media events through `BLOOB_SCRIPT`. The real binary cannot run under qemu here, because qemu-user cannot run the NDK's glibc 2.12 dynamic loader. Static ARM binaries do run.

## Install and test on the TV (Windows `cmd`)

```bat
ares-install -d rootedTV com.muhib.bloob_0.3.0_arm.ipk
ares-launch -d rootedTV com.muhib.bloob
ares-shell -d rootedTV -r "cat /tmp/bloob-native.log"
```

To play any URL directly (for example a resolved link), with no catalogue yet:

```bat
ares-launch -d rootedTV com.muhib.bloob -p "{\"uri\":\"https://...\",\"title\":\"Test\",\"autoplay\":true}"
```

The log never contains full URLs: it records scheme and host only. The first TV run answers, in this order:
1. Does the window appear and does the remote reach it? (Diagnostics shows the last key and its code.)
2. Does the system bus work from a native app? (The home screen shows the TV model, otherwise an error.)
3. Does `com.webos.media/load` accept our payload, and does video appear under the UI? (Log: `load …`, `loaded mediaId=…`, `first event keys: …`, `sourceInfo …`.)
4. Do the Audio and Subtitles menus list the file's real tracks, and does selecting one change what you hear and see?

If video does not show, the log says which step failed. Options to try are a different pipeline type (`"loadType":"sim"` in the launch parameters) or a different window placement.

## Next milestones

- **N2:** catalogue, search, sources and AllDebrid. Reuse the existing service (`tv-app/service`) over `http://127.0.0.1:8787`, with artwork decoded by stb_image and cached. Port the Concept A design.
- **N3:** a Starfish ES backend, if N1 shows the URI pipeline's limits. This means demuxing ourselves and feeding LG's decoder (the approach Kodi's webOS port uses), to get Dolby Vision profile conversion, audio fallback and buffering control.

## References (read, not copied)

Moonlight TV (GPL-3.0) for the NDK, SDL hints, packaging and the exported video window. Kodi's webOS port (GPL-2.0+) for the media pipeline options, TV capability queries and screensaver handling. The webosbrew documentation for remote scancodes. No code from these projects is included. stb_truetype and stb_image are public domain (MIT alternative). Inter is under the SIL Open Font License (`third_party/fonts/Inter-LICENSE.txt`).
