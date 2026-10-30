#!/usr/bin/env python3
"""A stand-in TV for testing bloob-cal on the desktop (not shipped).

One file plays three parts, sharing a state file ($FAKE_TV_STATE):
  fake_tv.py luna-send -n 1 <uri> <payload>   the pqcontroller and settings services
  fake_tv.py ssap                              the second-screen socket on 127.0.0.1:3000
  fake_tv.py spotread [options]                Argyll's spotread in a terminal

The panel is deliberately simple (Rec.709 primaries, gamma 2.4, a factory 1D LUT that
trims red): enough to check that bloob-cal sends the right commands and reads the
right changes, not a model of the G2 (that is cal/sim.cpp).
"""
import base64, hashlib, json, os, socket, struct, sys, threading, time

STATE = os.environ.get("FAKE_TV_STATE", "/tmp/fake-tv-state.json")
FACTORY_1D = [[round(i * 32767 / 1023 * k) for i in range(1024)] for k in (0.97, 1.0, 1.0)]


def load():
    try:
        with open(STATE) as f:
            return json.load(f)
    except Exception:
        return {"mode": "expert1", "backlight": 28, "session": None, "lut1d": None, "swap": False,
                "boxes": {}, "num": 0, "on": False, "log": []}


# Settings by category, as the settings service keeps them (processing on, as some owners have it).
DEFAULT_SETTINGS = {
    "picture": {"contrast": "85", "brightness": "50", "color": "50", "gamma": "high1", "colorGamut": "auto",
                "colorTemperature": "-50", "energySaving": "min", "dynamicContrast": "low", "dynamicColor": "off",
                "peakBrightness": "off", "logoLuminanceAdjust": "light"},
    "aiPicture": {"ai_Picture": "on", "ai_Brightness": "on", "ai_Genre": "off"},
}


def settings(s):
    if "settings" not in s:
        s["settings"] = json.loads(json.dumps(DEFAULT_SETTINGS))
    return s["settings"]


def save(s):
    tmp = STATE + ".tmp"
    with open(tmp, "w") as f:
        json.dump(s, f)
    os.replace(tmp, STATE)


