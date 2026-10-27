#!/bin/sh
# Bloob calibration, step C0: read-only checks before anything is built on them.
# Run on the TV as root:  sh cal-check.sh
# Writes nothing and changes no setting. Prints no keys: the pairing key file is
# only reported as present or missing. See native/CALIBRATION-PLAN.md section 11.
PROBE=/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service

json() {  # json <expression on o>: reads JSON from stdin
  node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{try{const o=JSON.parse(s);const v=('"$1"');if(v!==undefined)console.log(v)}catch(e){console.log("(no reply)")}})'
}
yes_no() { [ -e "$1" ] && echo yes || echo no; }

echo "== TV"
timeout 5 luna-send -n 1 luna://com.webos.service.tv.systemproperty/getSystemInfo \
  '{"keys":["modelName","firmwareVersion"]}' | json 'o.modelName+" · firmware "+o.firmwareVersion'
timeout 5 luna-send -n 1 luna://com.webos.service.update/getCurrentSWInformation '{}' |
  json '"chip model "+(o.product_name||"")+" "+(o.model_name||"")'

echo "== Picture now"
timeout 5 luna-send -n 1 luna://com.webos.settingsservice/getSystemSettings \
  '{"category":"picture","keys":["pictureMode","backlight","contrast","brightness","color","gamma","colorGamut","colorTemperature"]}' |
  json 'JSON.stringify(o.settings||o)'

echo "== Meter"
found=no
for d in /sys/bus/usb/devices/*; do
  [ -f "$d/idVendor" ] || continue
  [ "$(cat "$d/idVendor")" = 085c ] && [ "$(cat "$d/idProduct")" = 0500 ] && found=yes
done
echo "Spyder5 plugged in: $found"
echo "spotread installed (probe app): $(yes_no "$PROBE/spotread")"
echo "probe's pattern manifest: $(yes_no "$PROBE/itpg_manifest.json")"
echo "probe's pairing key file: $(yes_no /media/internal/spyder-reports/itpg-client-key.json) (not read)"

echo "== Meter correction files (Argyll's Spyder data, .ccss/.ccmx)"
for top in /home /media/internal /media/developer /usr/local/share /usr/share/color /var/lib; do
  [ -d "$top" ] && find "$top" -xdev \( -iname 'spyd4cal.bin' -o -iname '*.ccss' -o -iname '*.ccmx' \) 2>/dev/null
done | head -20
echo "(nothing listed above means none found)"

echo "== Calibration service on the bus"
grep -rl externalpq /usr/share/luna-service2 /usr/share/dbus-1 2>/dev/null | head -5
for svc in com.webos.service.externalpq com.webos.service.pqcontroller; do
  echo "$svc, 1D LUT getter (read-only):"
  timeout 5 luna-send -n 1 "luna://$svc/getExternalPqData" '{"command":"GET_1D_DPG_DATA"}' 2>&1 |
    json '"  returnValue "+o.returnValue+" · dataCount "+o.dataCount+" · dataType "+o.dataType+(o.errorText?" · "+o.errorText:"")'
done

echo "== USB sticks mounted"
ls /tmp/usb 2>/dev/null || echo "(none)"
echo "Done. Nothing was changed. Paste everything above."
