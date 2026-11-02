#!/bin/sh
# Desktop test of launcher/privacy.sh (4.0): a stand-in /etc/hosts and luna-send.
#   sh tools/privacy-sim/test.sh
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
SCRIPT="$HERE/../../launcher/privacy.sh"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
printf '127.0.0.1 localhost\n::1 localhost ip6-localhost\n10.0.0.5 nas.local\n' > "$T/hosts"
cp "$T/hosts" "$T/hosts.orig"
# Settings the stand-in TV reports: Live Plus and Home promotion on, no smart tips keys.
printf 'option:livePlus=on\noption:livePromotion=on\ngeneral:homePromotion=on\ngeneral:screenSaverAd=off\nother:contentRecommendation=on\n' > "$T/settings"
cat > "$T/luna-send" <<'LUNA'
#!/bin/sh
# luna-send -n 1 [-f] <uri> <json>
uri=""; json=""
for a in "$@"; do case "$a" in luna://*) uri=$a ;; \{*) json=$a ;; esac; done
cat=$(echo "$json" | sed -n 's/.*"category":"\([^"]*\)".*/\1/p')
case "$uri" in
  */getSystemSettings)
    key=$(echo "$json" | sed -n 's/.*"keys":\["\([^"]*\)"\].*/\1/p')
    v=$(grep "^$cat:$key=" "$SETTINGS" | cut -d= -f2)
    if [ -n "$v" ]; then echo "{\"returnValue\": true, \"settings\": {\"$key\": \"$v\"}}"; else echo '{"returnValue": true, "settings": {}}'; fi ;;
  */setSystemSettings)
    key=$(echo "$json" | sed -n 's/.*"settings":{"\([^"]*\)":"\([^"]*\)"}.*/\1/p')
    val=$(echo "$json" | sed -n 's/.*"settings":{"\([^"]*\)":"\([^"]*\)"}.*/\2/p')
    sed -i "s/^$cat:$key=.*/$cat:$key=$val/" "$SETTINGS"
    echo '{"returnValue": true}' ;;
esac
LUNA
chmod +x "$T/luna-send"
export SETTINGS="$T/settings" BLOOB_PRIVACY_FAKE=1 BLOOB_HOSTS="$T/hosts" BLOOB_PRIVACY_DIR="$T/state" \
  BLOOB_HOOK_DIR="$T/init.d" BLOOB_PRIVACY_HOME="$T/home" BLOOB_LUNA_SEND="$T/luna-send"
fail=0
check() { if eval "$2"; then echo "ok   $1"; else echo "FAIL $1"; fail=1; fi; }

sh "$SCRIPT" on >/dev/null
check "on: names blocked" "[ \$(grep -c '^0\.0\.0\.0 ' '$T/hosts') -gt 150 ]"
check "on: the TV's own lines kept" "grep -q '^10.0.0.5 nas.local' '$T/hosts' && grep -q '^127.0.0.1 localhost' '$T/hosts'"
check "on: Live Plus off" "grep -q '^option:livePlus=off' '$T/settings'"
check "on: Home promotion off" "grep -q '^general:homePromotion=off' '$T/settings'"
check "on: what was there is saved" "grep -q '^option:livePlus=on' '$T/state/settings-before' && grep -q '^general:screenSaverAd=off' '$T/state/settings-before'"
check "on: boot hook added" "[ -x '$T/init.d/61-bloob-privacy' ] && [ -x '$T/home/privacy.sh' ]"
check "status: mounted and counted" "grep -q '\"mounted\":true' '$T/state/status.json' && grep -q '\"livePlus\":\"off\"' '$T/state/status.json'"
sh "$SCRIPT" on >/dev/null
check "on twice: one block, not two" "[ \$(grep -c '^# Bloob: LG tracking' '$T/hosts') -eq 1 ]"
sh "$SCRIPT" off >/dev/null
check "off: the TV's hosts file is back exactly" "cmp -s '$T/hosts' '$T/hosts.orig'"
check "off: switches put back" "grep -q '^option:livePlus=on' '$T/settings' && grep -q '^general:homePromotion=on' '$T/settings' && grep -q '^general:screenSaverAd=off' '$T/settings'"
check "off: boot hook left in place (nothing deleted), and it does nothing while off" "[ -f '$T/init.d/61-bloob-privacy' ] && sh '$SCRIPT' apply && cmp -s '$T/hosts' '$T/hosts.orig'"
# A hosts file without localhost: the table adds it rather than lose it.
printf '10.0.0.5 nas.local\n' > "$T/hosts"
sh "$SCRIPT" on >/dev/null
check "a missing localhost is added" "grep -q '^127.0.0.1 localhost' '$T/hosts'"
sh "$SCRIPT" on >/dev/null
check "and kept when switched on again" "[ \$(grep -c '^127.0.0.1 localhost' '$T/hosts') -eq 1 ]"
sh "$SCRIPT" off >/dev/null
check "and taken out again" "[ \"\$(cat '$T/hosts')\" = '10.0.0.5 nas.local' ]"
exit $fail
