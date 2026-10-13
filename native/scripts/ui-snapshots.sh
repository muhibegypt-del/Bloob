#!/usr/bin/env bash
# Drive the desktop build of the native UI with scripted remote presses and
# simulated TV media events, saving 1920×1080 screenshots of each state.
#   native/scripts/ui-snapshots.sh [output-dir]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=${1:-build-host/shots}
cmake -S . -B build-host >/dev/null && cmake --build build-host -j"$(nproc)" >/dev/null
STAGE=build-host/stage
rm -rf "$STAGE" && mkdir -p "$STAGE/bin" "$STAGE/assets/fonts" "$STAGE/assets/media" "$OUT"
cp build-host/bloob_host "$STAGE/bin/" && cp third_party/fonts/*.ttf "$STAGE/assets/fonts/"

SRC='{"sourceInfo":{"container":"mkv","seekable":true,"programInfo":[{"duration":3125000,"audioTrackInfo":[{"language":"eng","codec":"eac3","channels":6},{"language":"eng","codec":"truehd","channels":8},{"language":"fre","codec":"ac3","channels":6}],"subtitleTrackInfo":[{"language":"eng","type":"text"},{"language":"spa","type":"text"}],"videoTrackInfo":[{"codec":"HEVC","width":3840,"height":2160,"frameRate":23.976}]}]}}'
VI='{"videoInfo":{"width":3840,"height":2160,"frameRate":23.976,"hdrType":"DolbyVision"}}'
STEPS=(
  "snap:01-home" "key:right" "key:right" "key:ok" "wait:100" "snap:02-diagnostics" "key:back"
  "key:left" "key:left" "key:ok" "wait:50" "pump" "snap:03-loading"
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}'
  'media:{"currentTime":{"currentTime":754000}}' "wait:150" "snap:04-playing-hud"
  "key:right" "key:right" "key:right" "wait:50" "snap:05-seek-preview" "wait:700" "snap:06-seeking"
  'media:{"seekDone":{}}' "wait:50" "key:down" "key:right" "key:right" "key:ok" "wait:50" "snap:07-audio-menu"
  "key:down" "key:ok" "key:right" "key:ok" "key:down" "key:ok" "wait:50" "snap:08-after-subs"
  'media:{"trackSelected":{"type":"audio","index":1}}' "key:left" "key:ok" "wait:50" "snap:09-audio-selected"
  "key:back" "key:pause" 'media:{"paused":{}}' "wait:50" "snap:10-paused"
  'media:{"error":{"errorCode":304,"errorText":"The TV could not decode this stream."}}' "wait:50" "snap:11-error" "quit"
)
SCRIPT=$(IFS='|'; echo "${STEPS[*]}")
SDL_VIDEODRIVER=dummy BLOOB_WINDOW=${BLOOB_WINDOW:-1280x720} BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$SCRIPT" \
  timeout 30 "$STAGE/bin/bloob_host" > "$OUT/run.log" 2>&1
grep -E '\[media\]|\[luna\] host call' "$OUT/run.log"
ls "$OUT"/*.bmp | wc -l
