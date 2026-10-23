#!/bin/sh
# Bloob as the TV's home screen (2.6). Adapted from Lumen's tools/tv/watch.sh (MIT, see LICENSE-lumen),
# trimmed to the Home takeover: no screensaver, Frame, recent list or hosts handling.
# Once a second, checks which app is in front:
#  - LG's own Home in front (Home button, power-on, wake): opens Bloob instead, at most every 3 s.
#  - Safety net: if Bloob has been launched 3 times and LG Home is still in front, Bloob is failing to start;
#    LG Home is then left alone until something else comes to the front, so the TV is never stuck.
#  - About a minute after a boot, starts Bloob hidden (a preload launch) so the first Home press is warm.
# Does nothing if Bloob is not installed. Log: /tmp/bloob-home.log.
BLOOB=com.muhib.bloob
LGHOME=com.webos.app.home
APPS=/media/developer/apps/usr/palm/applications
NOPRELOAD_FLAG=/var/lib/bloob-home/no-boot-preload
prev=""
last=0
tries=0
tick=$(date +%s)
preloadat=$((tick + 60))
log() { echo "$(date +%T) $*" >> /tmp/bloob-home.log; }

while true; do
  now=$(date +%s)
  if [ ! -d "$APPS/$BLOOB" ]; then sleep 5; continue; fi
  out=$(luna-send -n 1 luna://com.webos.applicationManager/getForegroundAppInfo '{}' </dev/null 2>/dev/null)
  case "$out" in
    *'"appId":"'$LGHOME'"'*)
      [ "$prev" != "$LGHOME" ] && tries=0
      if [ "$tries" -lt 3 ] && { [ $((now - last)) -ge 3 ] || [ $((now - last)) -lt 0 ]; }; then
        last=$now
        tries=$((tries + 1))
        r=$(luna-send -n 1 luna://com.webos.applicationManager/launch "{\"id\":\"$BLOOB\"}" </dev/null 2>&1)
        log "LG Home -> Bloob (try $tries): $r"
        [ "$tries" -eq 3 ] && log "Bloob did not come up; leaving LG Home alone"
      fi
      prev=$LGHOME
      ;;
    *'"appId":"'$BLOOB'"'*) prev=$BLOOB; tries=0 ;;
    *) prev="other" ;;
  esac
  # boot: start Bloob hidden once, so the first Home press is warm. Only with the screen on.
  if [ "$preloadat" -gt 0 ] && [ "$now" -ge "$preloadat" ]; then
    case "$(luna-send -n 1 luna://com.webos.service.tvpower/power/getPowerState '{}' </dev/null 2>/dev/null)" in
      *'"Active"'*)
        preloadat=0
        if [ ! -f "$NOPRELOAD_FLAG" ]; then
          case "$(luna-send -n 1 luna://com.webos.applicationManager/running '{}' </dev/null 2>/dev/null)" in
            *'"id":"'$BLOOB'"'*) ;;
            *) log "boot preload: $(luna-send -n 1 luna://com.webos.applicationManager/launch "{\"id\":\"$BLOOB\",\"preload\":\"full\"}" </dev/null 2>&1)" ;;
          esac
        fi
        ;;
    esac
  fi
  sleep 1
done
