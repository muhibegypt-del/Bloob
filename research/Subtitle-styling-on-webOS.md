## Report: subtitle size and styling for uMediaServer "media" pipeline playback on webOS 24

> Research note, 16 October 2026 (public sources, not a TV measurement). Acted on in 2.5.0: Settings > Subtitle size and Subtitle background send these calls after every enable.

Before 2.5.0 Bloob sent only `selectTrack`, `setSubtitleEnable` and `setSubtitlePosition` (`native/src/media.cpp`).

### 1. The luna methods and their payloads

**Verified from source code**
- Current open-source uMediaServer `master` and the 2024 `@gav` branch do not register any `setSubtitle*` luna method. `src/server/uMediaserver.cpp` registers only load/play/pause/seek/subscribe and similar.
- The early tag `submissions/1` (2018) registers 17 subtitle methods on `com.webos.media`, with Doxygen parameter tables. Every payload needs `"mediaId"`, and the reply is `{returnValue, mediaId}`.
- Current `src/pipeline/Pipeline.cpp` still forwards each command to the pipeline process as `<pipelineId>/setSubtitleX` with the same argument key. If the pipeline is not yet `PIPELINE_MEDIA_LOADED`, it queues the command and replays it after load.
- Sources: https://github.com/webosose/umediaserver (tag `submissions/1`, `src/server/uMediaserver.cpp` lines 197–213 and 1529–2449; `src/pipeline/Pipeline.cpp` around lines 400–460 and 1136; `include/public/uMediaClient.h`).
- The OSE reference page https://www.webosose.org/docs/reference/ls2-api/com-webos-media/ documents none of these methods.

