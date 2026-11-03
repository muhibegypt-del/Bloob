#!/usr/bin/env sh
# How smooth Bloob is on the TV (4.0): records every frame for a while, then
# prints the frame-to-frame times while something moves (as Native Tv measures
# itself on a B3: 16.7 ms is a perfect 60 Hz; "late" is over 25 ms, a dropped frame).
#   native/scripts/tv-frames.sh <TV address> [seconds, default 20]
# While it runs, move around Bloob with the remote (scroll a row, open a title).
# It only writes Bloob's own files in /tmp (/tmp/bloob-frames.on and .csv) and
# deletes nothing on the TV.
set -eu
TV=${1:?usage: tv-frames.sh <TV address> [seconds]}
SECS=${2:-20}
ssh "root@$TV" 'echo 1 > /tmp/bloob-frames.on'
echo "Recording for $SECS s: scroll around Bloob with the remote now."
sleep "$SECS"
ssh "root@$TV" 'echo 0 > /tmp/bloob-frames.on'
sleep 6  # the app writes its log every 5 s
OUT=${TMPDIR:-/tmp}/bloob-frames-$(date +%Y%m%d-%H%M%S).csv
ssh "root@$TV" 'cat /tmp/bloob-frames.csv' > "$OUT"
echo "Saved $OUT"
# Intervals of frames in a running animation (column 2, -1 otherwise).
awk -F, 'NR > 1 && $2 > 0 { print $2 }' "$OUT" | sort -n > "$OUT.sorted"
N=$(wc -l < "$OUT.sorted" | tr -d ' ')
if [ "$N" -eq 0 ]; then echo "No moving frames recorded: was Bloob open and did anything move?"; exit 0; fi
P50=$(awk -v n="$N" 'NR == int(n * 0.50) + 1 { printf "%.1f", $1 / 1000 }' "$OUT.sorted")
P95=$(awk -v n="$N" 'NR == int(n * 0.95) + 1 { printf "%.1f", $1 / 1000 }' "$OUT.sorted")
MAX=$(tail -1 "$OUT.sorted" | awk '{ printf "%.1f", $1 / 1000 }')
LATE=$(awk '$1 > 25000' "$OUT.sorted" | wc -l | tr -d ' ')
echo "Moving frames: $N, frame to frame p50 $P50 ms, p95 $P95 ms, max $MAX ms, late (over 25 ms) $LATE"
echo "Priority and the 5-second summaries are in the app log: grep -E 'priority|\[perf\]' /tmp/bloob-native.log"
rm -f "$OUT.sorted"
