#!/usr/bin/env bash
# Read-only: list which Access Control Groups the TV's own permission tables give
# the luna methods Bloob calls, so appinfo.json's requiredACG can name real groups
# instead of guesses. Reads files only; changes nothing on the TV.
#   native/scripts/acg-lookup.sh [user@host]      (default root@192.168.1.26)
set -euo pipefail
TV=${1:-root@192.168.1.26}
ssh "$TV" 'for f in /usr/share/luna-service2/api-permissions.d/*.json /etc/luna-service2/api-permissions.d/*.json; do
  [ -f "$f" ] || continue
  grep -l -E "com\.webos\.media|tv\.systemproperty|connectionmanager|settingsservice|com\.webos\.service\.tts|com\.lge\.service\.tts|applicationmanager" "$f" 2>/dev/null
done | sort -u | while read -r f; do echo "== $f"; grep -E "\"[a-z0-9.]+\"[[:space:]]*:|com\.webos\.media/|systemproperty/|connectionmanager/|settingsservice/|tts/" "$f" | head -60; done'
