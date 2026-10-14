#!/usr/bin/env bash
# Player check on the desktop build: launches straight into playback (as the
# TV launch parameters do), injects simulated TV media events and saves
# 1920×1080 screenshots of each player state. For the whole app, see ui-tour.sh.
#   native/scripts/ui-snapshots.sh [output-dir]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=$(realpath -m "${1:-build-host/shots}")
cmake -S . -B build-host >/dev/null && cmake --build build-host -j"$(nproc)" >/dev/null
./build-host/native_tests
STAGE=build-host/stage
rm -rf "$STAGE" "$OUT" && mkdir -p "$STAGE/bin" "$STAGE/assets/fonts" "$OUT"
cp build-host/bloob_host "$STAGE/bin/" && cp third_party/fonts/*.ttf "$STAGE/assets/fonts/"

SRC='{"sourceInfo":{"container":"mkv","seekable":true,"programInfo":[{"duration":3125000,"audioTrackInfo":[{"language":"eng","codec":"eac3","channels":6},{"language":"eng","codec":"truehd","channels":8},{"language":"fre","codec":"ac3","channels":6}],"subtitleTrackInfo":[{"language":"eng","type":"text"},{"language":"spa","type":"text"}],"videoTrackInfo":[{"codec":"HEVC","width":3840,"height":2160,"frameRate":23.976}]}]}}'
VI='{"videoInfo":{"width":3840,"height":2160,"frameRate":23.976,"hdrType":"DolbyVision"}}'
STEPS=(
  "wait:300" "pump" "snap:01-starting"
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}'
  'media:{"currentTime":{"currentTime":754000}}' "wait:150" "snap:02-playing-hud"
  "key:right" "key:right" "key:right" "wait:50" "snap:03-seek-preview" "wait:700" "snap:04-seeking"
  'media:{"seekDone":{}}' "wait:50" "key:down" "key:right" "key:ok" "wait:300" "snap:05-audio-menu"
  "key:down" "key:ok" "wait:300" "key:right" "key:ok" "wait:300" "key:down" "key:ok" "wait:300" "snap:06-subtitles-on"
  'media:{"trackSelected":{"type":"audio","index":1}}' "key:left" "key:ok" "wait:50" "snap:07-audio-selected"
  "key:back" "key:pause" 'media:{"paused":{}}' "wait:50" "snap:08-paused"
  'media:{"error":{"errorCode":304,"errorText":"The TV could not decode this stream."}}' "wait:50" "snap:09-error" "quit"
)
SCRIPT=$(IFS='|'; echo "${STEPS[*]}")
BLOOB_EFFECTS=full SDL_VIDEODRIVER=dummy BLOOB_WINDOW=${BLOOB_WINDOW:-1280x720} BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$SCRIPT" \
  timeout 60 "$STAGE/bin/bloob_host" '{"uri":"https://example.invalid/film.mkv","title":"Test film","autoplay":true}' > "$OUT/run.log" 2>&1
# Fill the transparent video hole with a still from the bundled clip.
STILL="$OUT/.still.png"
ffmpeg -loglevel error -y -ss 17 -i ../tv-app/service/www/media/big-buck-bunny-480p-30sec.mp4 -frames:v 1 -vf scale=1920:1080 "$STILL"
for f in "$OUT"/*.bmp; do
  ffmpeg -loglevel error -y -i "$STILL" -i "$f" -filter_complex "[1:v]format=rgba[ui];[0:v][ui]overlay" "${f%.bmp}.png" && rm "$f"
done
rm -f "$STILL"
grep -E '\[media\]|\[player\]' "$OUT/run.log"
ls "$OUT"/*.png | wc -l