Value meanings come from a community gist (https://gist.github.com/aabytt/bddbb1bcf031a050d89a89aeee3a6737), and Stremio's shipping retail-TV code agrees with them. They are not official.

| Method | Payload beyond `mediaId` | Values |
|---|---|---|
| `setSubtitleFontSize` | `"fontSize": int` | 0 tiny, 1 small, 2 standard (default), 3 large, 4 huge. These are levels, not points or percent. |
| `setSubtitleColor` | `"color": int` | 0 yellow, 1 red, 2 white, 3 green, 4 blue, 5 gray |
| `setSubtitleCharacterColor` | `"charColor": string` | black, white, yellow, red, green, blue |
| `setSubtitleCharacterOpacity` | `"charOpacity": int` | 0–255 (255 is the default) |
| `setSubtitleBackgroundColor` | `"bgColor": string` | same colour names |
| `setSubtitleBackgroundOpacity` | `"bgOpacity": int` | 0–255 (0 is the default, meaning no box) |
| `setSubtitleWindowColor` / `setSubtitleWindowOpacity` | `"windowColor": string` / `"windowOpacity": int` | colour names / 0–255 |
| `setSubtitlePosition` | `"position": int` | −3 (lowest) to 4 (highest), 0 is the default |
| `setSubtitleSync` | `"sync": int` | offset in ms, positive or negative |
| `setSubtitleEncoding` | `"encoding": string` | for example `UTF-8`, `WINDOWS-1251`, `BIG5` |
| `setSubtitleSource` | `"uri"`, optional `"preferredEncodings"` | external file, http or `file:///` |
| `setSubtitleCharacterFontSize` | `"charFontSize": string` | **unknown** |
| `setSubtitleCharacterFont` | `"charFont": string` | **unknown** |
| `setSubtitleCharacterEdge` | `"charEdgeType": string` | **unknown** |
| `setSubtitlePresentationMode` | `"presentationMode": string` | **unknown** |

My inference about the four unknown methods: they look like CEA-708 caption styling. Their names match the system caption keys (`captionSize` small/standard/large, `captionFont` font0–7, `captionEdgeType` none/uniform/raised/depressed/left_drop_shadow/right_drop_shadow). They may not affect MKV text subtitles at all.

### 2. Whether these work on retail LG TVs

**Verified: shipping apps use them on retail TVs**
- Stremio's `src/WebOsVideo/WebOsVideo.js` (commit 61141ee, 14 Oct 2026, https://github.com/Stremio/stremio-video) sends `setSubtitleFontSize {mediaId, fontSize}` with 0–4 and a default of 1.
  - It also sends `setSubtitleCharacterColor`, `setSubtitleBackgroundColor` with `setSubtitleBackgroundOpacity` (0 or 255), `setSubtitlePosition` and `setSubtitleCharacterOpacity` (percent × 2.55).
  - It chose `setSubtitleCharacterColor` over `setSubtitleColor` so text and background use the same colour names.
  - It applies all styles after `setSubtitleEnable(true)` succeeds, then waits 500 ms before `selectTrack {type:"text"}`.
  - Its `mediaId` comes from the HTML `<video>` element, but it is the same uMediaServer pipeline.
- NuvioTVSmart, on an LG C5 running webOS TV 11.2 (https://github.com/NuvioMedia/NuvioTVSmart/issues/980), found the TV may drop the size when subtitles are re-enabled. Its fix re-sends `setSubtitleFontSize` after every `setSubtitleEnable(true)`; in a retest, 46 of 46 re-enables were followed by the size reaching the platform.

**Main risk: the smallest level may still be too big**
- Nuvio issue #1002 (https://github.com/NuvioMedia/NuvioTVSmart/issues/1002) estimated native line pitch on a 1080 canvas from phone photos: level 0 ≈ 74 px, level 1 ≈ 91 px, level 2 ≈ 104 px, level 3 ≈ 110 px, level 4 ≈ 130 px.
- Their conclusion is that even level 0 is larger than a typical app-drawn subtitle.
- User reports agree: Plex users had to pick "Tiny" (https://forums.plex.tv/t/lg-tv-webos-subtitle-size/758125), and Lampa issue #274 called them "annoyingly huge", with "small" still too large (https://github.com/yumata/lampa-source/issues/274).

**TV system caption settings**
- LG documents `luna://com.webos.settingsservice/getSystemSettings {category:"caption", keys:[…]}` (https://webostv.developer.lge.com/develop/references/settings-service).
- The keys are `captionEnable`, `captionAnalog`, `captionDigital`, `captionStyle` (auto/manual), `captionSize` (small/standard/large), `captionFont`, `captionTextColor`, `captionTextOpacity`, `captionBgColor`, `captionBgOpacity`, `captionEdgeType`, `captionEdgeColor`, `captionWindowColor` and `captionWindowOpacity`.
- No source says they style embedded subtitles in uMediaServer playback. My inference is that they are for broadcast CC and do not apply.
- A read-only `getSystemSettings` call on the TV costs nothing if you want to confirm.

### 3. Subtitle options in the load payload

None documented. LG's `mediaOption` reference (https://webostv.developer.lge.com/develop/guides/mediaoption-parameter) lists only:
- `mediaTransportType`;
- `option.mediaFormat.type`;
- `option.drm.{type, clientId}`;
- `option.transmission.playTime.start`;
- `option.adaptiveStreaming.{audioOnly, adaptiveResolution, seamlessPlay, maxWidth, maxHeight, bps.start}`.

The OSE `load` `option` has no subtitle fields either. Styling is only possible through the per-media `setSubtitle*` calls after load.

### 4. Drawing subtitles ourselves

**Verified:** uMediaServer's client library parses a pipeline event named `subtitleData` with `{subtitleData: string, presentationTime: uint64}` (`src/media_client/uMediaClient.cpp` line 657, `onSubtitleData` in `uMediaClient.h`). The OSE `umc` tool prints it.

**Unknown:**
- whether the retail "media" pipeline ever emits it, and under what condition;
- what `presentationMode` value, if any, switches from the TV drawing subtitles to sending text to the app.

I found no retail report of it. The cheap test: Bloob's `subscribe` handler should log any event key it doesn't recognise (only key names and `presentationTime`, not text) while a text track is selected. Optionally try `setSubtitlePresentationMode` with guessed values; this is speculative.

**Proven elsewhere: extract the track yourself and draw it**
- Nuvio's embedded-subtitle path does this on retail LG (issue #1002 describes it).
  - Its Node service reads the MKV in windows of about 120 s, following cue entries to byte offsets.
  - It converts the text to VTT cues and draws them in its own overlay.
  - It falls back to the native renderer on repeated failure.
- A Lampa plugin claims the same for MKV/MP4.
- Kodi on webOS feeds Starfish its own demuxed streams and has its own subtitle parser and overlay classes (https://docs.kodi.tv/db/dca/class_c_d_v_d_video_codec_starfish.html). That it draws subtitles itself is my inference from its documented classes, not stated.

**My feasibility assessment for Bloob:** good for text tracks (SRT, ASS, SSA, plain text).
- The bundled Node service already has the HTTP URL. It can parse the EBML header, Tracks, SeekHead and Cues with range reads, then fetch only the clusters around the playhead and pull out the subtitle track's blocks. Timestamps come from cluster time plus block time.
- Cost: subtitle blocks are scattered through the file, and some files' Cues index only video. In that case the service has to walk cluster headers in the window ahead of the playhead.
- Prefetch about 2 minutes ahead, and re-window on seek.
- Draw in the native UI layer, synced to the uMediaServer `currentTime`, and call `setSubtitleEnable false` so the TV doesn't draw a second copy.
- Image subtitles (PGS, VobSub) are not practical this way; leave those to the TV's renderer. ASS styling would need a subset renderer or stripping tags; Nuvio handles `{\anN}` positioning.
- External SRT (OpenSubtitles or `setSubtitleSource`) also works. It needs an API key and may not be timed to the release; `setSubtitleSync` can shift TV-drawn subtitles.
- Server-side ffmpeg is not available on the TV, so it is not an option.

### Recommendation

1. Add `setSubtitleFontSize` 0–4 to media.cpp now, defaulting to 0 or 1.
   - Also add `setSubtitleCharacterColor`, `setSubtitleBackgroundColor` with `setSubtitleBackgroundOpacity`, and `setSubtitleCharacterOpacity`.
   - Re-send them after every `setSubtitleEnable(true)`, after `selectTrack`, and after a pipeline reopen.
2. Test on the TV whether level 0 is small enough.
3. If it isn't, build our own text-subtitle renderer fed by MKV range extraction in the service, keeping the TV's renderer for image subtitles.

Everything about value meanings rests on the gist and on third-party apps, not on LG documentation. Verify on webOS 9.2.2.

Other sources: https://forums.plex.tv/t/lg-tv-webos-subtitle-size/758125, https://www.webosbrew.org/pages/luna-service-bus.html
