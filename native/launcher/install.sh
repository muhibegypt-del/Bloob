#!/bin/sh
# Puts Bloob's home-screen takeover in place (run by Bloob at every start, as root through the
# Homebrew Channel). Idempotent: copies only what changed, restarts the watcher only if it changed.
# Adds files; deletes nothing. Opt out: touch /var/lib/bloob-home/disabled (README.md).
SRC=$(dirname "$0")
W=/var/lib/bloob-home/watch.sh
HOOK=/var/lib/webosbrew/init.d/60-bloob-home
[ -f /var/lib/bloob-home/disabled ] && { echo disabled; exit 0; }
[ -d /var/lib/webosbrew/init.d ] || { echo "no Homebrew Channel boot hooks"; exit 1; }
mkdir -p /var/lib/bloob-home
changed=0
if ! cmp -s "$SRC/watch.sh" "$W"; then cp "$SRC/watch.sh" "$W.new" && chmod 755 "$W.new" && mv "$W.new" "$W" && changed=1; fi
if ! cmp -s "$SRC/60-bloob-home" "$HOOK"; then cp "$SRC/60-bloob-home" "$HOOK.new" && chmod 755 "$HOOK.new" && mv "$HOOK.new" "$HOOK"; fi
# A new watcher replaces the running one; otherwise the hook starts it only if it is not running.
if [ "$changed" -eq 1 ]; then
  for p in /proc/[0-9]*; do
    case "$({ tr "\0" " " < $p/cmdline; } 2>/dev/null)" in "/bin/sh $W"*|"sh $W"*) kill "${p#/proc/}" 2>/dev/null ;; esac
  done
fi
"$HOOK"
echo "installed (watcher changed: $changed)"
