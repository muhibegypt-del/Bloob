#!/usr/bin/env python3
"""The Greyscale app (4.0.1) on the desktop: the real app in Greyscale mode (bloob_host,
BLOOB_GREYSCALE_APP=1) starts `bloob-cal greyscale 0.5 2.4` against the stand-in TV, shows the
results on screen, and nothing on the TV changes. Not shipped.

  python3 tools/pal-sim/greyscale_test.py [build-host] [output-dir]
"""
import json, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pal_sim  # noqa: E402

NATIVE = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(NATIVE, "build-host"))
OUT = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else os.path.join(BUILD, "greyscale"))
pal_sim.BIN = os.path.join(BUILD, "bloob-cal")


def main():
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT)
    stage = os.path.join(BUILD, "stage")
    os.makedirs(os.path.join(stage, "bin"), exist_ok=True)
    os.makedirs(os.path.join(stage, "assets", "fonts"), exist_ok=True)
    shutil.copy(os.path.join(BUILD, "bloob_host"), os.path.join(stage, "bin"))
    for f in os.listdir(os.path.join(NATIVE, "third_party", "fonts")):
        if f.endswith(".ttf"):
            shutil.copy(os.path.join(NATIVE, "third_party", "fonts", f), os.path.join(stage, "assets", "fonts"))
    tv = pal_sim.Tv("greyscale", {"FAKE_TV_VIDEO_FROM_SCREEN": "1"})
    try:
        before = dict(tv.st())
        steps = ["wait:2000", "snap:01-greyscale", "key:ok", "wait:500", 'media:{"loadCompleted":{}}', 'media:{"playing":{}}', "wait:2500", "snap:02-measuring", "wait:60000", "snap:03-results",
                 "key:down", "key:down", "key:right", "wait:300", "snap:04-results-scrolled", "quit"]
        env = dict(tv.env, SDL_VIDEODRIVER="dummy", BLOOB_WINDOW="1920x1080", BLOOB_EFFECTS="full", BLOOB_SNAPSHOT_DIR=OUT,
                   BLOOB_SCRIPT="|".join(steps), BLOOB_HOST_EXEC="1", BLOOB_CAL_BIN=pal_sim.BIN, BLOOB_GREYSCALE_APP="1")
        p = subprocess.run([os.path.join(stage, "bin", "bloob_host")], env=env, capture_output=True, text=True, timeout=200)
        log = p.stdout + p.stderr
        open(os.path.join(OUT, "app.log"), "w").write(log)
        rep = os.path.join(tv.data, "latest-greyscale.json")
        ok = os.path.exists(rep)
        pal_sim.check("greyscale: the app ran bloob-cal greyscale and a report was written", ok)
        if ok:
            r = json.load(open(rep))
            pal_sim.check("greyscale: 201 levels, 0 to 100% in 0.5% steps", len(r["levels"]) == 201 and r["levels"][1]["pct"] == 0.5)
            s = r["summary"]
            pal_sim.check("greyscale: a sane summary", s["white"] > 30 and 1.8 < s["avg_gamma"] < 3.0, json.dumps(s))
            for ext in ("html", "csv"):
                pal_sim.check("greyscale: latest-greyscale.%s written" % ext, os.path.exists(os.path.join(tv.data, "latest-greyscale." + ext)))
        after = tv.st()
        pal_sim.check("greyscale: nothing on the TV changed (no table uploads, no settings written)",
                      after.get("uploads_1d", 0) == before.get("uploads_1d", 0) and after.get("uploads_3d", 0) == before.get("uploads_3d", 0))
        pal_sim.check("greyscale: the black video played for the run", "black video loading" in log)
        pal_sim.check("greyscale: the results screen opened by itself", os.path.exists(os.path.join(OUT, "03-results.bmp")))
    finally:
        tv.close()
    for f in sorted(os.listdir(OUT)):
        if f.endswith(".bmp"):
            src = os.path.join(OUT, f)
            subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-f", "lavfi", "-i", "color=c=0x404040:s=1920x1080", "-i", src, "-filter_complex",
                            "[1:v]format=rgba[ui];[0:v][ui]overlay", "-frames:v", "1", src[:-4] + ".png"])
            os.remove(src)
    bad = [r for r in pal_sim.RESULTS if not r[1]]
    print("\n%d checks, %d failed; pictures in %s" % (len(pal_sim.RESULTS), len(bad), OUT))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
