#!/usr/bin/env python3
"""A stand-in TV for testing bloob-cal on the desktop (not shipped).

One file plays three parts, sharing a state file ($FAKE_TV_STATE):
  fake_tv.py luna-send -n 1 <uri> <payload>   the pqcontroller and settings services
  fake_tv.py ssap                              the second-screen socket on 127.0.0.1:$FAKE_TV_PORT (3000)
  fake_tv.py spotread [options]                Argyll's spotread in a terminal

The panel is a small WOLED model, enough to make a one-pass grey correction imperfect:
per-channel tone curves, a white sub-pixel that takes over the common part of R, G and B
(its own colour, so greys are not additive), colour temperature and OLED light, a factory
1D LUT that trims red. It is not a model of the G2 (that is cal/sim.cpp); it exists to
check that bloob-cal sends the right commands, reads the right changes and survives the
faults seen on the owner's TV.

The meter is a spot on the screen (default the middle): it reads whichever pattern box
covers it. Readings carry noise (FAKE_TV_NOISE, 1 = realistic, 0 = none; seeded by
FAKE_TV_SEED).

Behaviour switches (environment, read by every part):
  FAKE_TV_FLICKER_S        after a pattern change the picture flickers pink for this long (LG, B1)
  FAKE_TV_WARMUP_S         the panel brightens towards steady with this time constant (0: steady)
  FAKE_TV_NO_VIDEO         no video playing: the pattern generator accepts patterns and draws nothing
  FAKE_TV_METER_AT=x,y     where the meter sits, in panel pixels (state "meter_at" overrides); it reads a
                           disc 150 px across its middle, so a meter near a window's edge sees some of beyond
  FAKE_TV_1D_INDEX=code    the 1D table is indexed by the 10-bit code (64 → entry 64), not the signal
  FAKE_TV_IGNORE_1D        the TV accepts 1D tables and keeps the old one (a table that didn't load)
  FAKE_TV_SSAP_500         every pattern command over the socket fails with "500 Application error"
  FAKE_TV_SSAP_500_COUNT=n only the first n pattern commands fail
  FAKE_TV_METER_STALL_AFTER=n  the spotread session stops answering after n readings (once)
  FAKE_TV_HIDE_PATTERNS    the meter reads Bloob's page instead of the squares (3.3.3)
  FAKE_TV_PROTECT_EXPERT1  writes to expert1 are refused
  FAKE_TV_CAL_END_REVERTS  0: CAL_END leaves colour temperature and gamut as CAL_START set them
"""
import base64, hashlib, json, math, os, random, socket, struct, sys, threading, time

STATE = os.environ.get("FAKE_TV_STATE", "/tmp/fake-tv-state.json")
FACTORY_1D = [[round(i * 32767 / 1023 * k) for i in range(1024)] for k in (0.97, 1.0, 1.0)]
# A TV whose table follows the 10-bit code has a factory table built for it: black up to 64,
# full at 940 (FAKE_TV_1D_INDEX=code).
FACTORY_1D_CODE = [[round(min(max((i - 64) / 876, 0), 1) * 32767 * k) for i in range(1024)] for k in (0.97, 1.0, 1.0)]


def factory_1d():
    return FACTORY_1D_CODE if os.environ.get("FAKE_TV_1D_INDEX") == "code" else FACTORY_1D

PANEL_W, PANEL_H = 3840, 2160
GAMMA = (2.30, 2.40, 2.48)          # per-channel tone curves (what the 1D LUT corrects)
W_SHARE = 0.6                       # share of the common R, G, B drive the white sub-pixel takes
W_XY, W_GAIN = (0.300, 0.330), 1.15  # the white sub-pixel's colour and relative brightness
REC709 = [[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]]


def env(name, default=""):
    v = os.environ.get(name)
    return v if v not in (None, "") else default


def load():
    try:
        with open(STATE) as f:
            return json.load(f)
    except Exception:
        return {"mode": "expert1", "backlight": 28, "session": None, "lut1d": None, "swap": False,
                "boxes": {}, "num": 0, "on": False, "log": [], "power_on": time.time()}


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
    tmp = STATE + ".tmp.%d" % os.getpid()
    with open(tmp, "w") as f:
        json.dump(s, f)
    os.replace(tmp, STATE)