def decode_u16(b64):
    raw = base64.b64decode(b64)
    return list(struct.unpack("<%dH" % (len(raw) // 2), raw))


def pq(payload, write):
    s = load()
    cmd = payload.get("command")
    s["log"].append(cmd)
    reply = {"returnValue": True}
    if not write:
        if cmd == "GET_1D_DPG_DATA":
            t = s["lut1d"] or FACTORY_1D
            flat = [v for ch in t for v in ch]
            reply.update(data=base64.b64encode(struct.pack("<%dH" % len(flat), *flat)).decode(), dataCount=len(flat),
                         dataType="unsigned integer16")
        elif cmd == "GET_3D_LUT_DATA":
            reply.update(dataCount=107811, dataType="unsigned integer16", data="AAAA")
        else:
            reply = {"returnValue": False, "errorCode": -1, "errorText": "not supported"}
        save(s)
        return reply
    if cmd == "CAL_START":
        s["session"] = payload.get("picMode")
        pic = settings(s)["picture"]
        s["before_cal"] = {"colorTemperature": pic["colorTemperature"], "colorGamut": pic["colorGamut"]}
        pic.update({"colorTemperature": "50", "colorGamut": "wide"})  # CAL_START's side effects [bsc]
    elif cmd == "CAL_END":
        s["session"] = None
        if s.get("before_cal") and os.environ.get("FAKE_TV_CAL_END_REVERTS", "1") == "1":
            settings(s)["picture"].update(s.pop("before_cal"))  # the worst case: the TV puts them back
    elif cmd == "PTN_SINGLE_WINBOX_ATTR":
        s["boxes"][str(payload["winId"])] = [payload["fillR"], payload["fillG"], payload["fillB"]]
    elif cmd == "PTN_CTRL":
        if payload["enable"] == "false" and not s["on"]:
            reply = {"returnValue": False, "errorCode": 20}
        s["on"] = payload["enable"] == "true"
        s["num"] = payload["numOfBox"]
    else:
        if not s["session"]:
            reply = {"returnValue": False, "errorText": "not in calibration mode"}
        elif s["session"] == "expert1" and os.environ.get("FAKE_TV_PROTECT_EXPERT1"):
            reply = {"returnValue": False, "errorText": "the fake TV refuses writes to expert1 (the test must not touch it)"}
        elif cmd == "BACKLIGHT_UI_DATA":
            s["backlight"] = decode_u16(payload["data"])[0]
        elif cmd == "1D_DPG_DATA":
            if payload.get("dataOpt") == 2:
                s["lut1d"] = None
            else:
                flat = decode_u16(payload["data"])
                s["lut1d"] = [flat[0:1024], flat[1024:2048], flat[2048:3072]]
        elif cmd in ("BT709_3D_LUT_DATA", "BT2020_3D_LUT_DATA"):
            if payload.get("dataOpt") == 2:
                s["swap"] = False
                s["lut3d"] = None
            else:
                v = decode_u16(payload["data"])
                s["swap"] = v[3] == 0 and v[4] == 128  # node (r=1,g=0,b=0) became green
                s["lut3d"] = None if s["swap"] else v
    save(s)
    return reply


def luna(argv):
    uri, payload = argv[-2], json.loads(argv[-1])
    s = load()
    if uri.endswith("pqcontroller/setExternalPqData"):
        r = pq(payload, True)
    elif uri.endswith("pqcontroller/getExternalPqData"):
        r = pq(payload, False)
    elif uri.endswith("getSystemSettings"):
        cat = settings(s).get(payload.get("category", "picture"))
        if cat is None:
            r = {"returnValue": False, "errorText": "unknown category"}
        else:
            have = dict(cat, pictureMode=s["mode"], backlight=s["backlight"]) if payload.get("category", "picture") == "picture" else cat
            r = {"returnValue": True, "settings": {k: have[k] for k in payload.get("keys", []) if k in have}}
        save(s)
    elif uri.endswith("setSystemSettings"):
        st = payload["settings"]
        cat = payload.get("category", "picture")
        if "pictureMode" in st:
            s["mode"] = st["pictureMode"]
        if "backlight" in st:
            s["backlight"] = int(st["backlight"])
        settings(s).setdefault(cat, {}).update({k: v for k, v in st.items() if k not in ("pictureMode", "backlight")})
        s.setdefault("settings_set", []).append({cat: st})
        save(s)
        r = {"returnValue": True}
    else:
        r = {"returnValue": False, "errorText": "Service does not exist"}
    print(json.dumps(r))


def colour():
    """XYZ of what is on screen now."""
    s = load()
    if not s["on"]:
        return (0.0, 0.0, 0.0)
    code = s["boxes"].get("1" if s["num"] >= 2 else "0", [64, 64, 64])
    v = [min(max((c - 64) / 876, 0), 1) for c in code]
    if s["swap"]:
        v = [v[1], v[0], v[2]]
    elif s.get("lut3d"):
        # Trilinear between the 33-point table's nodes, as a TV blends them.
        t3 = s["lut3d"]
        f = [min(max(c, 0.0), 1.0) * 32 for c in v]
        i0 = [min(int(x), 31) for x in f]
        w = [f[k] - i0[k] for k in range(3)]
        acc = [0.0, 0.0, 0.0]
        for dz in (0, 1):
            for dy in (0, 1):
                for dx in (0, 1):
                    wt = (w[0] if dx else 1 - w[0]) * (w[1] if dy else 1 - w[1]) * (w[2] if dz else 1 - w[2])
                    i = (((i0[2] + dz) * 33 + i0[1] + dy) * 33 + i0[0] + dx) * 3
                    for k in range(3):
                        acc[k] += wt * (1.0 if t3[i + k] >= 4095 else t3[i + k] / 4096.0)
        v = acc
    t = s["lut1d"] or FACTORY_1D
    v = [t[c][min(int(v[c] * 1023 + 0.5), 1023)] / 32767 for c in range(3)]
    lin = [x ** 2.4 for x in v]
    m = [[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]]
    k = 100.0 * s["backlight"] / 28
    return tuple(k * sum(m[r][c] * lin[c] for c in range(3)) for r in range(3))


def spotread(argv):
    import termios, tty
    one_shot = "-O" in argv
    fd = sys.stdin.fileno()
    if os.isatty(fd):
        tty.setraw(fd)
    out = sys.stdout
    def prompt():
        out.write("\r\nPlace instrument on spot to be measured,\r\nHit ESC or Q to exit, any other key to take a reading: ")
        out.flush()
    prompt()
    while True:
        c = os.read(fd, 1)
        if not c or c in (b"q", b"Q", b"\x1b"):
            return
        time.sleep(0.05)
        X, Y, Z = colour()
        sxyz = X + Y + Z or 1
        out.write("\r\n Result is XYZ: %.6f %.6f %.6f, Yxy: %.6f %.6f %.6f\r\n" % (X, Y, Z, Y, X / sxyz, Y / sxyz))
        out.flush()
        if one_shot:
            return
        prompt()


def ws_server():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", int(os.environ.get("FAKE_TV_PORT", "3000"))))
    srv.listen(4)
    while True:
        conn, _ = srv.accept()
        threading.Thread(target=ws_client, args=(conn,), daemon=True).start()


def ws_client(conn):
    f = conn.makefile("rb")
    head = b""
    while not head.endswith(b"\r\n\r\n"):
        head += f.read(1)
    key = [l.split(b":", 1)[1].strip() for l in head.split(b"\r\n") if l.lower().startswith(b"sec-websocket-key")][0]
    acc = base64.b64encode(hashlib.sha1(key + b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest())
    conn.sendall(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + acc + b"\r\n\r\n")

    def recv():
        h = f.read(2)
        if len(h) < 2:
            return None
        n = h[1] & 0x7F
        if n == 126:
            n = struct.unpack(">H", f.read(2))[0]
        elif n == 127:
            n = struct.unpack(">Q", f.read(8))[0]
        mask = f.read(4) if h[1] & 0x80 else b"\0\0\0\0"
        data = bytearray(f.read(n))
        for i in range(len(data)):
            data[i] ^= mask[i & 3]
        return data.decode()

    def send(obj):
        b = json.dumps(obj).encode()
        h = bytes([0x81]) + (bytes([len(b)]) if len(b) < 126 else bytes([126]) + struct.pack(">H", len(b)) if len(b) < 65536 else bytes([127]) + struct.pack(">Q", len(b)))
        conn.sendall(h + b)

    while True:
        msg = recv()
        if msg is None:
            return
        m = json.loads(msg)
        if m["type"] == "register":
            if not m["payload"].get("client-key"):
                send({"type": "response", "id": m["id"], "payload": {"pairingType": "PROMPT"}})
            send({"type": "registered", "id": m["id"], "payload": {"client-key": "fake-key-0001"}})
        elif m["type"] == "request":
            write = m["uri"].endswith("setExternalPqData")
            send({"type": "response", "id": m["id"], "payload": pq(m["payload"], write)})


if __name__ == "__main__":
    part = sys.argv[1] if len(sys.argv) > 1 else ""
    if part == "luna-send":
        luna(sys.argv[2:])
    elif part == "ssap":
        ws_server()
    elif part == "spotread":
        spotread(sys.argv[2:])
    else:
        print(__doc__)
