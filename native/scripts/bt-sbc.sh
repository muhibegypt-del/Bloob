#!/bin/sh
# Bloob: the TV's Bluetooth sound quality for headphones (SBC). Run on the TV as root.
# Nothing is written to the TV: every change ends when the headphones reconnect
# (turn them off and on), so it is safe to try and compare by ear.
#   sh bt-sbc.sh          show the connected headphones and their SBC settings
#   sh bt-sbc.sh max      SBC at the highest bitpool the headphones accept
#   sh bt-sbc.sh xq       SBC XQ: dual channel, bitpool 47 per channel (about 600 kb/s at 48 kHz)
#   sh bt-sbc.sh set N    bitpool N
# Methods from LG's Bluetooth service (webOS OSE com.webos.service.bluetooth2:
# a2dp/internal/getCodecConfiguration, setSbcEncoderBitpool; setCodecConfiguration is LG's addition).
BT=luna://com.webos.service.bluetooth2

json() {  # json <expression on o>: reads JSON from stdin
  node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{try{const o=JSON.parse(s);const v=('"$1"');if(v!==undefined)console.log(v)}catch(e){}})'
}

# The connected headphones: the device with an a2dp connection (or BT_ADDR=xx:xx:… to choose).
command -v node >/dev/null || { echo "The TV's node is missing: run as BT_ADDR=<headphones address> sh bt-sbc.sh"; }
LINE=${BT_ADDR:+$BT_ADDR headphones}
[ -n "$LINE" ] || LINE=$(luna-send -n 1 -f $BT/device/getStatus '{}' |
  json 'o.devices.filter(d=>(d.connectedProfiles||[]).indexOf("a2dp")>=0).map(d=>d.address+" "+d.name)[0]')
ADDR=${LINE%% *}
if [ -z "$ADDR" ]; then
  echo "No Bluetooth headphones are playing from the TV. Connect them, start a film, then run this again."
  exit 1
fi
echo "Headphones: ${LINE#* } ($ADDR)"

config() {
  timeout 4 luna-send -n 2 -f $BT/a2dp/internal/getCodecConfiguration "{\"address\":\"$ADDR\",\"subscribe\":true}" 2>&1
}
CONFIG=$(config)
echo "$CONFIG" | grep -E '"(sampleFrequency|channelMode|blockLength|subbands|allocationMethod|minBitpool|maxBitpool|errorText)"'
MAX=$(echo "$CONFIG" | tr -d '\n' | sed -n 's/.*"maxBitpool": *\([0-9]*\).*/\1/p')
# LG's service reports 0 for numbers it doesn't track (seen on an OLED55G2): treat as unknown.
# 53 is the usual maximum headphones accept for joint stereo, and the A2DP high-quality setting.
[ "${MAX:-0}" -gt 0 ] 2>/dev/null || MAX=

bitpool() {
  case "$1" in ''|*[!0-9]*) echo "Bitpool must be a number from 2 to 250."; exit 1 ;; esac
  [ "$1" -ge 2 ] && [ "$1" -le 250 ] || { echo "Bitpool must be a number from 2 to 250."; exit 1; }
  echo "Setting bitpool $1"
  timeout 5 luna-send -n 1 -f $BT/a2dp/internal/setSbcEncoderBitpool "{\"address\":\"$ADDR\",\"bitpool\":$1}"
}

case "$1" in
  max)
    bitpool "${MAX:-53}"
    ;;
  set)
    bitpool "${2:?bitpool number}"
    ;;
  xq)
    echo "Asking for dual channel"
    timeout 5 luna-send -n 1 -f $BT/a2dp/internal/setCodecConfiguration \
      "{\"address\":\"$ADDR\",\"sbcConfiguration\":{\"channelMode\":\"dualChannel\"}}"
    # In dual channel the bitpool counts per channel: 47 stays under the usual maximum of 53.
    P=47
    [ -n "$MAX" ] && [ "$MAX" -lt 47 ] && P=$MAX
    bitpool $P
    echo "Now:"
    config | grep -E '"(channelMode|maxBitpool|errorText)"'
    echo "If dual channel was refused, paste this too:"
    strings /usr/sbin/webos-bluetooth-service | grep -E 'sbcConfiguration' | grep '{' | head -5
    ;;
  "") ;;
  *) echo "usage: sh bt-sbc.sh [max | xq | set N]"; exit 1 ;;
esac
