#!/bin/sh
# Bloob calibration, steps C0.4, C1.1 and C1.2: the meter and the TV's own patterns.
# Run on the TV as root, with the Spyder5 plugged in and hanging flat on the middle
# of the screen, in a dark room:   sh cal-meter-check.sh
#
# It writes no calibration data and changes no setting. It only shows test
# patches with the TV's pattern generator (as the Spyder probe app did) and
# switches them off at the end, even if stopped with Ctrl-C. About 5 minutes.
#   1. reads the other calibration tables back (3D LUT, matrix), read-only;
#   2. measures white ten times: how steady the meter is on this panel;
#   3. measures mid grey and near black;
#   4. compares code 940 with 1019, to learn whether SDR patches are limited range.
# See native/CALIBRATION-PLAN.md section 11.
PROBE=${PROBE:-/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service}
CCSS=${CCSS:-/media/internal/com.muhib.bloob/calibration/argyll/lg-55g2.ccss}
PQ=luna://com.webos.service.pqcontroller

json() {  # json <expression on o>
  node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{let o;try{o=JSON.parse(s)}catch(e){console.log("    (no JSON reply: "+s.replace(/\s+/g," ").trim().slice(0,160)+")");return}try{const v=('"$1"');if(v!==undefined)console.log(v)}catch(e){console.log("    (unexpected reply)")}})'
}
pq() { timeout 8 luna-send -n 1 "$PQ/$1" "$2" 2>&1; }

# The pattern generator: a full-screen black background and a 10% window in the middle.
W=1214 H=683 X=1313 Y=738
patch() {  # patch R G B (10-bit codes)
  pq setExternalPqData '{"command":"PTN_SINGLE_WINBOX_ATTR","fillR":64,"fillG":64,"fillB":64,"winId":0,"width":3840,"height":2160,"startX":0,"startY":0,"programID":1}' >/dev/null
  pq setExternalPqData "{\"command\":\"PTN_SINGLE_WINBOX_ATTR\",\"fillR\":$1,\"fillG\":$2,\"fillB\":$3,\"winId\":1,\"width\":$W,\"height\":$H,\"startX\":$X,\"startY\":$Y,\"programID\":1}" >/dev/null
  pq setExternalPqData '{"command":"PTN_CTRL","enable":"true","numOfBox":2,"ptnType":0,"programID":1}'
}
patterns_off() { pq setExternalPqData '{"command":"PTN_CTRL","enable":"false","numOfBox":0,"ptnType":0,"programID":1}' >/dev/null; }
trap 'patterns_off; echo; echo "Patterns off."' EXIT
trap 'exit 1' INT TERM

# One reading: prints "Y x y" (cd/m²), or nothing if the meter failed.
EXTRA=
read_meter() {
  printf '\n' | LD_LIBRARY_PATH="$PROBE/libs" timeout 60 /lib/ld-linux.so.3 "$PROBE/spotread" -e -O -x $EXTRA 2>&1 |
    sed -n 's/.*Result is XYZ: *[-0-9.e+]* *[-0-9.e+]* *[-0-9.e+]*, *Yxy: *\([-0-9.e+]*\) *\([-0-9.e+]*\) *\([-0-9.e+]*\).*/\1 \2 \3/p' | head -1
}
# measure LABEL N R G B: N readings of one patch, then their average and spread.
measure() {
  label=$1 n=$2
  shift 2
  out=$(patch "$@")
  echo "$out" | grep -q '"returnValue": *true' || { echo "  $label: the TV refused the pattern: $(echo "$out" | tr -d '\n' | cut -c1-160)"; return 1; }
  sleep 1
  vals=
  i=0
  t0=$(date +%s)
  while [ $i -lt "$n" ]; do
    v=$(read_meter)
    [ -n "$v" ] && vals="$vals$v
"
    i=$((i + 1))
  done
  t1=$(date +%s)
  printf '%s' "$vals" | awk -v label="$label" -v n="$n" -v secs=$((t1 - t0)) '
    NF == 3 { k++; Y[k] = $1; x[k] = $2; y[k] = $3; sY += $1; sx += $2; sy += $3 }
    END {
      if (k == 0) { printf "  %s: no readings (is the Spyder plugged in and on the screen?)\n", label; exit }
      mY = sY / k; mx = sx / k; my = sy / k
      for (i = 1; i <= k; i++) { vY += (Y[i] - mY) ^ 2; vx += (x[i] - mx) ^ 2; vy += (y[i] - my) ^ 2 }
      dY = k > 1 ? sqrt(vY / (k - 1)) : 0; dx = k > 1 ? sqrt(vx / (k - 1)) : 0; dy = k > 1 ? sqrt(vy / (k - 1)) : 0
      rel = mY > 0 ? 100 * dY / mY : 0
      printf "  %s: Y %.4f cd/m2 (spread %.2f%%), x %.4f (+-%.4f), y %.4f (+-%.4f), %d of %d readings, %.1f s each\n", label, mY, rel, mx, dx, my, dy, k, n, secs / n
    }'
}

echo "== Tables (read-only)"
pq getExternalPqData '{"command":"GET_3D_LUT_DATA"}' | json '"  3D LUT: returnValue "+o.returnValue+(o.dataCount!==undefined?" · dataCount "+o.dataCount+" (33-point: 107811)":"")+(o.errorText?" · "+o.errorText:"")'
pq getExternalPqData '{"command":"GET_3BY3_GAMUT_DATA"}' | json '"  3x3 matrix: returnValue "+o.returnValue+(o.dataCount!==undefined?" · dataCount "+o.dataCount+" · "+o.dataType:"")+(o.errorText?" · "+o.errorText:"")'
pq getExternalPqData '{"command":"GET_1D_DPG_DATA"}' | json '"  1D LUT: returnValue "+o.returnValue+(o.dataCount!==undefined?" · dataCount "+o.dataCount:"")+(o.errorText?" · "+o.errorText:"")'

echo "== Meter"
[ -x "$PROBE/spotread" ] || { echo "  spotread is missing (the Spyder probe app's copy)"; exit 1; }
found=no
for d in /sys/bus/usb/devices/*; do
  [ -f "$d/idVendor" ] && [ "$(cat "$d/idVendor")" = 085c ] && [ "$(cat "$d/idProduct")" = 0500 ] && found=yes
done
echo "  Spyder5 plugged in: $found"
[ "$found" = yes ] || { echo "  Plug the Spyder5 into the TV, hang it on the middle of the screen, and run this again."; exit 1; }
if [ -f "$CCSS" ]; then
  EXTRA="-X $CCSS"
  echo "  correction: $(sed -n 's/^DESCRIPTOR "\(.*\)"/\1/p' "$CCSS" | head -1)"
else
  echo "  correction: none (readings are uncorrected; see the note at the end)"
fi

echo "== Patterns and readings (keep the room dark and the meter still)"
measure "white 940, steadiness" 10 940 940 940 || exit 1
measure "mid grey 502" 4 502 502 502
measure "near black 120" 4 120 120 120
measure "white 1019 (range test)" 3 1019 1019 1019
if [ -n "$EXTRA" ]; then
  EXTRA=
  measure "white 940, no correction (for comparison)" 3 940 940 940
fi
echo "Range test: if white 1019 reads about 1.2x white 940, the TV takes these patches as full range;"
echo "if about the same, as limited range (the plan's assumption)."
[ -f "$CCSS" ] || echo "Note: put the LG 55G2 correction at $CCSS for corrected readings (see the instructions you were given)."
echo "Done. No calibration data or setting was changed. Paste everything above."
