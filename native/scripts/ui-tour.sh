#!/usr/bin/env bash
# Desktop tour of the whole native app: the real Bloob service (live catalogue
# and artwork) with fixture sources and a fake AllDebrid, driven by scripted
# remote presses. Follows the acceptance task: pick an episode, understand the
# best match, start it, change a track, return to browsing. Saves a 1920×1080
# PNG of each state.
#   native/scripts/ui-tour.sh [output-dir]
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=$(realpath -m "${1:-build-host/tour}")
PORT=${BLOOB_TOUR_PORT:-8791}
cmake -S . -B build-host >/dev/null && cmake --build build-host -j"$(nproc)" >/dev/null
STAGE=build-host/stage
rm -rf "$STAGE" "$OUT" && mkdir -p "$STAGE/bin" "$STAGE/assets/fonts" "$OUT"
cp build-host/bloob_host "$STAGE/bin/" && cp third_party/fonts/*.ttf "$STAGE/assets/fonts/"

run() {  # run <name> <unpaired 0|1> <steps...>
  local name=$1 unpaired=$2
  shift 2
  local data
  data=$(mktemp -d)
  (cd ../tv-app && TV_APP_DATA_DIR="$data" TV_APP_PORT=$PORT BLOOB_DEV_FIXTURES=../native/tests/fixtures/dev-service.js \
    BLOOB_DEV_UNPAIRED=$([ "$unpaired" = 1 ] && echo 1 || echo "") exec node service/server.js) > "$OUT/$name-service.log" 2>&1 &
  local service=$!
  sleep 1
  local script
  script=$(IFS='|'; echo "$*")
  SDL_VIDEODRIVER=dummy BLOOB_WINDOW=1280x720 BLOOB_SERVICE_PORT=$PORT BLOOB_SNAPSHOT_DIR="$OUT" BLOOB_SCRIPT="$script" \
    timeout 240 "$STAGE/bin/bloob_host" > "$OUT/$name.log" 2>&1 || { echo "$name: app exited with $?"; tail -30 "$OUT/$name.log"; }
  kill $service 2>/dev/null || true
  wait $service 2>/dev/null || true
}

SRC='{"sourceInfo":{"container":"mkv","seekable":true,"programInfo":[{"duration":3125000,"audioTrackInfo":[{"language":"eng","codec":"eac3","channels":6},{"language":"eng","codec":"truehd","channels":8},{"language":"fre","codec":"ac3","channels":6}],"subtitleTrackInfo":[{"language":"eng","type":"text"},{"language":"spa","type":"text"}],"videoTrackInfo":[{"codec":"HEVC","width":3840,"height":2160,"frameRate":23.976}]}]}}'
VI='{"videoInfo":{"width":3840,"height":2160,"frameRate":23.976,"hdrType":"DolbyVision"}}'

# The acceptance task, start to finish.
run task 0 \
  "wait:250" "snap:00-loading" \
  "wait:8000" "snap:01-home" \
  "key:right" "wait:300" "snap:02-home-focus-moves" "key:left" \
  "key:down" "wait:2200" "snap:03-home-shelves" \
  "key:right" "key:right" "wait:1600" "snap:04-home-shelf-follows" \
  "key:down" "wait:1600" "snap:05-home-series-shelf" \
  "key:ok" "wait:4500" "snap:06-details-series" \
  "key:down" "wait:1500" "snap:07-seasons-episodes" \
  "key:down" "key:right" "wait:900" "snap:08-episode-focused" \
  "key:ok" "wait:2000" "snap:09-sources" \
  "key:down" "wait:500" "snap:10-sources-other" \
  "key:right" "wait:600" "snap:11-sources-release-details" \
  "key:left" "key:up" "wait:300" "key:ok" "wait:1200" "snap:12-starting" \
  "wait:14000" "snap:13-player-starting" \
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}' \
  'media:{"currentTime":{"currentTime":754000}}' "wait:500" "snap:14-player-hud" \
  "key:right" "key:right" "key:right" "wait:120" "snap:15-player-seek-preview" \
  "wait:800" 'media:{"seekDone":{}}' \
  "key:down" "key:right" "key:ok" "wait:500" "snap:16-player-audio" \
  "key:down" "key:ok" 'media:{"trackSelected":{"type":"audio","index":1}}' "wait:300" "snap:17-player-audio-changed" \
  "key:back" "wait:2000" 'media:{"currentTime":{"currentTime":786000}}' "wait:2000" 'media:{"currentTime":{"currentTime":788000}}' \
  "wait:2000" 'media:{"currentTime":{"currentTime":790000}}' "window:focus-lost" 'media:{"currentTime":{"currentTime":790400}}'  "wait:400" "snap:17a-player-under-quick-menu" "window:focus-gained" \
  'media:{"unloadCompleted":{}}' "wait:900" "snap:17b-player-reconnecting" \
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}' "wait:300" \
  "window:hidden" 'media:{"unloadCompleted":{}}' "wait:300" "window:shown" "wait:300" \
  'media:{"loadCompleted":{}}' "media:$SRC" "media:$VI" 'media:{"playing":{}}' "wait:300" \
  'media:{"endOfStream":{}}' "wait:600" "snap:18-player-up-next" \
  "key:back" "key:back" "key:back" "wait:1500" "snap:19-back-to-details" \
  "key:back" "wait:1500" "snap:20-back-to-home" \
  "key:up" "key:up" "key:up" "key:up" "wait:400" "snap:21-tabs" \
  "key:left" "key:left" "key:down" "key:right" "key:right" "key:right" "key:ok" "key:down" "key:down" "key:down" "key:ok" \
  "key:up" "key:up" "key:left" "key:ok" "wait:3000" "snap:22-search" \
  "key:right" "key:right" "key:right" "key:right" "wait:500" "snap:23-search-results" \
  "quit"

# Film path, pairing and failure states.
run states 1 \
  "wait:8000" "key:down" "wait:1500" "key:ok" "wait:4000" "snap:30-details-film" \
  "key:ok" "wait:2000" "key:ok" "wait:1500" "snap:31-pairing" \
  "key:back" "wait:300" "key:back" "key:back" "wait:800" \
  "key:up" "key:up" "key:up" "wait:300" "key:right" "key:right" "key:right" "wait:800" "snap:32-settings" \
  "quit"

# Player shots have a transparent hole where the TV's video plane shows through;
# fill it with a still from the bundled test clip so the overlay reads correctly.
STILL="$OUT/.still.png"
ffmpeg -loglevel error -y -ss 17 -i ../tv-app/service/www/media/big-buck-bunny-480p-30sec.mp4 -frames:v 1 -vf scale=1920:1080 "$STILL"
for f in "$OUT"/*.bmp; do
  case "$f" in
    *player*) ffmpeg -loglevel error -y -i "$STILL" -i "$f" -filter_complex "[1:v]format=rgba[ui];[0:v][ui]overlay" "${f%.bmp}.png" ;;
    *) ffmpeg -loglevel error -y -i "$f" "${f%.bmp}.png" ;;
  esac
  rm "$f"
done
rm -f "$STILL"
grep -hE '\[(nav|sources|player|browse|api|app|media)\]' "$OUT"/task.log | grep -v '\[media\] <-' | head -60
ls "$OUT"/*.png | wc -l
