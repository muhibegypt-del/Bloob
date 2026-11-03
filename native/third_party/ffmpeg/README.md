# FFmpeg for the audio bridge

`bin/ffmpeg` and `bin/ffprobe` are FFmpeg 7.1.1 (tag `n7.1.1`, https://github.com/FFmpeg/FFmpeg),
built by `native/scripts/build-ffmpeg.sh` for the TV (ARMv7, cortex-a9, static, stripped).

- **Licence:** LGPL 2.1 or later. The build enables no GPL or non-free parts and no external
  libraries. The complete corresponding source is FFmpeg's `n7.1.1` tag; the script holds the
  exact configure line, so anyone can rebuild identical binaries from it.
- **Contents:** Matroska, MP4 and MPEG-TS input over plain HTTP from Bloob's own service; DTS,
  Dolby Digital (Plus), TrueHD, AAC, FLAC and PCM decoders; Dolby Digital (Plus) encoders;
  Matroska output. Video and subtitles are copied, never re-encoded.
- **Use:** `tv-app/service/bridge.js` runs them; `native/scripts/build.sh` copies them into the
  service's `bin/` in the IPK.
