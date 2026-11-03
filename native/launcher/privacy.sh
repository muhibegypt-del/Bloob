#!/bin/sh
# Bloob 4.0: block LG's tracking and adverts. Run as root (the app starts it
# through the Homebrew Channel; or over SSH).
#   bloob-privacy.sh on      block, and switch LG's tracking and ad settings off
#   bloob-privacy.sh off     put everything back as it was
#   bloob-privacy.sh apply   (boot hook) block again after a restart, if on
#   bloob-privacy.sh status  write status.json
# How (the method of Native Tv, MIT, see LICENSE-native-tv):
#   - a hosts table = every line of the TV's own /etc/hosts, then LG's ad,
#     content-recognition (Live Plus / Alphonso) and telemetry names and the
#     common ad and analytics names, all to 0.0.0.0; bind-mounted over
#     /etc/hosts. Nothing on the TV is changed or deleted: a restart drops the
#     mount, and the boot hook mounts it again only while this is on. LG's
#     store, Home and firmware-update servers are left alone.
#   - LG's switches that the TV reports (Live Plus; Home, screen-saver and
#     live promotions; content recommendations; smart tips) are turned off;
#     what they were is saved, and "off" writes it back.
# Never risks the TV: the table must keep localhost, the mount is checked and
# undone if it is not right, and nothing is ever deleted.
export PATH="/bin:/sbin:/usr/bin:/usr/sbin:$PATH"
STATE=${BLOOB_PRIVACY_DIR:-/media/internal/com.muhib.bloob/privacy}
HOSTS=${BLOOB_HOSTS:-/etc/hosts}
HOOKDIR=${BLOOB_HOOK_DIR:-/var/lib/webosbrew/init.d}
HOME_DIR=${BLOOB_PRIVACY_HOME:-/var/lib/bloob-privacy}
LUNA=${BLOOB_LUNA_SEND:-luna-send}
MARK="# Bloob: LG tracking and adverts blocked (bloob-privacy.sh)"
mkdir -p "$STATE"
log() { echo "$(date '+%Y-%m-%d %H:%M:%S') $*" >> "$STATE/privacy.log"; echo "$*"; }

NAMES="ad.lgsmartad.com ibis.lgappstv.com ibs.lgappstv.com ibsstat.lgappstv.com lgsmartad.com info.lgsmartad.com
rdx.lgtvcommon.com rdx2.lgtvsdp.com aic.lgtvcommon.com cdpbeacon.lgtvcommon.com cdpsvc.lgtvcommon.com wau.lgtvcommon.com
smartad.lge.com smartclip.com smartclip-services.com yumenetworks.com
alphonso.tv prov-lg.alphonso.tv prov.alphonso.tv eulacheck.alphonso.tv bl-server.alphonso.tv clockskew.alphonso.tv
acrdb.alphonso.tv ads.alphonso.tv api.alphonso.tv tvads.alphonso.tv sync.alphonso.tv tr.alphonso.tv tn.alphonso.tv
insights.alphonso.tv bwlkup.alphonso.tv eu-acr86.alphonso.tv
ad.doubleclick.net googleads.g.doubleclick.net stats.g.doubleclick.net securepubads.g.doubleclick.net
pubads.g.doubleclick.net cm.g.doubleclick.net adclick.g.doubleclick.net fls.doubleclick.net static.doubleclick.net
adservice.google.com www.googleadservices.com pagead2.googlesyndication.com pagead.googlesyndication.com
tpc.googlesyndication.com www.google-analytics.com ssl.google-analytics.com region1.google-analytics.com
analytics.google.com app-measurement.com aax.amazon-adsystem.com aax-us-east.amazon-adsystem.com
aax-eu.amazon-adsystem.com c.amazon-adsystem.com mads.amazon-adsystem.com s.amazon-adsystem.com
device-metrics-us.amazon.com device-metrics-us-2.amazon.com bat.bing.com ads.msn.com adnxs.com ib.adnxs.com
secure.adnxs.com acdn.adnxs.com prebid.adnxs.com ads.pubmatic.com image6.pubmatic.com image8.pubmatic.com
hbopenbid.pubmatic.com fastlane.rubiconproject.com pixel.rubiconproject.com token.rubiconproject.com
eus.rubiconproject.com us-u.openx.net us-ads.openx.net rtb.openx.net sync.adap.tv ads.adaptv.advertising.com
adsrvr.org ad.adsrvr.org insight.adsrvr.org match.adsrvr.org js.adsrvr.org d.turn.com r.turn.com
ads.stickyadstv.com sb.scorecardresearch.com b.scorecardresearch.com pixel.quantserve.com secure.quantserve.com
edge.quantserve.com rules.quantcount.com pixel.adsafeprotected.com static.adsafeprotected.com px.moatads.com
z.moatads.com js-agent.newrelic.com bam.nr-data.net api.segment.io cdn.segment.com cdn.segment.io
api2.amplitude.com api.amplitude.com api-js.mixpanel.com api.mixpanel.com cdn.mxpnl.com events.appsflyer.com
t.appsflyer.com app.adjust.com sessions.bugsnag.com"
# LG's regional telemetry and ad names carry a region or country in front.
REGIONAL="info.lgsmartad.com rdx2.lgtvsdp.com ibs.lgappstv.com ibsstat.lgappstv.com cdpbeacon.lgtvcommon.com cdpsvc.lgtvcommon.com wau.lgtvcommon.com"
PREFIXES="aic eic kic eu us gb au br ca de fr"

# LG's switches: category and keys.
SWITCHES="option:livePlus option:livePromotion general:homePromotion general:screenSaverAd general:aiNudge general:aiSettingsNudge other:contentRecommendation"

