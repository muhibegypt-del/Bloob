#!/bin/sh
# Desktop test of launcher/takeover.sh (4.0) against a stand-in application
# manager: the mount is a marker file (BLOOB_TAKEOVER_FAKE), luna-send a script.
#   sh tools/takeover-sim/test.sh
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
SCRIPT="$HERE/../../launcher/takeover.sh"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
mkdir -p "$T/apps/com.muhib.bloob/bin" "$T/apps/com.muhib.bloob/assets" "$T/target" "$T/data"
cp "$HERE/../../package/appinfo.json" "$T/apps/com.muhib.bloob/appinfo.json"
printf '#!/bin/sh\n' > "$T/apps/com.muhib.bloob/bin/bloob"; chmod +x "$T/apps/com.muhib.bloob/bin/bloob"
printf '{"id":"com.webos.app.home","main":"/usr/bin/com.webos.app.home","type":"native"}\n' > "$T/target/appinfo.json"
echo com.webos.app.home > "$T/front"
cat > "$T/luna-send" <<'LUNA'
#!/bin/sh
uri=""; json=""
for a in "$@"; do case "$a" in luna://*) uri=$a ;; \{*) json=$a ;; esac; done
echo "$uri $json" >> "$T/calls"
case "$uri" in
  */getForegroundAppInfo) echo "{\"returnValue\":true,\"appId\":\"$(cat "$T/front")\"}" ;;
  */blockSystemApp|*/releaseBlockSystemApp) echo '{"returnValue":true}' ;;
  */getAppInfo)
    if [ -f "$T/state/.fake-mounted" ]; then m=$(sed -n 's/.*"main"[^"]*"\([^"]*\)".*/\1/p' "$T/state/app/appinfo.json"); else m=/usr/bin/com.webos.app.home; fi
    echo "{\"returnValue\":true,\"appInfo\":{\"id\":\"com.webos.app.home\",\"main\":\"$T/x/$m\",\"type\":\"native\"}}" ;;
  */launch)
    [ -z "${FAKE_NO_FRONT:-}" ] && echo com.webos.app.home > "$T/front"
    echo '{"returnValue":true}' ;;
  *) echo '{"returnValue":true}' ;;
esac
LUNA
chmod +x "$T/luna-send"
export T BLOOB_TAKEOVER_FAKE=1 BLOOB_TAKEOVER_TEST=1 BLOOB_TAKEOVER_STATE="$T/state" BLOOB_TAKEOVER_TARGET="$T/target" \
  BLOOB_APPS="$T/apps" BLOOB_DATA="$T/data" BLOOB_HOOK_DIR="$T/init.d" BLOOB_LUNA_SEND="$T/luna-send"
fail=0
check() { if eval "$2"; then echo "ok   $1"; else echo "FAIL $1"; fail=1; fi; }
reboot() { rm -f "$T/state/.fake-mounted"; }   # a restart drops the mount

sh "$SCRIPT" enable >/dev/null 2>&1
check "enable: Home's manifest is Bloob's program under Home's id" "grep -q '\"id\": \"com.webos.app.home\"' '$T/state/app/appinfo.json' && grep -q 'bin/bloob' '$T/state/app/appinfo.json'"
check "enable: hidden, in the home category" "grep -q '\"visible\": false' '$T/state/app/appinfo.json' && grep -q '\"appCategory\": \[\"home\"\]' '$T/state/app/appinfo.json'"
check "enable: Bloob's files linked, LG's untouched" "[ -L '$T/state/app/bin' ] && grep -q '/usr/bin/com.webos.app.home' '$T/target/appinfo.json'"
check "enable: mounted, on, boot hook added" "[ -f '$T/state/.fake-mounted' ] && [ \"\$(cat '$T/state/takeover')\" = on ] && [ -x '$T/init.d/62-bloob-takeover' ]"
check "enable: Home re-read (block, then release)" "grep -q blockSystemApp '$T/calls' && grep -q releaseBlockSystemApp '$T/calls'"

reboot; sh "$SCRIPT" boot >/dev/null 2>&1
check "after a restart: Home is Bloob again" "[ -f '$T/state/.fake-mounted' ] && [ \"\$(cat '$T/state/takeover-boots')\" = 1 ]"
sleep 1; touch "$T/data/home-ok"
reboot; sh "$SCRIPT" boot >/dev/null 2>&1
check "a good start resets the count" "[ \"\$(cat '$T/state/takeover-boots')\" = 1 ]"
sleep 1
reboot; sh "$SCRIPT" boot >/dev/null 2>&1
reboot; sh "$SCRIPT" boot >/dev/null 2>&1
check "two restarts without Bloob starting: LG Home stays" "[ ! -f '$T/state/.fake-mounted' ] && grep -q 'LG Home stays' '$T/state/takeover.log'"

sh "$SCRIPT" disable >/dev/null 2>&1
check "disable: unmounted and off, LG's manifest read again" "[ ! -f '$T/state/.fake-mounted' ] && [ \"\$(cat '$T/state/takeover')\" = off ] && grep -q 'LG Home is back' '$T/state/takeover.log'"
check "disable: nothing deleted (stage and hook kept)" "[ -f '$T/state/app/appinfo.json' ] && [ -f '$T/init.d/62-bloob-takeover' ]"
reboot; sh "$SCRIPT" boot >/dev/null 2>&1
check "off: the boot hook does nothing" "[ ! -f '$T/state/.fake-mounted' ]"

: > "$T/state/takeover.log"
echo some.other.app > "$T/front"
FAKE_NO_FRONT=1 sh "$SCRIPT" enable >/dev/null 2>&1
check "Bloob not in front within 15 s: undone" "[ ! -f '$T/state/.fake-mounted' ] && [ \"\$(cat '$T/state/takeover')\" = off ] && grep -q 'undoing' '$T/state/takeover.log'"

touch "$T/state/.fake-foreign"
sh "$SCRIPT" enable >/dev/null 2>&1
check "another tool over LG Home: left alone" "[ ! -f '$T/state/.fake-mounted' ] && grep -q 'another tool' '$T/state/takeover.log'"
rm -f "$T/state/.fake-foreign"
mv "$T/apps/com.muhib.bloob" "$T/apps/gone"
sh "$SCRIPT" enable >/dev/null 2>&1
check "Bloob not installed: refused" "[ ! -f '$T/state/.fake-mounted' ] && grep -q 'not installed' '$T/state/takeover.log'"
exit $fail
