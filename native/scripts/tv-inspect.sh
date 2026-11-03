#!/usr/bin/env bash
# Read-only: print Bloob's runtime snapshot from the TV (R5d). Bloob writes
# /tmp/bloob-state.json at most once a second while it runs: frames and their
# stages, wakes, worker queues, GPU budget by class, artwork pipeline counters,
# process memory, the service's memory. No URLs, keys or release names.
# Reads one file; changes nothing on the TV.
#   native/scripts/tv-inspect.sh [user@host] [--watch]   (default root@192.168.1.26)
set -euo pipefail
TV=${1:-root@192.168.1.26}
WATCH=${2:-}
show() {
  ssh "$TV" 'cat /tmp/bloob-state.json 2>/dev/null || echo "{\"error\":\"Bloob is not running, or has not written a snapshot yet\"}"' |
    (command -v python3 >/dev/null && python3 -m json.tool || cat)
}
if [ "$WATCH" = "--watch" ]; then
  while true; do clear; show; sleep 2; done
else
  show
fi
