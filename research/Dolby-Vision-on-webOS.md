## Dolby Vision on LG webOS: how Kodi does it, and what Bloob can do

> Research note, 16 October 2026 (public sources and code reading, not a TV measurement).
>
> **Correction (owner report, 16 October 2026): Dolby Vision in MKV works on this TV (OLED55G26LA, webOS 9.2.2).** That overrides this note's main conclusion, which rested on LG's 2022 forum reply and Jellyfin's webOS rule. The MP4/HLS remux and Starfish routes below are not needed for MKV here. 2.5.0 briefly labelled MKV Dolby Vision as HDR10 and sent the `DolbyHdrInfo` option; 2.5.1 removed both. The Kodi and LG details are kept for reference.

Short answer: the URI `com.webos.media/load` path has no documented option that makes LG's demuxer signal Dolby Vision for MKV on webOS 24. Two routes are known to work:
- **Change the container.** LG engages DV from MP4 (ISOBMFF) or MPEG-TS that carry the correct DV signalling.
- **Feed elementary streams, as Kodi does.** Kodi demuxes the file itself and passes `DolbyHdrInfo` to the Starfish pipeline.

Facts below are marked [F] with a source. Inference is marked [I].

### 1. How Kodi gets Dolby Vision on webOS

**Code.** I read xbmc master at `c7c26084` (16 Oct 2026). The old `DVDVideoCodecStarfish.cpp` no longer exists; the logic now lives in `xbmc/cores/VideoPlayer/MediaPipelineWebOS.cpp` (https://github.com/xbmc/xbmc/blob/master/xbmc/cores/VideoPlayer/MediaPipelineWebOS.cpp).

**API and transport [F].**
- It uses `StarfishMediaAPIs` from `<starfish-media-pipeline/StarfishMediaAPIs.h>` and calls `m_mediaAPIs->Load(payload, PlayerCallback, this)`, then `Feed(json)` for each packet.
- The transport is `"mediaTransportType": "BUFFERSTREAM"`, not URI.

**Load payload, abridged excerpt [F].** The payload is wrapped as `{"args":[p]}`:

```cpp
CVariant& contents = p["option"]["externalStreamingInfo"]["contents"];
if (videoHint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION) {
  contents["DolbyHdrInfo"]["encryptionType"] = videoHint.cryptoSession ? "all" : "clear";
  contents["DolbyHdrInfo"]["profileId"] = videoHint.dovi.dv_profile;
  contents["DolbyHdrInfo"]["trackType"] = videoHint.dovi.el_present_flag ? "dual" : "single";
}
p["option"]["windowId"] = winSystem->GetExportedWindowName();
contents["codec"]["video"] = "H265";          // stays H265 for DV (map: HEVC->"H265", AV1->"AV1")
contents["format"] = "RAW";
p["mediaTransportType"] = "BUFFERSTREAM";
p["option"]["transmission"]["contentsType"] = "LIVE";
p["option"]["transmission"]["trickType"] = "client-side";
p["option"]["seekMode"] = "keep-rate";
contents["esInfo"]{pauseAtDecodeTime, seperatedPTS, ptsToDecode, videoWidth/Height, videoFpsValue/Scale}
```

- **What it does not send [F].** There is no `DVHE`/`DVH1` codec string and no `vsi`, `dvConfig` or `dolbyVisionProfile` field. The 2023 original commented the allowed values: `encryptionType` "clear"/"bl"/"el"/"all", `profileId` 0–9, `trackType` "single"/"dual".
- **HDR10 metadata [F].** It goes separately through `setHdrInfo()` with `hdrType` "HDR10"/"HLG" plus `sei`/`vui`. On webOS below 5 the keys are `mediaSei`/`mediaVui`.
- **Feeding [F].** Each packet is `{"bufferAddr":"0x…","bufferSize":n,"pts":ns,"esData":1}`, which is a pointer into Kodi's own process memory.
- **Seeking [F].** It calls `setTimeToDecode` and then `sendSegmentEvent`.

**Bitstream preparation in `SetupBitstreamConverter` [F].**
- hvcC is converted to Annex-B.
- HDR10+ is always stripped ("webOS doesn't support HDR10+ and it can cause issues").
- If the file is profile 7, or the user enabled conversion, it converts to profile 8. The code reads `if (... convertDovi && hint.dovi.dv_profile == 7) { SetConvertDovi(true); dv_profile = 8; el_present_flag = false; }`.
- The conversion itself is done by **libdovi**: `dovi_convert_rpu_with_mode(rpu, 2)` in `xbmc/utils/BitstreamConverter.cpp`.
- An optional setting zeroes the L5 offsets.

**History [F].**

| Change | Commit / PR | Date | First release |
|---|---|---|---|
| Initial webOS port | PR #23092 (webosbrew/feature/webOS) | merged 6 Apr 2023 | 21.0a1-Omega |
| DV profile and FEL detection | `c3e08b0` | 2023 | 21.0a1-Omega |
| AV1 DV signalling (profile 10) | PR #23771 | Sep 2023 | 21.0b1-Omega |
| DV compatibility mode | PR #24859 | Mar 2024 | 22.0a1-Piers |
| Always convert when an enhancement layer is present | `3954197` | Aug 2025 | 22.0a2-Piers |

- The compatibility-mode commit message says: "It is necessary to playback profile 7 FEL. Otherwise, the Dolby Vision metadata is completely ignored."
- The August 2025 commit says: "webOS cannot play MEL or FEL so convert these regardless of dovi conversion setting."
- Kodi 22 is still at rc1. I found no backport of the conversion to 21.x Omega.

**Profiles:**
- Profile 5 and profile 8.1 are signalled directly [F].
- Profile 10 (AV1) is signalled [F].
- Profile 7 (FEL or MEL) must be converted to 8.1 through libdovi. The enhancement layer is discarded, so FEL detail is lost [F/I].

### 2. URI `com.webos.media/load` options

- **LG's documented mediaOption list [F].** It covers `mediaTransportType`, `option.mediaFormat.type`, `drm`, `transmission.playTime.start` and `adaptiveStreaming.*`. It has no HDR or DV field (https://webostv.developer.lge.com/develop/guides/mediaoption-parameter).
- **Open-source uMediaServer [F].** It contains `hdrType` only as an output field of `videoInfo` (`src/media_client/uMediaClient.cpp:450`). I found no input option.
- **Wider search [F].** Searching the web and GitHub for `DolbyHdrInfo` and similar keys found nothing used with URI transport.
- **Consequence [I].** `DolbyHdrInfo` is an ES-path (`externalStreamingInfo`) field. Adding it to a URI load is untested; it might work, but it is unlikely. It costs one call to try and then read `videoInfo.hdrType`. Pipeline types such as "sim", "ref" or "camera" are not documented for this purpose.
- **LG staff, Dec 2022 [F].** "Supported Dolby Vision profile IDs are 5, 6, 8, 9 and 10, and Media Player supports MPEG-2 TS and ISOBMFF." The required signalling is the DOVI Video Stream Descriptor in the PMT for TS, or a `dvcC`/`dvvC`/`dvwC` box in the sample entry for ISOBMFF. In Jan 2024 a developer asked for MKV DV support; LG replied only that it would announce updates. https://forum.webostv.developer.lge.com/t/dolby-vision-video-playback-via-media-player-app/1358
- **LG's own spec page [F].** The webOS 24 AV format page does not mention DV at all (https://webostv.developer.lge.com/develop/specifications/video-audio-240).

### 3. What other apps do

- **Jellyfin [F].** jellyfin-web (`src/scripts/browserDeviceProfile.js`, HEAD `f21682a`) contains this rule: "On WebOS 25 and newer, mp4, ts, and mkv containers are allowed. On WebOS 24 and lower, only mp4 and ts." For DV MKV on webOS 24 or older, the server therefore remuxes.
  - Jellyfin assumes profile 8 support on webOS 4 and later even though the TV does not report it.
  - The webOS app passes `deviceInfo.dolbyVision` through.
  - Jellyfin 12.0 added DV-in-MKV for webOS 25+ and a "spec-compliant dvh1 HLS variant" for profile 5 (https://jellyfin.org/posts/jellyfin-release-12.0/).
- **Universal Media Server [F].** It has remuxed DV MKV for LG on the fly since 2023 and automatically after that; a March 2024 post says it muxes to MP4.
  - A user found that FFmpeg MKV-to-TS lost DV, while tsMuxeR kept it (https://universalmediaserver.com/forum/viewtopic.php?p=48295).
  - In Dec 2025 UMS warned that the webOS 25 Media Player crashed; by 7 Oct 2026 it called this resolved (https://universalmediaserver.com/forum/viewtopic.php?p=50381).
- **Plex [F].** In 2022, "MP4 is the only thing that activates the Dolby Vision on the TV". MKV gave a green/purple picture, which is the profile 5 symptom (https://forums.plex.tv/t/solved-dolby-vision-lg-webos/799587).
- **Moonlight TV and ss4s [F].** They use the same StarfishMediaAPIs ES route (`ss4s/modules/webos/smp`, with a C wrapper `StarfishMediaAPIs_C.cpp`), but for game streams. They do not use DV.
- **FFmpeg muxers [F].**
  - Upstream `movenc.c` writes `dvcC` (profile ≤7) or `dvvC` (profile >7) only with `-strict unofficial`, and accepts `-tag:v dvh1`.
  - Upstream `mpegtsenc.c` has no DOVI descriptor. jellyfin-ffmpeg adds one in patch `0027-pass-dovi-sidedata-to-hlsenc-and-mpegtsenc.patch` (tag 0xb0).
  - The upstream `dovi_rpu` bitstream filter can only strip or compress metadata. It cannot convert profile 7 to 8.1.

**HLS on webOS 24.** LG lists HLS version 7 for webOS 22–26, which allows fMP4 segments. I found no direct LG statement that DV engages over HLS fMP4 on webOS 24 [F/I]. MP4 is the documented-safe target.

### 4. webOS 25

- **Feature [F].** Native DV in MKV, per UMS and Jellyfin's webOS ≥25 rule. A G4 owner reports profiles 5 and 8 in MKV now work, but "the new WebOS update broke remuxes with DV profile 7" (https://forum.jellyfin.org/t-jellyfin-on-lg-g4-webos25-dolby-vision).
- **Our model [F].** The 2022 B2, C2 and G2 are on the webOS 25 Re:New rollout list. Reports give the timing as late 2025 to 2026, varying by region (https://en.eloutput.com/news/Home/Many-LG-smart-TVs-are-receiving-a-critical-update-with-new-features./).
- **Our TV [I].** 9.2.2 is webOS 24. Nothing suggests the MKV DV feature was backported to webOS 24 firmware. Getting it means taking the webOS 25 firmware, which is the owner's decision; Bloob should not trigger it.

### 5. Options, ranked by effort

**(a) Load-option fix: hours, low odds [I].**
- Try `option.externalStreamingInfo.contents.DolbyHdrInfo {encryptionType:"clear", profileId:8, trackType:"single"}` on the URI load and watch `videoInfo.hdrType`.
- Read-only check on the TV: `strings` the starfish pipeline libraries for `DolbyHdrInfo` to see whether the URI path parses it. Do not expect success.

**(b) Remux on the TV to MP4 or HLS-fMP4, served over localhost: days for a proof, weeks to do properly.**
- **Proof test [I, strongly supported by LG staff, Jellyfin, UMS and Plex].**
  1. `ffmpeg -i <url> -t 60 -map 0:v -map 0:a:0 -c copy -tag:v dvh1 -strict unofficial clip.mp4`
  2. Serve it from the Node service on 127.0.0.1.
  3. Load it through the same uMediaServer URI path and check for the DV badge and `hdrType`.
- **What it needs:**
  - A static armhf ffmpeg on the TV, spawned by our service. FFmpeg 5 or later reads DV configuration from Matroska [I: version not verified].
- **Format limits:**
  - **Profiles 5 and 8.1:** a container copy is enough.
  - **Profile 7:** LG's list excludes it, so it needs RPU conversion and enhancement-layer removal. That means libdovi or dovi_tool (`-m 2 convert --discard`) in the pipe, not plain FFmpeg.
  - **Audio:** TrueHD is not in LG's MP4 audio list. DTS is listed for MP4 on webOS 24, with a model footnote.
- **Seeking:** progressive fMP4 from a pipe cannot seek, so the real design is on-demand HLS-fMP4 VOD segmenting, as Jellyfin does.
- **Unknown:** I found no published webOS 24 test of HLS-fMP4 with DV.

**(c) Starfish ES route like Kodi: weeks to months, highest control, works for MKV profile 5, 8.1 and converted profile 7 [F for Kodi].**
- **Libraries on the TV:**
  - `libplayerAPIs.so` (StarfishMediaAPIs; pkg-config `libplayerAPIs`) and the player-factory headers, so Kodi can cast to `CustomPlayer`/`CustomPipeline`.
  - `AcbAPI`, needed only without an exported window, which we already use.
- **Headers and stubs:** they come from webosbrew/webos-userland (`include/starfish-media-pipeline/StarfishMediaAPIs.h`, stub `.so` files; Kodi pins v0.0.17), https://github.com/webosbrew/webos-userland. It works with the buildroot-nc4 toolchain we already use.
- **What we would have to bring:**
  - Our own demuxer and HTTPS input: libavformat with TLS.
  - libdovi (Rust, cross-built for armv7-unknown-linux-gnueabi, as Kodi's depends do).
  - hvcC to Annex-B conversion and HDR10+ stripping.
  - Audio ES passthrough: AC3, EAC3 with `immersive:"ATMOS"`, DTS/DTSX. TrueHD would need transcoding to EAC3, as Kodi does.
  - Buffer-level control and the seek and flush protocol.
- **ABI risk [I].** The API passes `std::string` and is C++, which conflicts with our "no GLIBCXX" guard. Use a small C shim built against the TV's libstdc++, as ss4s does in `wrapper/StarfishMediaAPIs_C.cpp`. Feeding passes raw in-process pointers, so the BUFFERSTREAM path cannot be driven over luna alone.
- **Reference code for the full flow:**
  - Kodi `MediaPipelineWebOS.cpp` (Load, Feed, `SetHDR`, seek and flush)
  - Kodi `BitstreamConverter.cpp` (`processDoviRpu`)
  - Kodi `tools/depends/target/{webos-userland,libdovi}`
  - mariotaku/ss4s `modules/webos/smp` (C wrapper, `docs/load.json`)

**Recommendation [I].** Run (a) and the (b) proof clip first; together they take about a day. If the MP4 clip engages DV, build the HLS-fMP4 remux in the service for profiles 5 and 8.1. Treat profile 7 as HDR10, or add dovi_tool. Consider (c) only if seeking, startup or profile 7 quality justify a native demuxer.
