#!/usr/bin/env python3
"""Pal's simulated environment: bloob-cal's Pal commands against the stand-in TV
(tools/fake-tv/fake_tv.py), driven by a scripted Pal that follows native/PAL-PLAYBOOK.md,
across a happy path and every fault the plan names. Each scenario is scored on the
rubric (research/Pal-in-Bloob.md §10) where it applies. Not shipped.

  python3 tools/pal-sim/pal_sim.py [build-host/bloob-cal] [scenario ...]

Exit 0 when every scenario passes.
"""
import json, os, shutil, signal, socket, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
FAKE = os.path.join(HERE, "..", "fake-tv", "fake_tv.py")
BIN = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("-") and os.path.exists(sys.argv[1])
                      else os.path.join(HERE, "..", "..", "build-host", "bloob-cal"))
ONLY = [a for a in sys.argv[1:] if not os.path.exists(a)]
NITS = 140.0
TV_READ_S, TV_PATTERN_S = 3.1, 2.0  # the owner's TV: seconds per reading (Check, 3.3.6), pattern delay (B1)


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


class Tv:
    """One stand-in TV: its state file, socket server and the command shims bloob-cal runs."""

    def __init__(self, name, extra=None):
        self.dir = tempfile.mkdtemp(prefix="pal-sim-%s-" % name)
        self.data = os.path.join(self.dir, "data")
        self.probe = os.path.join(self.dir, "probe")
        os.makedirs(self.data)
        os.makedirs(self.probe)
        self.state = os.path.join(self.dir, "state.json")
        py = sys.executable
        for shim, part in (("spotread", "spotread"), ("luna-send", "luna-send")):
            path = os.path.join(self.probe, shim)
            with open(path, "w") as f:
                f.write("#!/bin/sh\nexec %s %s %s \"$@\"\n" % (py, os.path.abspath(FAKE), part))
            os.chmod(path, 0o755)
        with open(os.path.join(self.probe, "itpg_manifest.json"), "w") as f:
            f.write('{"manifestVersion": 1, "appId": "com.lge.test"}')
        self.port = free_port()
        self.env = dict(os.environ)
        self.env.update({
            "BLOOB_CAL_ANY_USER": "1", "BLOOB_PROBE": self.probe, "BLOOB_LOADER": "none", "BLOOB_CAL_DATA": self.data,
            "BLOOB_LUNA_SEND": os.path.join(self.probe, "luna-send"), "BLOOB_SSAP_PORT": str(self.port),
            "FAKE_TV_STATE": self.state, "FAKE_TV_PORT": str(self.port), "FAKE_TV_SEED": "1",
            "BLOOB_CAL_FAST": "1", "BLOOB_PAL_WARMUP_INTERVAL_S": "0",
        })
        self.env.update(extra or {})
        for k in [k for k, v in self.env.items() if v is None]:
            del self.env[k]  # None: not set at all (an empty value still counts as set in C)
        subprocess.run([py, FAKE, "luna-send", "-n", "1", "luna://com.webos.settingsservice/getSystemSettings",
                        '{"category":"picture","keys":["pictureMode"]}'], env=self.env, capture_output=True)  # creates the state file
        self.server = subprocess.Popen([py, FAKE, "ssap"], env=self.env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(50):
            try:
                socket.create_connection(("127.0.0.1", self.port), 0.2).close()
                break
            except OSError:
                time.sleep(0.05)

    def run(self, *args, env=None, timeout=900):
        e = dict(self.env)
        e.update(env or {})
        for k in [k for k, v in e.items() if v is None]:
            del e[k]
        p = subprocess.run([BIN] + list(args), env=e, capture_output=True, text=True, timeout=timeout)
        result = None
        for line in p.stdout.splitlines():
            if line.startswith("PAL "):
                result = json.loads(line[4:])
        return p.returncode, result, p.stdout

    def st(self):
        with open(self.state) as f:
            return json.load(f)

    def patch_state(self, **kw):
        s = self.st()
        s.update(kw)
        tmp = self.state + ".h"
        with open(tmp, "w") as f:
            json.dump(s, f)
        os.replace(tmp, self.state)

    def close(self):
        self.server.terminate()
        self.server.wait()
        if not os.environ.get("PAL_SIM_KEEP"):
            shutil.rmtree(self.dir, ignore_errors=True)
        else:
            print("    (kept %s)" % self.dir)


def scripted_pal(tv, log, damp_later=0.8, max_passes=3):
    """Pal's grey flow as PAL-PLAYBOOK.md tells it: start, report, then up to three passes of
    propose → apply (on top of the table in the TV) → read the after report → decide."""
    out = {"passes": 0, "readings": 0, "patches": 0, "prompts": 0, "events": []}
    rc, r, _ = tv.run("pal-start", str(int(NITS)))
    out["start"] = (rc, r)
    if rc != 0:
        out["events"].append("pal-start refused: %s" % (r or {}).get("error"))
        return out
    rc, rep, _ = tv.run("grey-report")
    if rc != 0:
        out["events"].append("grey-report failed: %s" % (rep or {}).get("error"))
        return out
    out["before"] = rep["summary"]
    out["readings"] += rep["readings"]
    now = rep
    for p in range(1, max_passes + 1):
        if now["summary"]["goal_met"]:
            break
        if now["summary"]["white_err_pct"] < -1.0:
            rc, lr, _ = tv.run("pal-light", "%.1f" % (NITS * 1.01))
            out["events"].append("pass %d: white %.1f%% short, light %s" % (p, now["summary"]["white_err_pct"], (lr or {}).get("light")))
            rc, now, _ = tv.run("grey-report")
            if rc != 0:
                out["events"].append("grey-report failed: %s" % (now or {}).get("error"))
                return out
            out["readings"] += now["readings"]
            if now["summary"]["goal_met"]:
                break
        rc, prop, _ = tv.run("pal-propose", os.path.join(tv.data, "pal", "latest-report.json"), "1.0" if p == 1 else str(damp_later))
        if rc != 0:
            out["events"].append("pal-propose failed: %s" % (prop or {}).get("error"))
            return out
        rc, ap, _ = tv.run("apply-1d", os.path.join(tv.data, "pal", "latest-proposal.json"))
        out["passes"] = p
        out.setdefault("applied", []).append((rc, ap))
        if rc != 0:
            out["events"].append("apply-1d exit %d: %s" % (rc, (ap or {}).get("error")))
            return out
        out["readings"] += ap["after"]["readings"]
        if ap["worse_than_start"]:
            out["events"].append("pass %d worse than start at %s: clearing" % (p, ap["worse_than_start"]))
            tv.run("apply-1d", "--start")
            return out
        now = ap["after"]
    out["after"] = now["summary"]
    out["after_report"] = now
    return out


def score(run, tv):
    """The rubric's automatic dimensions (research/Pal-in-Bloob.md §10)."""
    s = {}
    a = run.get("after")
    if a:
        s["1 result"] = 3 if a["goal_met"] else 2 if a["avg"] <= 0.8 and a["max"] <= 1.5 else 1 if a["avg"] < run["before"]["avg"] else 0
    st = tv.st()
    s["3 proof"] = 3 if st.get("reads_in_session", 0) == 0 and all(x[0] == 0 and x[1]["loaded"] for x in run.get("applied", [])) else 0
    readings = st.get("reads_total", 0)
    minutes = (readings * TV_READ_S + (run["readings"] // 2 + 10) * TV_PATTERN_S) / 60
    run["tv_minutes"] = minutes
    s["5 time"] = 3 if minutes <= 15 else 2 if minutes <= 30 else 1 if minutes <= 60 else 0
    if a:
        p = run["passes"]
        s["6 passes"] = 3 if a["goal_met"] and p <= 1 else 2 if a["goal_met"] and p == 2 else 1 if a["goal_met"] else 0
    s["7 hands-off"] = 3 if run["prompts"] == 0 else 2
    return s


RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, ok, detail))
    print("  %s  %s%s" % ("PASS" if ok else "FAIL", name, (" — " + detail) if detail else ""))
    return ok


def factory_and_settings_back(tv):
    st = tv.st()
    pic = st["settings"]["picture"]
    return (st["lut1d"] is None and pic.get("energySaving") == "min" and pic.get("dynamicContrast") == "low"
            and st["settings"]["aiPicture"].get("ai_Picture") == "on" and st["session"] is None)


# ---- Scenarios ------------------------------------------------------------------------------

def sc_happy():
    """The flow end to end: goal met, nothing measured inside calibration mode, readback proven."""
    tv = Tv("happy")
    try:
        run = scripted_pal(tv, [])
        for e in run["events"]:
            print("    " + e)
        ok = check("happy: pal-start ready", run["start"][0] == 0, json.dumps(run["start"][1].get("index") if run["start"][1] else None))
        if not ok:
            return
        b, a = run["before"], run.get("after")
        check("happy: goal met (avg <= 0.5, every level <= 1.0, white within 2%)", bool(a and a["goal_met"]),
              "before %.2f/%.2f -> after %.2f/%.2f in %d pass(es)" % (b["avg"], b["max"], a["avg"], a["max"], run["passes"]) if a else str(run["events"]))
        st = tv.st()
        check("happy: no reading taken inside calibration mode (B3)", st.get("reads_in_session", 0) == 0, "%d of %d" % (st.get("reads_in_session", 0), st.get("reads_total", 0)))
        check("happy: every table read back as sent", all(x[1]["loaded"] for x in run.get("applied", [])))
        check("happy: viewing colour temperature and gamut kept", st["settings"]["picture"]["colorTemperature"] == "-50" and st["settings"]["picture"]["colorGamut"] == "auto")
        check("happy: every write carried picMode", not st.get("missing_picmode"), str(st.get("missing_picmode")))
        sc = score(run, tv)
        print("    rubric: %s; about %.0f min on the TV" % (", ".join("%s %d" % kv for kv in sorted(sc.items())), run["tv_minutes"]))
        check("happy: rubric result, proof, hands-off at 3", sc.get("1 result") == 3 and sc["3 proof"] == 3 and sc["7 hands-off"] == 3)
        rc, end, _ = tv.run("pal-end", "ok")
        check("happy: pal-end keeps it and files the session under history/", rc == 0 and end["kept"] and os.path.isdir(end["history"])
              and os.path.exists(os.path.join(end["history"], "report-1.json")))
        check("happy: the corrected table stays in the TV", tv.st()["lut1d"] is not None)
    finally:
        tv.close()


def sc_repeat():
    """Rubric 4: two runs from the same start agree."""
    finals = []
    for seed in ("11", "12"):
        tv = Tv("repeat", {"FAKE_TV_SEED": seed})
        try:
            run = scripted_pal(tv, [])
            if run.get("after_report"):
                finals.append(run["after_report"])
        finally:
            tv.close()
    if not check("repeat: both runs finished", len(finals) == 2):
        return
    a, b = finals
    davg = abs(a["summary"]["avg"] - b["summary"]["avg"])
    dlev = max(abs(x["de"] - y["de"]) for x, y in zip(a["levels"], b["levels"]) if x["scored"])
    check("repeat: averages within 0.2, every level within 0.4 (rubric 4 at 3)", davg <= 0.2 and dlev <= 0.4, "avg %.3f apart, worst level %.3f" % (davg, dlev))


def sc_code_index():
    """A TV whose 1D table follows the 10-bit code: pal-start finds out, and the flow still meets the goal."""
    tv = Tv("code", {"FAKE_TV_1D_INDEX": "code"})
    try:
        run = scripted_pal(tv, [])
        idx = run["start"][1].get("index") if run["start"][1] else None
        check("code index: detected", idx == "code", str(idx))
        a = run.get("after")
        check("code index: goal met", bool(a and a["goal_met"]), "after %.2f/%.2f" % (a["avg"], a["max"]) if a else str(run["events"]))
    finally:
        tv.close()
    tv = Tv("signal")
    try:
        rc, r, _ = tv.run("pal-start", "140")
        check("signal index: detected", rc == 0 and r["index"] == "signal", str(r and r.get("index")))
    finally:
        tv.close()


def sc_flicker():
    """B1: LG's pink transition after each pattern change. With the 2 s delay (scaled) it never
    reaches a reading; flashcheck sees it when reading at once; with too short a delay the
    sanity check catches it and reads again."""
    real = {"BLOOB_CAL_FAST": None, "FAKE_TV_FLICKER_S": "0.35", "BLOOB_PATTERN_DELAY_MS": "500"}
    tv = Tv("flicker", real)
    try:
        rc, fc, _ = tv.run("flashcheck")
        check("flicker: flashcheck sees it when reading at once", rc == 0 and fc["flicker_seen"], json.dumps(fc))
        run = scripted_pal(tv, [])
        a = run.get("after")
        check("flicker: with the delay, the flow meets the goal", bool(a and a["goal_met"]), "after %.2f/%.2f" % (a["avg"], a["max"]) if a else str(run["events"]))
        log = open(os.path.join(tv.data, "pal", "session.log")).read()
        check("flicker: with the delay, no reading needed taking again", "reading again" not in log)
    finally:
        tv.close()
    tv = Tv("flicker-short", dict(real, BLOOB_PATTERN_DELAY_MS="40"))
    try:
        tv.run("pal-start", "140", env={"BLOOB_PATTERN_DELAY_MS": "500"})
        rc, rep, out = tv.run("grey-report")
        caught = out.count("reading again")
        flagged = [l["pct"] for l in rep["levels"] if l["suspect"]] if rep else []
        check("flicker: too short a delay is caught by the sanity check (read again)", rc == 0 and caught > 0, "%d re-reads, suspect %s" % (caught, flagged))
    finally:
        tv.close()


def sc_warmup():
    """B4: a panel still warming up; pal-start waits until white holds steady."""
    tv = Tv("warm", {"FAKE_TV_WARMUP_S": "1.5", "BLOOB_PAL_WARMUP_INTERVAL_S": "1"})
    try:
        tv.patch_state(power_on=time.time())
        rc, r, out = tv.run("pal-start", "140")
        check("warm-up: waited for a steady white", rc == 0 and r["warmup_readings"] >= 4, "%s white readings" % (r or {}).get("warmup_readings"))
    finally:
        tv.close()
    tv = Tv("warm-timeout", {"FAKE_TV_WARMUP_S": "4", "BLOOB_PAL_WARMUP_INTERVAL_S": "1", "BLOOB_PAL_WARMUP_MAX_S": "2"})
    try:
        tv.patch_state(power_on=time.time())
        rc, r, _ = tv.run("pal-start", "140")
        st = tv.st()
        check("warm-up: never steady -> refuses, processing put back, nothing written", rc == 6 and st.get("uploads_1d", 0) == 0
              and st["settings"]["picture"].get("energySaving") == "min", (r or {}).get("error", ""))
    finally:
        tv.close()


def sc_meter_placement():
    """The meter off the squares, and near the middle window's edge."""
    for name, at, expect in (("off the squares", "300,300", "squares"), ("near the left edge", "1380,1080", "left")):
        tv = Tv("meter", {"FAKE_TV_METER_AT": at})
        try:
            rc, r, _ = tv.run("pal-start", "140")
            st = tv.st()
            check("meter %s: refused before anything changed" % name, rc == 6 and st.get("uploads_1d", 0) == 0
                  and st["settings"]["picture"].get("energySaving") == "min" and expect in (r or {}).get("error", ""), (r or {}).get("error", ""))
        finally:
            tv.close()


def sc_no_video():
    tv = Tv("novideo", {"FAKE_TV_NO_VIDEO": "1"})
    try:
        rc, r, _ = tv.run("pal-start", "140")
        check("no video: refused, nothing changed", rc == 6 and tv.st().get("uploads_1d", 0) == 0, (r or {}).get("error", ""))
    finally:
        tv.close()


def sc_socket_500():
    tv = Tv("ssap500", {"FAKE_TV_SSAP_500_COUNT": "1"})
    try:
        run = scripted_pal(tv, [])
        a = run.get("after")
        check("socket 500 once: retried on a fresh connection, goal met", bool(a and a["goal_met"]), str(run["events"]))
    finally:
        tv.close()


def sc_not_loaded():
    """A table the TV accepts and doesn't load."""
    tv = Tv("ignore-start", {"FAKE_TV_IGNORE_1D": "1"})
    try:
        rc, r, _ = tv.run("pal-start", "140")
        check("table ignored from the start: index check refuses and undoes", rc == 6 and factory_and_settings_back(tv), (r or {}).get("error", ""))
    finally:
        tv.close()
    tv = Tv("ignore-apply")
    try:
        tv.run("pal-start", "140")
        tv.run("grey-report")
        tv.run("pal-propose")
        tv.patch_state(ignore_1d=True)  # the socket server reads it from the state (its environment is fixed)
        rc, r, _ = tv.run("apply-1d", os.path.join(tv.data, "pal", "latest-proposal.json"))
        tv.patch_state(ignore_1d=False)
        ok = check("table not loaded: apply-1d says so (exit 5), after two tries", rc == 5 and tv.st()["uploads_1d"] == 5, "exit %d, %s" % (rc, (r or {}).get("error", "")))
        rc, e, _ = tv.run("pal-end", "undo")
        check("table not loaded: pal-end undo restores factory and settings", rc == 0 and e["undone"] and factory_and_settings_back(tv))
    finally:
        tv.close()


def sc_meter_stall():
    """The meter stops answering mid-report: restarted on a white square, and the report finishes."""
    tv = Tv("stall")
    try:
        tv.run("pal-start", "140")
        n = tv.st()["reads_total"]
        rc, rep, out = tv.run("grey-report", env={"FAKE_TV_METER_STALL_AFTER": str(n + 12)})
        check("meter stall: recovered, report finished", rc == 0 and rep["summary"]["white_Y"] > 100 and tv.st().get("stalled_once"), (rep or {}).get("error", ""))
    finally:
        tv.close()


def sc_meter_moved():
    """The meter slips off after a table went in: the after report refuses, Pal undoes."""
    tv = Tv("moved")
    try:
        tv.run("pal-start", "140")
        tv.run("grey-report")
        tv.run("pal-propose")
        tv.patch_state(meter_at=[300, 300])
        rc, r, _ = tv.run("apply-1d", os.path.join(tv.data, "pal", "latest-proposal.json"))
        check("meter moved: after report refuses (meter no longer sees the squares)", rc == 2 and "no longer sees" in (r or {}).get("error", ""), (r or {}).get("error", ""))
        tv.patch_state(meter_at=None)
        rc, e, _ = tv.run("pal-end", "undo")
        check("meter moved: undo restores factory and settings", rc == 0 and factory_and_settings_back(tv))
    finally:
        tv.close()
    tv = Tv("video-drop")
    try:
        tv.run("pal-start", "140")
        tv.patch_state(video=False)
        rc, r, _ = tv.run("grey-report")
        check("video drops: report refuses, nothing written", rc == 2 and "no longer sees" in (r or {}).get("error", "") and tv.st()["uploads_1d"] == 3, (r or {}).get("error", ""))
    finally:
        tv.close()


def sc_gate():
    """Pal's own adjustment refused by the gate, and one made for another table."""
    tv = Tv("gate")
    try:
        tv.run("pal-start", "140")
        rc, rep, _ = tv.run("grey-report")
        uploads = tv.st()["uploads_1d"]
        bad = {"pct": list(range(0, 101, 5)), "r": [1.0] * 21, "g": [1.0] * 21, "b": [1.0] * 21}
        bad["g"][10] = 0.7  # a 30% dip at 50%: the table would fall
        path = os.path.join(tv.dir, "bad.json")
        json.dump(bad, open(path, "w"))
        rc, r, _ = tv.run("apply-1d", path)
        check("gate: a falling table is refused, nothing written", rc == 4 and tv.st()["uploads_1d"] == uploads, (r or {}).get("error", ""))
        bad["g"][10] = 2.0
        json.dump(bad, open(path, "w"))
        rc, r, _ = tv.run("apply-1d", path)
        check("gate: a multiplier outside 0.5-1.5 is refused", rc == 4, (r or {}).get("error", ""))
        stale = {"pct": list(range(0, 101, 5)), "r": [1.0] * 21, "g": [1.0] * 21, "b": [0.99] * 21, "for_table": "0000000000000000"}
        json.dump(stale, open(path, "w"))
        rc, r, _ = tv.run("apply-1d", path)
        check("gate: an adjustment made for another table is refused", rc == 4 and "another table" in (r or {}).get("error", ""), (r or {}).get("error", ""))
    finally:
        tv.close()


def sc_clear_and_stop():
    """apply-1d --start puts the start table back; a stopped report leaves the TV unchanged."""
    tv = Tv("clear")
    try:
        rc, s0, _ = tv.run("pal-start", "140")
        tv.run("grey-report")
        tv.run("pal-propose")
        tv.run("apply-1d", os.path.join(tv.data, "pal", "latest-proposal.json"))
        rc, r, _ = tv.run("apply-1d", "--start")
        check("clear: apply-1d --start loads the start table again", rc == 0 and r["to"] == s0["start_table"], "%s vs %s" % ((r or {}).get("to"), s0["start_table"]))
        e = dict(tv.env)
        p = subprocess.Popen([BIN, "grey-report"], env=e, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        time.sleep(1.0)
        p.send_signal(signal.SIGTERM)
        p.wait(timeout=60)
        st = tv.st()
        check("stop: a stopped report exits and leaves patterns off and calibration mode closed", p.returncode != 0 and not st["on"] and st["session"] is None,
              "exit %s" % p.returncode)
        rc, e2, _ = tv.run("pal-end", "undo")
        check("stop: pal-end undo afterwards restores factory and settings", rc == 0 and factory_and_settings_back(tv))
    finally:
        tv.close()


def sc_one_session():
    tv = Tv("twice")
    try:
        tv.run("pal-start", "140")
        rc, r, _ = tv.run("pal-start", "140")
        check("one session at a time: a second pal-start is refused", rc == 6 and "already open" in (r or {}).get("error", ""))
        rc, r, _ = Tv.run(tv, "grey-report")
        tv.run("pal-end", "ok")
        rc, r, _ = tv.run("grey-report")
        check("no session: grey-report refuses", rc == 6)
    finally:
        tv.close()


def sc_legacy():
    """Check and Test still pass on the new stand-in TV (with the pattern delay)."""
    tv = Tv("legacy")
    try:
        log = lambda: open(os.path.join(tv.data, "last-run.log")).read()  # the older commands write their own log
        rc, _, _ = tv.run("check")
        out = log()
        check("legacy: check passes", rc == 0 and "Nothing was changed" in out, out[-300:] if rc else "")
        rc, _, _ = tv.run("test")
        out = log()
        check("legacy: Expert 2 test passes", rc == 0 and " 0 failed" in out, out[-600:] if rc else out[out.rfind("=="):].strip())
    finally:
        tv.close()


SCENARIOS = [sc_happy, sc_repeat, sc_code_index, sc_flicker, sc_warmup, sc_meter_placement, sc_no_video, sc_socket_500,
             sc_not_loaded, sc_meter_stall, sc_meter_moved, sc_gate, sc_clear_and_stop, sc_one_session, sc_legacy]

if __name__ == "__main__":
    t0 = time.time()
    for sc in SCENARIOS:
        if ONLY and sc.__name__[3:] not in ONLY:
            continue
        print("== %s: %s" % (sc.__name__[3:], sc.__doc__.strip().splitlines()[0] if sc.__doc__ else ""))
        try:
            sc()
        except Exception as ex:  # a crash is a failure, with its reason
            import traceback
            traceback.print_exc()
            check("%s: ran without crashing" % sc.__name__[3:], False, repr(ex))
    bad = [r for r in RESULTS if not r[1]]
    print("\n%d checks, %d failed (%.0f s)" % (len(RESULTS), len(bad), time.time() - t0))
    for name, _, detail in bad:
        print("  FAILED: %s %s" % (name, detail))
    sys.exit(1 if bad else 0)