mounted() { grep -q "^$MARK" "$HOSTS" 2>/dev/null; }

table() {  # the TV's own lines, then ours
  if mounted; then sed "/^$MARK/,\$d" "$HOSTS"; else cat "$HOSTS"; fi > "$STATE/hosts.own"
  cat "$STATE/hosts.own"
  echo "$MARK"
  # localhost must resolve; inside Bloob's block, so it goes when the block goes.
  grep -q '^127\.0\.0\.1[[:space:]].*localhost' "$STATE/hosts.own" || echo "127.0.0.1 localhost"
  for n in $NAMES; do echo "0.0.0.0 $n"; done
  for b in $REGIONAL; do for p in $PREFIXES; do echo "0.0.0.0 $p.$b"; done; done
}

bind() {
  if [ -n "${BLOOB_PRIVACY_FAKE:-}" ]; then cp "$1" "$2"; else mount --bind "$1" "$2"; fi
}
unbind() {
  if [ -n "${BLOOB_PRIVACY_FAKE:-}" ]; then sed -i "/^$MARK/,\$d" "$1"; else umount "$1"; fi
}

block() {
  table > "$STATE/hosts.next" || return 1
  grep -q '^127\.0\.0\.1[[:space:]].*localhost' "$STATE/hosts.next" || { log "refused: the table lost localhost"; return 1; }
  if mounted; then
    cat "$STATE/hosts.next" > "$STATE/hosts"   # in place: the mount follows the file
  else
    cat "$STATE/hosts.next" > "$STATE/hosts"
    bind "$STATE/hosts" "$HOSTS" || { log "could not mount the hosts table"; return 1; }
  fi
  if ! mounted || ! grep -q '^127\.0\.0\.1' "$HOSTS"; then
    log "the hosts table did not take: undone"
    mounted && unbind "$HOSTS"
    return 1
  fi
  log "blocking $(grep -c '^0\.0\.0\.0 ' "$STATE/hosts") names"
}

value_of() {  # category key -> value the TV reports, empty if none
  $LUNA -n 1 -f "luna://com.webos.service.settings/getSystemSettings" "{\"category\":\"$1\",\"keys\":[\"$2\"]}" 2>/dev/null |
    sed -n "s/.*\"$2\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" | head -1
}
set_value() {  # category key value
  $LUNA -n 1 "luna://com.webos.service.settings/setSystemSettings" "{\"category\":\"$1\",\"settings\":{\"$2\":\"$3\"}}" 2>/dev/null |
    grep -q '"returnValue"[[:space:]]*:[[:space:]]*true'
}

switches_off() {
  [ -f "$STATE/settings-before" ] || : > "$STATE/settings-before"
  for s in $SWITCHES; do
    cat=${s%%:*}; key=${s#*:}
    v=$(value_of "$cat" "$key")
    [ -n "$v" ] || continue
    grep -q "^$cat:$key=" "$STATE/settings-before" || echo "$cat:$key=$v" >> "$STATE/settings-before"
    [ "$v" = off ] && continue
    if set_value "$cat" "$key" off; then log "$key: $v -> off"; else log "$key: the TV refused"; fi
  done
}

switches_back() {
  [ -f "$STATE/settings-before" ] || return 0
  while IFS='=' read -r ck v; do
    [ -n "$ck" ] || continue
    cat=${ck%%:*}; key=${ck#*:}
    if set_value "$cat" "$key" "$v"; then log "$key: back to $v"; else log "$key: could not put back $v"; fi
  done < "$STATE/settings-before"
  : > "$STATE/settings-before"
}

install_hook() {  # adds files only; the hook does nothing while the state says off
  mkdir -p "$HOME_DIR" "$HOOKDIR" 2>/dev/null || return 0
  cp "$0" "$HOME_DIR/privacy.sh" 2>/dev/null && chmod 755 "$HOME_DIR/privacy.sh"
  if [ ! -f "$HOOKDIR/61-bloob-privacy" ]; then
    printf '#!/bin/sh\n# Bloob: block LG tracking and adverts again after a restart, while it is on.\n[ -x %s/privacy.sh ] && sh %s/privacy.sh apply >/dev/null 2>&1 &\nexit 0\n' "$HOME_DIR" "$HOME_DIR" > "$HOOKDIR/61-bloob-privacy"
    chmod 755 "$HOOKDIR/61-bloob-privacy"
  fi
}

status() {
  st=$(cat "$STATE/state" 2>/dev/null || echo off)
  m=false; n=0
  if mounted; then m=true; n=$(grep -c '^0\.0\.0\.0 ' "$HOSTS"); fi
  lp=$(value_of option livePlus)
  printf '{"state":"%s","mounted":%s,"blocked":%s,"livePlus":"%s","at":"%s"}\n' "$st" "$m" "$n" "$lp" "$(date '+%Y-%m-%dT%H:%M:%S')" > "$STATE/status.json"
  cat "$STATE/status.json"
}

case "${1:-status}" in
  on)
    echo on > "$STATE/state"
    block || { echo off > "$STATE/state"; status; exit 1; }
    switches_off
    install_hook
    status ;;
  off)
    echo off > "$STATE/state"
    if mounted; then unbind "$HOSTS" && log "hosts table removed: the TV's own is back"; fi
    switches_back
    status ;;
  apply)
    [ "$(cat "$STATE/state" 2>/dev/null)" = on ] || exit 0
    block; status >/dev/null ;;
  status) status ;;
  *) echo "usage: bloob-privacy.sh on|off|apply|status" >&2; exit 2 ;;
esac
