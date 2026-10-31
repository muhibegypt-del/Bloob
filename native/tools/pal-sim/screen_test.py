#!/usr/bin/env python3
"""The Calibrate screen's Pal session, on the desktop: the real app (bloob_host, scripted
remote presses) with the stand-in TV, while a scripted Pal runs bloob-cal's commands beside it
as the owner's computer would over SSH. Not shipped.

  python3 tools/pal-sim/screen_test.py [build-host] [output-dir]

Checks: the Pal row holds the screen see-through without the black video; the card follows
Pal's commands; pal-screen video on/off is obeyed; pal-end on the computer releases the
screen; OK on the screen ends an open session with an undo. Saves PNGs of each state.
"""
import json, os, shutil, subprocess, sys, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pal_sim  # noqa: E402  (the stand-in TV and its environment)

NATIVE = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(NATIVE, "build-host"))
OUT = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else os.path.join(BUILD, "pal-screen"))
pal_sim.BIN = os.path.join(BUILD, "bloob-cal")

# Straight to Settings → Calibrate once Home has loaded (remote presses through the first-run
# welcome are timing-sensitive; the Settings row itself is covered by the tour).
TO_CALIBRATE = ["wait:5000", "open:calibrate", "wait:800", "snap:01-calibrate"]


def app(tv, name, steps, port):
    """Runs the app with the scripted remote; returns its log."""
    stage = os.path.join(BUILD, "stage")
    data = tempfile.mkdtemp(prefix="pal-screen-svc-")
    svc = subprocess.Popen(["node", "service/server.js"], cwd=os.path.join(NATIVE, "..", "tv-app"),
                           env=dict(os.environ, TV_APP_DATA_DIR=data, TV_APP_PORT=str(port),
                                    BLOOB_DEV_FIXTURES="../native/tests/fixtures/dev-service.js", BLOOB_DEV_UNPAIRED="1"),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1)
    env = dict(tv.env, SDL_VIDEODRIVER="dummy", BLOOB_WINDOW="1280x720", BLOOB_SERVICE_PORT=str(port), BLOOB_EFFECTS="full",
               BLOOB_SOUND_OUTPUT="bt_soundbar", BLOOB_SNAPSHOT_DIR=OUT, BLOOB_SCRIPT="|".join(steps), BLOOB_HOST_EXEC="1",
               BLOOB_CAL_BIN=pal_sim.BIN)
    p = subprocess.run([os.path.join(stage, "bin", "bloob_host")], env=env, capture_output=True, text=True, timeout=300)
    svc.terminate()
    svc.wait()
    shutil.rmtree(data, ignore_errors=True)
    with open(os.path.join(OUT, name + ".log"), "w") as f:
        f.write(p.stdout + p.stderr)
    return p.stdout + p.stderr


def later(delay, fn):
    t = threading.Timer(delay, fn)
    t.start()
    return t


def main():
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        os.remove(os.path.join(OUT, f))
    stage = os.path.join(BUILD, "stage")
    os.makedirs(os.path.join(stage, "bin"), exist_ok=True)
    os.makedirs(os.path.join(stage, "assets", "fonts"), exist_ok=True)
    shutil.copy(os.path.join(BUILD, "bloob_host"), os.path.join(stage, "bin"))
    for f in os.listdir(os.path.join(NATIVE, "third_party", "fonts")):
        if f.endswith(".ttf"):
            shutil.copy(os.path.join(NATIVE, "third_party", "fonts", f), os.path.join(stage, "assets", "fonts"))

    # 1. A full session from the computer: the screen holds, follows Pal, obeys pal-screen, releases on pal-end.
    tv = pal_sim.Tv("screen", {"BLOOB_CAL_FAST": None, "BLOOB_PATTERN_DELAY_MS": "150"})
    events = {}
    try:
        def pal_side():
            events["start"] = tv.run("pal-start", "140")[0]
            events["video_on"] = tv.run("pal-screen", "video", "on")[0]
            time.sleep(6)
            events["video_off"] = tv.run("pal-screen", "video", "off")[0]
            events["report"] = tv.run("grey-report")[0]
            time.sleep(2)
            events["end"] = tv.run("pal-end", "ok")[0]
        later(10, pal_side)
        steps = TO_CALIBRATE + ["key:ok", "key:ok", "wait:1500", "snap:02-pal-waiting"] + \
                ["wait:12000", "snap:03-pal-running"] + ["wait:150000", "snap:05-after-end", "quit"]
        log = app(tv, "session", steps, 8795)
        pal_sim.check("screen: Pal's commands all ran", all(events.get(k) == 0 for k in ("start", "video_on", "video_off", "report", "end")), str(events))
        pal_sim.check("screen: the Pal row holds the session", "Pal session held" in log)
        held = log[log.find("Pal session held"):]
        on, off = held.find("black video loading"), held.find("black video unloaded")
        pal_sim.check("screen: no black video until Pal asks; on and off as Pal says", on > 0 and off > on and "black video" not in held[:on])
        pal_sim.check("screen: pal-end from the computer releases the screen", "Pal session ended" in log)
    finally:
        tv.close()

    # 2. OK on the screen ends an open session: the TV goes back to how it was.
    tv = pal_sim.Tv("screen-stop")
    try:
        later(10, lambda: tv.run("pal-start", "140"))
        steps = TO_CALIBRATE + ["key:ok", "key:ok", "wait:12000", "snap:06-pal-open", "key:ok", "wait:8000", "snap:07-after-stop", "quit"]
        log = app(tv, "stop", steps, 8796)
        st = tv.st()
        hist = os.listdir(os.path.join(tv.data, "history")) if os.path.isdir(os.path.join(tv.data, "history")) else []
        pal_sim.check("screen: OK ends an open session with an undo", "Pal session ended" in log and pal_sim.factory_and_settings_back(tv) and len(hist) == 1,
                      "history %s" % hist)
    finally:
        tv.close()

    # Pictures: the see-through card over black, as the meter's side of the screen would look.
    for f in sorted(os.listdir(OUT)):
        if f.endswith(".bmp"):
            src = os.path.join(OUT, f)
            subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-f", "lavfi", "-i", "color=c=0x404040:s=1920x1080", "-i", src, "-filter_complex",
                            "[1:v]format=rgba,scale=1920:1080[ui];[0:v][ui]overlay", "-frames:v", "1", src[:-4] + ".png"])
            os.remove(src)
    bad = [r for r in pal_sim.RESULTS if not r[1]]
    print("\n%d checks, %d failed; pictures in %s" % (len(pal_sim.RESULTS), len(bad), OUT))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
