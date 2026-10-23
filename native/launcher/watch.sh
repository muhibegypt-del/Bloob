#!/bin/sh
# Bloob as the TV's home screen (2.6). Adapted from Lumen's tools/tv/watch.sh (MIT, see LICENSE-lumen),
# trimmed to the Home takeover: no screensaver, Frame, recent list or hosts handling.
# Once a second, checks which app is in front:
#  - LG's own Home in front (Home button, power-on, wake): opens Bloob instead, at most every 3 s.
#  - For WAKE_SECS after a boot or a wake (power state back to Active, or a gap in this loop's ticks):
#    an input or Live TV in front (the TV resumes the last input) is replaced by Bloob too.
#  - Safety net: if Bloob has been launched 3 times and LG Home is still in front, Bloob is failing to start;
#    LG Home is then left alone until something else comes to the front, so the TV is never stuck.
#  - About a minute after a boot, starts Bloob hidden (a preload launch) so the first Home press is warm.
# Does nothing if Bloob is not installed. Log: /tmp/bloob-home.log.
BLOOB=com.muhib.bloob
LGHOME=com.webos.app.home
APPS=/media/developer/apps/usr/palm/applications
NOPRELOAD_FLAG=/var/lib/bloob-home/no-boot-preload
WAKE_SECS=20
prev=""
last=0
tries=0
tick=$(date +%s)
preloadat=$((tick + 60))
wakeuntil=$((tick + 90))   # a boot counts as a wake; the TV may take a while to show its first app
pwlast=""
log() { echo "$(date +%T) $*" >> /tmp/bloob-home.log; }

while true; do
  now=$(date +%s)
  gap=$((now - tick)); tick=$now
  if [ ! -d "$APPS/$BLOOB" ]; then sleep 5; continue; fi
  # wake: power back to Active from standby or off (not from the screensaver), or the loop was suspended
  pw=$(luna-send -n 1 luna://com.webos.service.tvpower/power/getPowerState '{}' </dev/null 2>/dev/null)
  pwstate=$(printf '%s' "$pw" | sed -n 's/.*"state": *"\([^"]*\)".*/\1/p')
  woke=0
  [ "$gap" -gt 5 ] && woke=1
  if [ "$pwstate" = Active ] && [ -n "$pwlast" ] && [ "$pwlast" != Active ] && [ "$pwlast" != "Screen Saver" ]; then woke=1; fi
  [ -n "$pwstate" ] && pwlast=$pwstate
  if [ "$woke" -eq 1 ]; then wakeuntil=$((now + WAKE_SECS)); tries=0; log "wake (gap ${gap}s, power $pwstate)"; fi
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
    *'"appId":"'$BLOOB'"'*) prev=$BLOOB; tries=0; wakeuntil=0 ;;
    *'"appId":"com.webos.app.hdmi'*|*'"appId":"com.webos.app.livetv"'*)
      # Just after power-on the TV resumes the last input: Bloob instead, once the screen is on.
      if [ "$now" -lt "$wakeuntil" ] && [ "$pwstate" = Active ] && [ "$tries" -lt 3 ] && [ $((now - last)) -ge 3 ]; then
        last=$now
        tries=$((tries + 1))
        log "input at power-on -> Bloob (try $tries): $(luna-send -n 1 luna://com.webos.applicationManager/launch "{\"id\":\"$BLOOB\"}" </dev/null 2>&1)"
      fi
      prev="other"
      ;;
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
