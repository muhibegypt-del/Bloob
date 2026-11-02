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
  "key:back" "key:pause" 'media:{"paused":{}}' "wait:50" "snap:08-paused" 'media:{"bufferRange":{"beginTime":0,"endTime":812000,"remainingTime":38000,"percent":42}}' "key:info" "wait:1100" "snap:08b-info-panel" "key:info"
  'media:{"error":{"errorCode":304,"errorText":"The TV could not decode this stream."}}' "wait:50" "snap:09-error" "quit"
)
SCRIPT=$(IFS='|'; echo "${STEPS[*]}")
BLOOB_EFFECTS=full SDL_VIDEODRIVER=dummy BLOOB_WINDOW=${BLOOB_WINDOW:-1280x720} BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$SCRIPT" \
  timeout 60 "$STAGE/bin/bloob_host" '{"uri":"https://example.invalid/film.mkv","title":"Test film","autoplay":true}' > "$OUT/run.log" 2>&1

# Skip intro (4.0): the service (with fixtures) knows a recap, an intro and the credits.
PORT=8797
DATA=$(mktemp -d)
(cd ../tv-app && TV_APP_DATA_DIR="$DATA" TV_APP_PORT=$PORT BLOOB_DEV_FIXTURES=../native/tests/fixtures/dev-service.js \
  exec node service/server.js) > "$OUT/skip-service.log" 2>&1 &
SVC=$!
sleep 1
SKIP_STEPS=(
  "wait:300" "pump" 'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}'
  'media:{"currentTime":{"currentTime":20000}}' "wait:800" "key:back" "wait:400" "snap:10-skip-recap"
  'media:{"currentTime":{"currentTime":620000}}' "wait:400" "snap:11-skip-intro" "key:ok" "wait:100"
  'media:{"seekDone":{}}' 'media:{"currentTime":{"currentTime":690100}}' "wait:400" "snap:12-after-skip" "quit"
)
SCRIPT=$(IFS='|'; echo "${SKIP_STEPS[*]}")
BLOOB_EFFECTS=full SDL_VIDEODRIVER=dummy BLOOB_WINDOW=${BLOOB_WINDOW:-1280x720} BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$SCRIPT" BLOOB_SERVICE_PORT=$PORT \
  timeout 60 "$STAGE/bin/bloob_host" '{"uri":"https://example.invalid/film.mkv","title":"Test film","autoplay":true}' > "$OUT/skip-run.log" 2>&1 || true
kill $SVC 2>/dev/null || true
rm -rf "$DATA"
grep -E 'skips|skipped' "$OUT/skip-run.log" || echo "skip run: no skip lines"

# Styled subtitles (4.0): a clip with an ASS track (tests/fixtures/styled.ass) through Bloob's engine.
ASSMKV="$OUT/.styled.mkv"
ffmpeg -v error -y -f lavfi -i testsrc2=s=1920x1080:r=24:d=10 -f lavfi -i sine=f=440:d=10:r=48000 -i tests/fixtures/styled.ass \
  -map 0:v -map 1:a -map 2:s -c:v libx265 -x265-params log-level=none -preset ultrafast -c:a ac3 -c:s ass -metadata:s:s:0 language=eng "$ASSMKV"
BLOOB_EFFECTS=full SDL_VIDEODRIVER=dummy BLOOB_WINDOW=${BLOOB_WINDOW:-1280x720} BLOOB_SNAPSHOT_DIR="$OUT" \
  BLOOB_SCRIPT="wait:1500|key:down|key:right|key:right|key:ok|wait:300|key:down|key:ok|wait:300|key:back|key:back|wait:4000|snap:13-styled-subtitles|quit" \
  timeout 40 "$STAGE/bin/bloob_host" "{\"uri\":\"file:$ASSMKV\",\"title\":\"Styled\",\"autoplay\":true}" > "$OUT/styled-run.log" 2>&1 || true
rm -f "$ASSMKV"
grep -E 'styled subtitles' "$OUT/styled-run.log" || echo "styled run: no styled subtitles"
# Fill the transparent video hole with a still from the bundled clip.
STILL="$OUT/.still.png"
ffmpeg -loglevel error -y -ss 17 -i ../tv-app/service/www/media/big-buck-bunny-480p-30sec.mp4 -frames:v 1 -vf scale=1920:1080 "$STILL"
for f in "$OUT"/*.bmp; do
  ffmpeg -loglevel error -y -i "$STILL" -i "$f" -filter_complex "[1:v]format=rgba[ui];[0:v][ui]overlay" "${f%.bmp}.png" && rm "$f"
done
rm -f "$STILL"
grep -E '\[media\]|\[player\]' "$OUT/run.log"
ls "$OUT"/*.png | wc -l
