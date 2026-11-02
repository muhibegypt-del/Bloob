#!/bin/sh
# Bloob 4.0: Bloob *is* LG Home, so the Home button and power-on open it
# directly (no LG Home flash first). Run as root, from a computer:
#   native/launcher/setup-tv.sh <TV> --takeover      (copies this and enables it)
#   sh /var/lib/bloob-home/takeover.sh enable|disable|status
# Method adapted from Luma's Home integration (ecklf/lg-luma, MIT, see LICENSE-luma),
# the manifest method it uses on webOS 9 to 11:
#   1. stage /var/lib/bloob-home/app: Bloob's files (links) and Bloob's appinfo.json
#      with the id com.webos.app.home, hidden, in the "home" category;
#   2. bind-mount it over /usr/palm/applications/com.webos.app.home (LG's files stay
#      untouched underneath);
#   3. block and release Home so the application manager reads the manifest again,
#      and check it did;
#   4. stop LG's Home process and open Home: Bloob must be in front within 15 s.
# Safety, so the TV is never stuck:
#   - any step failing undoes the others (unmount, re-read, LG Home opened);
#   - a restart always drops the mount; the boot hook mounts it again only while
#     enabled, and not after two boots in a row in which Bloob never came up as
#     Home (Bloob marks each good start in its data folder);
#   - nothing is deleted on the TV; disable leaves the files, marked off.
# Bloob's watcher (watch.sh) still handles a TV that resumes an HDMI input.
export PATH="/bin:/sbin:/usr/bin:/usr/sbin:$PATH"
HOME_ID=com.webos.app.home
BLOOB=com.muhib.bloob
STATE=${BLOOB_TAKEOVER_STATE:-/var/lib/bloob-home}
STAGE=$STATE/app
TARGET=${BLOOB_TAKEOVER_TARGET:-/usr/palm/applications/$HOME_ID}
APPS=${BLOOB_APPS:-/media/developer/apps/usr/palm/applications}
APP=$APPS/$BLOOB
DATA=${BLOOB_DATA:-/media/internal/com.muhib.bloob}
HOOKDIR=${BLOOB_HOOK_DIR:-/var/lib/webosbrew/init.d}
HOOK=$HOOKDIR/62-bloob-takeover
LUNA=${BLOOB_LUNA_SEND:-luna-send}
mkdir -p "$STATE"
log() { echo "$(date '+%Y-%m-%d %H:%M:%S') $*" >> "$STATE/takeover.log"; echo "$*"; }
am() { $LUNA -n 1 -w 4000 "luna://com.webos.applicationManager/$1" "$2" 2>/dev/null; }
ok() { printf '%s' "$1" | grep -q '"returnValue"[[:space:]]*:[[:space:]]*true'; }

# Mounted when the target and the stage are the same directory (same device and inode).
# BLOOB_TAKEOVER_FAKE (desktop tests): a marker file stands in for the mount.
if [ -n "${BLOOB_TAKEOVER_FAKE:-}" ]; then
  mounted() { [ -f "$STATE/.fake-mounted" ]; }
  foreign() { [ -f "$STATE/.fake-foreign" ]; }
  do_mount() { touch "$STATE/.fake-mounted"; }
  do_umount() { [ -n "${BLOOB_FAKE_UMOUNT_FAILS:-}" ] && return 1; rm -f "$STATE/.fake-mounted"; }
else
  mounted() { [ "$(stat -c %d:%i "$STAGE" 2>/dev/null)" = "$(stat -c %d:%i "$TARGET" 2>/dev/null)" ] && [ -d "$STAGE" ]; }
  foreign() { ! mounted && grep -q " $TARGET[ /]" /proc/self/mountinfo 2>/dev/null; }
  do_mount() { mount --bind "$STAGE" "$TARGET"; }
  do_umount() { umount "$TARGET" 2>/dev/null || umount -l "$TARGET"; }
fi
foreground() { am getForegroundAppInfo '{}' | sed -n 's/.*"appId"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1; }

reread() {  # the application manager reads Home's manifest again; $1: "bloob" or "lg"
  payload="{\"id\":\"$HOME_ID\"}"
  grep -q '"forceBlockSystemApp"' /etc/palm/schemas/sam/applicationManager.blockSystemApp.schema 2>/dev/null &&
    payload="{\"id\":\"$HOME_ID\",\"forceBlockSystemApp\":false}"
  b=$(am blockSystemApp "$payload")
  ok "$b" || printf '%s' "$b" | grep -qi 'already.*blocked' || log "blockSystemApp: $b"
  for i in 1 2 3; do
    r=$(am releaseBlockSystemApp "{\"id\":\"$HOME_ID\"}")
    if ok "$r" || printf '%s' "$r" | grep -qi 'not.*blocked'; then break; fi
    sleep 1
  done
  for i in $(seq 1 15); do
    info=$(am getAppInfo "{\"id\":\"$HOME_ID\"}")
    case "$1" in
      bloob) printf '%s' "$info" | grep -q '"main"[[:space:]]*:[[:space:]]*"[^"]*bin/bloob"' && return 0 ;;
      lg) printf '%s' "$info" | grep -q '"main"[[:space:]]*:[[:space:]]*"[^"]*bin/bloob"' || return 0 ;;
    esac
    sleep 1
  done
  log "the application manager did not read the $1 Home manifest"
  return 1
}

