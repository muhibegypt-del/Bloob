#!/bin/sh
# Bloob calibration, step C0: read-only checks before anything is built on them.
# Run on the TV as root:  sh cal-check.sh
# Writes nothing and changes no setting. Prints no keys: the pairing key file is
# only reported as present or missing. See native/CALIBRATION-PLAN.md section 11.
PROBE=/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service
DATA=/media/internal/com.muhib.bloob
GATEWAY_WS=/usr/palm/services/com.webos.service.secondscreen.gateway/node_modules/ws

# json <expression on o>: reads JSON from stdin and prints the expression.
json() {
  if command -v node >/dev/null 2>&1; then
    node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{let o;try{o=JSON.parse(s)}catch(e){console.log("    (no JSON reply: "+s.replace(/\s+/g," ").trim().slice(0,160)+")");return}try{const v=('"$1"');if(v!==undefined)console.log(v)}catch(e){console.log("  (unexpected reply)")}})'
  else
    head -c 300; echo
  fi
}
yes_no() { [ -e "$1" ] && echo yes || echo no; }
luna() { timeout 6 luna-send -n 1 "$@" 2>&1; }

echo "== TV"
luna luna://com.webos.service.tv.systemproperty/getSystemInfo '{"keys":["modelName","firmwareVersion"]}' |
  json '"  "+o.modelName+" · firmware "+o.firmwareVersion'
luna luna://com.webos.service.update/getCurrentSWInformation '{}' |
  json '"  model code "+(o.model_name||"?")+" · product "+(o.product_name||"?")+" · version "+(o.major_ver||"?")+"."+(o.minor_ver||"?")'

echo "== Picture now"
luna luna://com.webos.settingsservice/getSystemSettings \
  '{"category":"picture","keys":["pictureMode","backlight","contrast","brightness","color","gamma","colorGamut","colorTemperature","energySaving"]}' |
  json '"  "+JSON.stringify(o.settings||o)'

echo "== Meter"
found=no node=
for d in /sys/bus/usb/devices/*; do
  [ -f "$d/idVendor" ] && [ -f "$d/idProduct" ] || continue
  if [ "$(cat "$d/idVendor")" = 085c ] && [ "$(cat "$d/idProduct")" = 0500 ]; then
    found=yes
    node=$(printf '/dev/bus/usb/%03d/%03d' "$(cat "$d/busnum" 2>/dev/null || echo 0)" "$(cat "$d/devnum" 2>/dev/null || echo 0)")
  fi
done
echo "  Spyder5 plugged in: $found${node:+ ($node, readable $( [ -r "$node" ] && echo yes || echo no), writable $( [ -w "$node" ] && echo yes || echo no))}"
echo "  spotread installed (probe app): $(yes_no "$PROBE/spotread")"
echo "  spotread libraries: $(yes_no "$PROBE/libs")"
echo "  probe's pattern manifest: $(yes_no "$PROBE/itpg_manifest.json")"
echo "  probe's pairing key file: $(yes_no /media/internal/spyder-reports/itpg-client-key.json) (not read)"
echo "  second-screen socket module: $(yes_no "$GATEWAY_WS")"

echo "== Meter data (spyd4cal.bin, .ccss, .ccmx)"
for top in "$DATA" "$PROBE" /home /root /media/internal /media/developer /usr/local/share /usr/share/color /usr/share/ArgyllCMS /var/lib; do
  [ -d "$top" ] && find "$top" -xdev \( -iname 'spyd4cal.bin' -o -iname '*.ccss' -o -iname '*.ccmx' \) 2>/dev/null
done | sort -u | head -20 | sed 's/^/  /'
echo "  (nothing listed above means none were found)"

echo "== Calibration service on the bus"
grep -rl externalpq /usr/share/luna-service2 /usr/share/dbus-1 /etc/luna-service2 2>/dev/null | head -5 | sed 's/^/  /'
for svc in com.webos.service.externalpq com.webos.service.pqcontroller; do
  echo "  $svc, 1D LUT getter (read-only):"
  luna "luna://$svc/getExternalPqData" '{"command":"GET_1D_DPG_DATA"}' |
    json '"    returnValue "+o.returnValue+(o.dataCount!==undefined?" · dataCount "+o.dataCount+" · dataType "+o.dataType:"")+(o.errorText?" · "+o.errorText:"")+(o.errorCode!==undefined?" · code "+o.errorCode:"")'
done

echo "== Space and sticks"
df -h /media/internal 2>/dev/null | tail -1 | awk '{print "  /media/internal free: "$4}'
if ls /tmp/usb/* >/dev/null 2>&1; then ls -d /tmp/usb/*/* 2>/dev/null | sed 's/^/  USB: /'; else echo "  USB sticks: none mounted"; fi

echo "Done. Nothing was changed. Paste everything above."