def decode_u16(b64):
    raw = base64.b64decode(b64)
    return list(struct.unpack("<%dH" % (len(raw) // 2), raw))


def pq(payload, write, route="luna"):
    s = load()
    cmd = payload.get("command")
    s["log"].append(cmd)
    s.setdefault("routes", {}).setdefault(route, 0)
    s["routes"][route] += 1
    if write and not cmd.startswith("PTN_") and "picMode" not in payload:
        s.setdefault("missing_picmode", []).append(cmd)  # Calman sends picMode with every command
    reply = {"returnValue": True}
    if not write:
        if cmd == "GET_1D_DPG_DATA":
            t = s["lut1d"] or factory_1d()
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
        if s.get("before_cal") and env("FAKE_TV_CAL_END_REVERTS", "1") == "1":
            settings(s)["picture"].update(s.pop("before_cal"))  # the worst case: the TV puts them back
    elif cmd == "PTN_SINGLE_WINBOX_ATTR":
        s["boxes"][str(payload["winId"])] = {"rgb": [payload["fillR"], payload["fillG"], payload["fillB"]],
                                             "w": payload.get("width", PANEL_W), "h": payload.get("height", PANEL_H),
                                             "x": payload.get("startX", 0), "y": payload.get("startY", 0)}
    elif cmd == "PTN_CTRL":
        if payload["enable"] == "false" and not s["on"]:
            reply = {"returnValue": False, "errorCode": 20}
        s["on"] = payload["enable"] == "true"
        s["num"] = payload["numOfBox"]
        s["ptn_time"] = time.time()
    else:
        if not s["session"]:
            reply = {"returnValue": False, "errorText": "not in calibration mode"}
        elif s["session"] == "expert1" and env("FAKE_TV_PROTECT_EXPERT1"):
            reply = {"returnValue": False, "errorText": "the fake TV refuses writes to expert1 (the test must not touch it)"}
        elif cmd == "BACKLIGHT_UI_DATA":
            s["backlight"] = decode_u16(payload["data"])[0]
        elif cmd == "1D_DPG_DATA":
            s.setdefault("uploads_1d", 0)
            s["uploads_1d"] += 1
            if payload.get("dataOpt") == 2:
                s["lut1d"] = None
            elif not (env("FAKE_TV_IGNORE_1D") or s.get("ignore_1d")):
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
        elif cmd in ("1D_2_2_EN", "1D_0_45_EN"):
            s.setdefault("switches", {})[cmd] = None if payload.get("dataOpt") == 2 else decode_u16(payload["data"])[0]
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


def table_out(t, c, v):
    """Output of 1D table channel c at signal v, linear between entries."""
    f = min(max(v, 0.0), 1.0) * 1023
    i = min(int(f), 1022)
    w = f - i
    return (t[c][i] * (1 - w) + t[c][i + 1] * w) / 32767


METER_R = 150  # px: the meter's field on the screen


def meter_points(s):
    at = s.get("meter_at") or [float(x) for x in env("FAKE_TV_METER_AT", "1920,1080").split(",")]
    pts = [(at[0], at[1])]
    for r, n in ((METER_R / 2, 4), (METER_R, 8)):
        for i in range(n):
            a = 2 * math.pi * i / n
            pts.append((at[0] + r * math.cos(a), at[1] + r * math.sin(a)))
    return pts


def code_at(s, x, y):
    """The 10-bit code on screen at panel pixel (x, y); None where nothing is drawn."""
    if env("FAKE_TV_NO_VIDEO") or s.get("video") is False:
        return None  # the generator draws only while a video plays
    if not s["on"]:
        return None
    for i in range(int(s["num"]) - 1, -1, -1):
        b = s["boxes"].get(str(i))
        if not b:
            continue
        if isinstance(b, list):  # an old state file
            return b
        if b["x"] <= x < b["x"] + b["w"] and b["y"] <= y < b["y"] + b["h"]:
            return b["rgb"]
    return [64, 64, 64]  # the black video around the boxes


def emit(s, code):
    """XYZ the panel gives for a 10-bit RGB code."""
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
    t = s["lut1d"] or factory_1d()
    if env("FAKE_TV_1D_INDEX") == "code" and not s.get("lut3d") and not s["swap"]:
        x = [min(max(c, 0), 1023) / 1023 for c in code]
    else:
        x = v
    o = [table_out(t, c, x[c]) for c in range(3)]
    ct = float(settings(s)["picture"].get("colorTemperature", "0") or 0)
    gain = (1 + 0.0006 * ct, 1.0, 1 - 0.0016 * ct)  # warm (+) lifts red and lowers blue
    lin = [gain[c] * o[c] ** GAMMA[c] for c in range(3)]
    common = W_SHARE * min(lin)
    rgb = [x - common for x in lin]
    wX, wY = W_XY[0] / W_XY[1], 1.0
    wZ = (1 - W_XY[0] - W_XY[1]) / W_XY[1]
    k = 100.0 * s["backlight"] / 28
    return [k * (sum(REC709[r][c] * rgb[c] for c in range(3)) + W_GAIN * common * (wX, wY, wZ)[r]) for r in range(3)]


def colour(s=None):
    """XYZ the meter sees now, before noise: the average over its field."""
    s = s or load()
    if env("FAKE_TV_HIDE_PATTERNS"):
        return (11.5, 18.1, 3.6)  # what the owner's meter read with Bloob's page over the squares
    acc, cache = [0.0, 0.0, 0.0], {}
    pts = meter_points(s)
    for (x, y) in pts:
        code = code_at(s, x, y)
        if code is None:
            continue
        key = tuple(code)
        if key not in cache:
            cache[key] = emit(s, code)
        for r in range(3):
            acc[r] += cache[key][r] / len(pts)
    xyz = acc
    warm = float(env("FAKE_TV_WARMUP_S", "0"))
    if warm > 0:
        xyz = [x * (1 - 0.04 * math.exp(-(time.time() - s.get("power_on", time.time())) / warm)) for x in xyz]
    flick = float(env("FAKE_TV_FLICKER_S", "0"))
    if flick > 0 and time.time() - s.get("ptn_time", 0) < flick:
        xyz = [xyz[0] + 8.0, xyz[1] + 3.0, xyz[2] + 12.0]  # LG's pink transition after a pattern change
    return tuple(xyz)


def measure():
    """One reading: colour plus meter noise; counts readings (and those taken in calibration mode)."""
    s = load()
    X, Y, Z = colour(s)
    n = s.get("reads_total", 0) + 1
    s["reads_total"] = n
    if s["session"]:
        s["reads_in_session"] = s.get("reads_in_session", 0) + 1
    save(s)
    noise = float(env("FAKE_TV_NOISE", "1"))
    if noise > 0:
        # As a tristimulus colorimeter: mostly shared by X, Y and Z (luminance), a little per
        # channel (colour), and a floor near black (the same model as cal/sim.cpp).
        rng = random.Random(int(env("FAKE_TV_SEED", "1")) * 100003 + n)
        shared = 1 + rng.gauss(0, 0.002 * noise)
        X, Y, Z = [max(0.0, c * shared * (1 + rng.gauss(0, 0.0007 * noise)) + rng.gauss(0, 0.0008 * noise)) for c in (X, Y, Z)]
    return X, Y, Z


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
        stall = env("FAKE_TV_METER_STALL_AFTER")
        if stall:
            s = load()
            if not s.get("stalled_once") and s.get("reads_total", 0) >= int(stall):
                s["stalled_once"] = True
                save(s)
                while True:  # stops answering, as the owner's meter did after a dark patch
                    time.sleep(60)
        X, Y, Z = measure()
        sxyz = X + Y + Z or 1
        out.write("\r\n Result is XYZ: %.6f %.6f %.6f, Yxy: %.6f %.6f %.6f\r\n" % (X, Y, Z, Y, X / sxyz, Y / sxyz))
        out.flush()
        if one_shot:
            return
        prompt()


def ws_server():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", int(env("FAKE_TV_PORT", "3000"))))
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
            if m["payload"].get("command", "").startswith("PTN_"):
                fail = bool(env("FAKE_TV_SSAP_500"))
                count = env("FAKE_TV_SSAP_500_COUNT")
                if count:
                    s = load()
                    done = s.get("ssap_500_sent", 0)
                    fail = done < int(count)
                    if fail:
                        s["ssap_500_sent"] = done + 1
                        save(s)
                if fail:
                    send({"type": "error", "id": m["id"], "error": "500 Application error", "payload": {}})  # as the owner's TV answered
                    continue
            write = m["uri"].endswith("setExternalPqData")
            send({"type": "response", "id": m["id"], "payload": pq(m["payload"], write, "socket")})


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