stop_home() {  # LG's Home process, or Bloob running as Home: asked to stop (never anything else)
  for p in /proc/[0-9]*; do
    cmd=$({ tr '\0' ' ' < "$p/cmdline"; } 2>/dev/null)
    case "$cmd" in
      "/usr/bin/$HOME_ID"*|*"flutter-client"*"-i $HOME_ID"*|*"bin/bloob "*"nid"*"$HOME_ID"*) kill -TERM "${p#/proc/}" 2>/dev/null ;;
    esac
  done
  sleep 2
}

stage() {
  [ -x "$APP/bin/bloob" ] || { log "Bloob is not installed at $APP"; return 1; }
  mkdir -p "$STAGE"
  for f in bin assets icon.png; do ln -sfn "$APP/$f" "$STAGE/$f"; done
  # Bloob's manifest as Home's: same program, Home's id.
  sed -e "s/\"id\"[[:space:]]*:[[:space:]]*\"$BLOOB\"/\"id\": \"$HOME_ID\"/" \
      -e 's/"title"[[:space:]]*:[[:space:]]*"[^"]*"/"title": "Home"/' \
      -e 's/"visible"[[:space:]]*:[[:space:]]*true/"visible": false/' \
      "$APP/appinfo.json" > "$STAGE/appinfo.json.new"
  grep -q '"visible"' "$STAGE/appinfo.json.new" || sed -i 's/^{/{ "visible": false,/' "$STAGE/appinfo.json.new"
  grep -q '"appCategory"' "$STAGE/appinfo.json.new" || sed -i 's/^{/{ "appCategory": ["home"],/' "$STAGE/appinfo.json.new"
  grep -q "\"$HOME_ID\"" "$STAGE/appinfo.json.new" || { log "could not write the Home manifest"; return 1; }
  mv "$STAGE/appinfo.json.new" "$STAGE/appinfo.json"
  echo "$BLOOB" > "$STAGE/.bloob-home"
}

restore() {
  if mounted; then do_umount; fi
  reread lg
  stop_home
  am launch "{\"id\":\"$HOME_ID\"}" >/dev/null
  log "LG Home is back"
}

apply() {  # $1: "open" to bring Home to the front
  foreign && { log "another tool is mounted over LG Home: leaving it"; return 1; }
  stage || return 1
  front=$(foreground)
  if ! mounted; then do_mount || { log "could not mount over LG Home"; return 1; }; fi
  mounted || { log "the mount did not take"; return 1; }
  reread bloob || { restore; return 1; }
  stop_home
  if [ "$1" = open ] || [ "$front" = "$HOME_ID" ]; then
    for i in $(seq 1 15); do
      { [ "$i" -eq 1 ] || [ "$i" -eq 7 ]; } && am launch "{\"id\":\"$HOME_ID\"}" >/dev/null
      [ "$(foreground)" = "$HOME_ID" ] && { log "Bloob is Home"; return 0; }
      sleep 1
    done
    log "Bloob did not come to the front as Home: undoing"
    restore
    return 1
  fi
}

status() {
  st=$(cat "$STATE/takeover" 2>/dev/null || echo off)
  m=no; mounted && m=yes
  echo "takeover: $st, mounted: $m, failed boots: $(cat "$STATE/takeover-boots" 2>/dev/null || echo 0), foreground: $(foreground)"
}

case "${1:-status}" in
  enable)
    [ "$(id -u)" = 0 ] || [ -n "${BLOOB_TAKEOVER_TEST:-}" ] || { echo "Run as root." >&2; exit 1; }
    grep -q 'release [56]\.' /etc/starfish-release 2>/dev/null && { echo "webOS 5 and 6 need Luma's other method; not supported by Bloob." >&2; exit 1; }
    cp "$0" "$STATE/takeover.sh.new" && chmod 755 "$STATE/takeover.sh.new" && mv "$STATE/takeover.sh.new" "$STATE/takeover.sh"
    if apply open; then
      echo on > "$STATE/takeover"
      echo 0 > "$STATE/takeover-boots"
      mkdir -p "$HOOKDIR"
      if [ ! -f "$HOOK" ]; then
        printf '#!/bin/sh\n# Bloob: be LG Home again after a restart, while enabled (takeover.sh).\n[ -x %s/takeover.sh ] && (sleep 3; sh %s/takeover.sh boot) </dev/null >/dev/null 2>&1 &\nexit 0\n' "$STATE" "$STATE" > "$HOOK"
        chmod 755 "$HOOK"
      fi
      status
    else
      echo off > "$STATE/takeover"; status; exit 1
    fi ;;
  disable|restore)
    echo off > "$STATE/takeover"
    restore
    status ;;
  boot)
    [ "$(cat "$STATE/takeover" 2>/dev/null)" = on ] || exit 0
    # Bloob writes $DATA/home-ok when it has started as Home; two boots without it: give up.
    n=$(cat "$STATE/takeover-boots" 2>/dev/null || echo 0)
    if [ -f "$DATA/home-ok" ] && { [ ! -f "$STATE/takeover-boots" ] || [ "$DATA/home-ok" -nt "$STATE/takeover-boots" ]; }; then n=0; fi
    if [ "$n" -ge 2 ]; then log "Bloob did not start as Home after two restarts: LG Home stays (takeover.sh enable to try again)"; exit 0; fi
    echo $((n + 1)) > "$STATE/takeover-boots"
    apply ;;
  status) status ;;
  *) echo "usage: takeover.sh enable|disable|status" >&2; exit 2 ;;
esac
