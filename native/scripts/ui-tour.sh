#!/usr/bin/env bash
# Desktop tour of the whole native app: the real Bloob service (live catalogue
# and artwork) with fixture sources and a fake AllDebrid, driven by scripted
# remote presses. Saves a PNG of each step.
#   native/scripts/ui-tour.sh [output-dir]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=$(realpath -m "${1:-build-host/tour}")
PORT=${BLOOB_TOUR_PORT:-8791}
cmake -S . -B build-host >/dev/null && cmake --build build-host -j"$(nproc)" >/dev/null
STAGE=build-host/stage
rm -rf "$STAGE" "$OUT" && mkdir -p "$STAGE/bin" "$STAGE/assets/fonts" "$OUT"
cp build-host/bloob_host "$STAGE/bin/" && cp third_party/fonts/*.ttf "$STAGE/assets/fonts/"

DATA=$(mktemp -d)
(cd ../tv-app && TV_APP_DATA_DIR="$DATA" TV_APP_PORT=$PORT BLOOB_DEV_FIXTURES=../native/tests/fixtures/dev-service.js \
  exec node service/server.js) > "$OUT/service.log" 2>&1 &
SERVICE=$!
trap 'kill $SERVICE 2>/dev/null || true' EXIT
sleep 1

SRC='{"sourceInfo":{"container":"mkv","seekable":true,"programInfo":[{"duration":8722000,"audioTrackInfo":[{"language":"eng","codec":"eac3","channels":6},{"language":"eng","codec":"truehd","channels":8},{"language":"fre","codec":"ac3","channels":6}],"subtitleTrackInfo":[{"language":"eng","type":"text"},{"language":"spa","type":"text"}],"videoTrackInfo":[{"codec":"HEVC","width":3840,"height":2160,"frameRate":23.976}]}]}}'
VI='{"videoInfo":{"width":3840,"height":2160,"frameRate":23.976,"hdrType":"DolbyVision"}}'
STEPS=(
  "wait:8000" "snap:01-home"
  "key:down" "wait:2500" "snap:02-home-rows"
  "key:ok" "wait:3500" "snap:03-details-film"
  "key:ok" "wait:1500" "snap:04-sources"
  "key:down" "key:down" "wait:400" "snap:05-sources-table"
  "key:up" "key:up" "key:ok" "wait:900" "snap:06-resolving"
  "wait:15000" "snap:07-player-starting"
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}'
  'media:{"currentTime":{"currentTime":1754000}}' "wait:300" "snap:08-player-hud"
  "key:down" "key:right" "key:right" "key:right" "key:ok" "wait:200" "snap:09-player-audio"
  "key:back" "key:back" "key:back" "wait:1500" "snap:10-back-to-details"
  "key:back" "wait:1500" "snap:11-home-continue"
  "key:left" "wait:400" "snap:12-rail"
  "key:down" "key:right" "key:right" "key:right" "key:right" "key:ok"
  "key:down" "key:down" "key:down" "key:left" "key:ok" "key:up" "key:left" "key:ok" "key:ok" "key:down" "key:right" "key:ok"
  "wait:3500" "snap:13-search"
  "key:right" "key:right" "key:right" "wait:300" "snap:14-search-results"
  "key:left" "key:left" "key:left" "key:left" "key:left" "key:left" "key:left" "key:down" "key:down" "wait:6000" "snap:15-series"
  "key:right" "key:down" "key:ok" "wait:4000" "snap:16-series-details"
  "key:down" "key:down" "key:right" "wait:1500" "snap:17-episodes"
  "key:back" "key:back" "key:back" "key:left" "key:down" "key:right" "wait:1000" "snap:18-settings"
  "quit"
)
SCRIPT=$(IFS='|'; echo "${STEPS[*]}")
SDL_VIDEODRIVER=dummy BLOOB_WINDOW=1280x720 BLOOB_SERVICE_PORT=$PORT BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$SCRIPT" \
  timeout 180 "$STAGE/bin/bloob_host" > "$OUT/run.log" 2>&1 || { echo "app exited with $?"; tail -30 "$OUT/run.log"; exit 1; }
for f in "$OUT"/*.bmp; do ffmpeg -loglevel error -y -i "$f" "${f%.bmp}.png" && rm "$f"; done
grep -E '\[(nav|sources|player|media|browse|api)\]' "$OUT/run.log" | head -60
ls "$OUT"/*.png | wc -l
