# Pal in Bloob

Written 30 October 2026. The owner shared a design called **Pal: an agentic OLED calibrator**: a frontier model as planner and operator, a numerical engine for the precision, a meter for the physics. It covers a calibration manifest, characterisation first, a learnt control model, a fixed correction order, an objective with uncertainty, auditable tools, safeguards, and a verification report. This note maps Pal onto what Bloob has, names the gaps, and sets the build order. It follows on from `research/Agentic-calibration-for-Bloob.md`, whose steps A0–A4 it replaces. Nothing here is built yet.

Labels: **[have]** in Bloob 3.3.8; **[part]** partly there; **[gap]** missing; **[limit]** not possible with the owner's equipment; **[tv]** seen on the owner's G2.

## 1. What Bloob already is, in Pal's terms

Most of Pal's lower half exists. That is the good news: Pal is not a rewrite.

| Pal | Bloob today |
| --- | --- |
| Numerical engine; local sensitivity model ΔM ≈ J ΔC from small reversible probes (§4) | **[have]** `cal/grey.cpp` measures J from ± steps on each channel (central differences, because of the WOLED kink) and corrects it with Broyden updates. `cal/colour.cpp` fits the 3D LUT |
| Separate profiles per signal system: SDR BT.1886, HDR10 PQ, HLG (§0, §2) | **[part]** SDR is built and running on the TV. HDR10 (ΔE ITP, matrix LUT) and Dolby Vision are proven on the simulator only. HLG: none |
| Correction order: mode → levels → transfer → grey → gamut → LUT → verify (§5) | **[have]** SDR goes processing off → factory start → Calman bypass → light → 1D grey → 3D colour → verify |
| Verification on patterns not used in the fit (§9) | **[have]** `verification_set(1)` is never used to fit (`sdr.cpp:54`) |
| Measurements repeated when inconsistent; drift checked (§3, §8) | **[part]** The drift sentinel reads white again and rescales, and stops the run below half. There is no per-reading consistency test |
| `restore_snapshot()` | **[part]** Undo puts back the saved settings, the processing and the light, and LG's factory tables. **[limit]** The TV's 3D LUT can't be read, so "original" means factory, not what was there before |
| Pattern generator | **[have]** LG's own generator, over the second-screen socket. **[tv]** It draws only while a video plays, so a video is always under the patterns |
| `set_signal_format()` | **[part]** Here Bloob's quirk becomes a feature: the clip under the patterns *is* the signal format. `black.mp4` puts the TV in SDR. The HDR10 and Dolby Vision carrier clips the plan calls for are not made yet |
| TV control API with readback | **[have]** Settings by luna-send (read back), `externalpq` over one socket, 1D readback. **[limit]** No 3D readback, so the 3D LUT is verified by measurement |

## 2. What Pal adds, and what it means for Bloob

1. **The manifest (§2) [gap].** The target lives in code today: 140 nits, BT.1886, Quick. Pal writes it down first, as a file: target, transfer, white point, viewing conditions, which processing is off, pass limits. It shows the manifest on the Calibrate screen and waits for one OK. That settles the open BT.1886-or-2.2 question properly: Pal asks how and when you watch, proposes a target and gives its reason.
2. **Characterise before correcting (§3) [part].** Check already measures white, black, spread and clipping. Pal adds:
   - a repeatability estimate (the noise floor used later to decide when to stop);
   - near-black steps and crush;
   - highlight steps;
   - **window size and APL**: the same patch at 10% on black and at a larger window, or over a grey background, so an ABL dimming isn't mistaken for a colour error. LG's generator already supports a background box (`show_box`).

   Uniformity means moving the meter, so Pal asks you, or skips it.
3. **Noise-aware stopping (§6) [gap].** Quick stops at a fixed score of 0.3. Pal stops when what's left is within the measured repeatability, and takes smaller steps when readings vary. This is where the simulator's harsh-meter runs failed (5 of 12).
4. **The objective (§6) [part].** The grey score is the average plus a quarter of the worst, and steps are clamped to 0.1. Pal adds weighted near-black and highlight terms and a penalty on large corrections. Most of this is a solver change that Pal tunes per profile.
5. **Safeguards on every action (§8) [part].** These checks don't exist yet:
   - the reading belongs to the pattern shown (a sequence number on each patch; a camera later);
   - the TV is still in Expert 1 with the clip playing (picture mode re-read between stages);
   - the result matches what the model predicted (predicted against measured after every upload; a large miss means restore the checkpoint and measure J again).

   Checkpoints after every stage are new too: today the only restore point is the start.
6. **The report (§9) [gap].** This is the calibration record you asked about:
   - model, firmware, mode and signal;
   - the manifest;
   - the meter and its correction file;
   - baseline and final with uncertainty;
   - every change with its time;
   - pass or fail for each criterion;
   - the panel's limits;
   - the restore point.

   It is saved under `calibration/history/` and shown on the screen.
7. **Calibration database (§1) [part].** The research notes already record this TV's known behaviour (generator needs video, CAL_START sets Warm 50 and Wide, no 3D readback). Pal reads them as its memory, together with every past report.

## 3. What can't be done here, said plainly

- **[limit] Spectroradiometer.** You can't change the meter, so Pal can't profile the Spyder5 against this panel. It uses DisplayCAL's G2 correction file, measured with an i1 Pro 2 on another G2, and the report says so.
- **[limit] Room sensors.** Spotread has an ambient mode for meters that support it; whether this Spyder5 does through the probe's spotread is untested. Pal tries it once; failing that, it asks about the room. There is no temperature sensor; it uses the time since the panel woke instead.
- **[gap] An external pattern source.** Not needed: the TV's own generator over a clip measures the same path films take in Bloob.

## 4. Where the line between model and code goes

Pal's design says the LLM keeps the session state (§7). **In Bloob the state lives in `bloob-cal`, and Pal reads it.** If the network drops, the API stalls or the app is closed mid-run, the program still knows what it changed and can finish the stage or undo it. Pal's context can be lost; the TV's state must not be.

| Pal (the model) decides | `bloob-cal` (code) does |
| --- | --- |
| The manifest, with you | Holds the session: changes, checkpoints, journal |
| What to measure next, and how many reads | Shows patches, reads the meter, reports uncertainty |
| Whether a reading or a stage makes sense | Computes J, the corrections and the tables |
| Retry, more reads, change strategy, restore a checkpoint | Gate: refuses out-of-range or non-monotone tables and settings off the allow-list |
| When it's good enough, and the report's explanation | Undo, the drift sentinel and a time limit work with or without Pal |

If Pal goes quiet for more than a minute, `bloob-cal` finishes the current stage the Quick way, verifies, and leaves a report that says Pal dropped out.

## 5. How it fits into Bloob

```text
Settings → Calibrate → "Calibrate with Pal"
   │  manifest shown → OK
   ▼
Bloob service (Node, on the TV)            bloob-cal serve (root, on the TV)
  pal.js: agent loop, Claude API,   ──JSON──>  tools below; session, journal,
  key in keys.json                  <────────  checkpoints, gate
  narration → Calibrate card                   └─ LG generator + clip, Spyder5
```

Pal's tools, mapped:

| Pal tool | `bloob-cal serve` command |
| --- | --- |
| `identify_display()` | Model, firmware, picture mode and the calibration features that answer (getters) |
| `set_signal_format()` | Play the carrier clip: `black.mp4` (SDR); HDR10 and Dolby Vision clips (to make) |
| `display_pattern()` | `show_box`: code values, window size, background |
| `measure_patch()` | Read n times; returns mean, spread and the patch's sequence number |
| `set_control()` | Setting with readback, allow-listed |
| `upload_lut()` | Upload through the gate; activation confirmed by a measurement |
| `solve_grey/solve_colour()` | The existing solvers, with options and per-level masks |
| `verify_profile()` | The held-out verification set |
| `restore_snapshot()` | A stage checkpoint, or full Undo |

Quick and Thorough stay as they are, for runs without a network and as the yardstick.

## 6. Build order

| Step | What | Done when |
| --- | --- | --- |
| P0 | Claude Code on the owner's PC with SSH; I act as Pal by hand through `bloob-cal` | One SDR run where I make every decision; which tools mattered is written down |
| P1 | `bloob-cal serve`: tools, journal, stage checkpoints, gate, Pal-dropped fallback | Stand-in TV: a scripted Pal completes SDR; a killed Pal still ends in verify or undo |
| P2 | Manifest and report (useful without Pal too) | Every run, Quick included, leaves a report under `calibration/history/` |
| P3 | Characterisation and noise-aware stopping in the engine | Simulator: harsh meter clearly above 5/12 |
| P4 | `pal.js` and "Calibrate with Pal" on the screen | An unattended SDR run on the TV, narrated |
| P5 | HDR10 and Dolby Vision carriers; Pal takes HDR10, then DV | The plan's HDR marks on the TV |
| P6 | Eyes: a phone camera confirms each pattern | Readings tied to what the screen showed |

Success is judged against Quick on the same TV: final grey and colour as watched, minutes, unattended completion, and failures recovered inside the run.

## 7. Pal as the command centre: a hands-off run

Owner, 30 October 2026: Pal should be the brains of the whole operation, so the experience is hands-off. That is the target. Pal decides and recovers; `bloob-cal` holds the state and does the work safely; the Calibrate screen is its face.

**What you do, all of it:**

1. Choose **Calibrate** once. The first time only, Pal asks a few questions about the room, viewing and targets, and remembers the answers.
2. Hang the meter when Pal asks. It guides you to the centre with live readings: it draws the patch at a few places and says "a little left", "good".
3. Take the meter off when Pal says it's finished.

The TV's one-time "allow Bloob" prompt has already been answered. Everything else is Pal's.

**What Pal owns:**

- **Before:** checks the meter, the clip, the picture mode, the processing and the firmware (a new firmware means re-checking what the playbook assumes); warms the panel; waits for stability.
- **During:** one session across every mode in the manifest (Day and Night SDR, then HDR10 and Dolby Vision once built), each with its own checkpoints. Pal may split a long session over two evenings and tells you when.
- **Recovery without you:** every failure met in 3.3.x is a playbook entry Pal acts on:
  - no patches → the clip isn't playing → restart it;
  - socket 500 → fresh connection, then a route test;
  - meter stopped → restart on white, refresh-rate flags;
  - a reading that doesn't fit → read again;
  - a stage that made things worse → restore the checkpoint and try another strategy;
  - `CAL_START` changing colour temperature and gamut → put them back.

  A new failure becomes a new entry, written by Pal into its playbook on the TV, not a release from me.
- **After:** verification, the report, the "remove the meter" message, and the screen switched off so LG's panel care runs (if the TV allows Bloob to do it; not yet verified).
- **Stopping:** only for what it can't do from the TV: the meter fell off, the TV was switched off, the network is gone (`bloob-cal` then finishes the stage the Quick way). It says in one line what it needs.

**Limits that stay:** the meter needs hands twice per run. The viewing check (P-ISF7 in `research/Pal-moonshot.md`) is optional and off unless you ask. Monthly health checks need the meter, so Pal reminds you rather than running them.

**Build path:** P1 (`bloob-cal serve`) and P2 (manifest and report) first, then the recovery playbook from the 3.3.x history, meter centring, then P4 (Pal in the service). Pal needs a Claude API key in `keys.json` on the TV, written by the owner like the other keys.

## 8. The technical layer, and keeping the spend small

### How information reaches Pal

```text
bloob-cal serve (root)  ──events──>  pal.js (Bloob service)  ──HTTPS──>  Claude API
   runs every stage itself          wakes only on an event            one turn: read digest,
   journal, checkpoints, gate  <──tool calls (JSON)──                  decide, call tools
```

- **One local channel.** `bloob-cal serve` listens on a local socket and accepts only the service, using a one-time token written for each session. Commands and replies are single JSON lines.
- **Events, not polling.** `bloob-cal` runs each stage's inner loop (hundreds of readings) by itself and wakes Pal only at a decision point:
  - session start and the manifest;
  - end of each stage, with its result;
  - an anomaly: a reading that doesn't fit, drift, the meter stopping, a pattern error, a stage that made things worse;
  - the end of the run.

  Between events there are no API calls at all.
- **Digests, with detail on request.** An event carries a summary of a few hundred tokens, for example: `grey pass 4: avg 0.42, worst 1.1 at code 108 (5%), white 139.6 cd/m², repeatability 0.3%`. Raw readings stay in the journal. Pal asks for the part it needs (`readings(stage, codes)`) only when a digest raises a question.
- **State lives in the journal, not in the conversation.** If the conversation is lost (a crash, a restart, too much context), `pal.js` starts a new one from the journal's digest and carries on.

### Keeping the spend small

| Lever | What it does |
| --- | --- |
| Call Pal only at decisions | Tens of calls per SDR run (estimate 50–150), never one per reading |
| Prompt caching | The fixed part (instructions, playbook, tool definitions, about 20k tokens) is cached with a one-hour lifetime, because gaps between events can pass five minutes. The growing conversation is cached automatically. Cache reads cost a twentieth of normal input on the default model |
| A stable prefix | Nothing that changes (times, readings) goes in the instructions or tool list, so the cache isn't broken by accident |
| Short replies | Pal answers with tool calls and one line for the screen, not essays. Effort is medium by default and raised only for a diagnosis |
| A hard cap per run | `pal.js` adds up the cost of every call from the API's own usage figures. At the cap, Pal hands the run to Quick, which finishes it the normal way and says so in the report. Nothing fails because of the cap |
| Measured, not guessed | Every call's tokens and cost go into the run report, so the second run's budget comes from the first run's numbers |

**Estimate before measuring** (list prices as of October 2026, the default Opus model: $4 per million input tokens, $20 output, $0.20 cached reads): about $0.03–0.06 per Pal turn, so roughly **$3–8 for an SDR run** (100 turns). Night, HDR10 and Dolby Vision cost about the same again each. The default cap is $15 per run. A smaller model is cheaper, but the choice of model is the owner's.

**Not used:** the Batch API (half price, but answers can take hours) and anything that puts the model inside the reading loop.

### Plan of action

| Step | Work | Where it's proven |
| --- | --- | --- |
| T0 | Read the TV's Node version: the official Anthropic SDK for Node needs a recent Node. If the TV's is too old, `pal.js` runs on a newer Node shipped with Bloob's service | One read-only command on the TV |
| T1 | `bloob-cal serve`: socket and token, the event stream, digests, journal, checkpoints after each stage, the write gate, a Quick fallback when Pal is silent | Stand-in TV and simulator: a scripted Pal (no API) completes SDR; killing it mid-run still ends in verify or undo |
| T2 | The run report and manifest, for every run, Quick included | Stand-in TV |
| T3 | Known-failure recovery from 3.3.x as code; the playbook text Pal reads | Stand-in TV with each failure injected (`FAKE_TV_*`) |
| T4 | `pal.js`: the loop, caching, cost meter and cap, conversation rebuilt from the journal; key from `keys.json` | Desktop against the stand-in TV with the real API (a few dollars, with the owner's approval) |
| T5 | Settings → Pal → SDR calibration; Pal's line on the card; meter-centring check | Desktop tour, then the TV |
| T6 | First Pal run on the TV | The report: result, minutes, turns and cost |

## 9. SDR grey, the simple flow, and Pal's goal (owner, 30 October 2026)

The owner's own test: HCFR greys at 0–100% in 5% steps, the PDF given to a language model, a 1D LUT made from the deviations. Pal's first job is that flow, hands-off: report → Claude → 1D LUT → push → confirm loaded → after report.

### The goal (the contract Pal works to)

Measured as ΔE2000 relative to the target white (Rec.709 D65, BT.1886, the chosen white), at 21 grey levels:

| Rule | Value |
| --- | --- |
| Levels that count | 10–100%. 0% isn't scored (OLED black reads 0). 5% (about 0.1 cd/m² at 140 nits) is reported but not scored, because it is below what the Spyder5 reads reliably |
| **Goal** | Average ≤ **0.5**, and **every** counted level ≤ **1.0**. White luminance within 2% of target |
| Why that number | `CALIBRATION-PLAN.md` §1's SDR mark; a community report puts LG AutoCal at 0.3–0.4 average, worst under 1; the owner's previous tool reached about 0.8. Around 1 is commonly treated as the smallest grey difference a trained eye can see |
| Floor | No counted level ends worse than **before** by more than the meter's repeatability. If Pal can't beat the starting point, it puts the starting point back |
| Stop | When the goal is met; or when what's left is within the meter's repeatability (measured, three reads of white and of 50% at the start); or after **3 passes** |
| Staying within | Every run starts with a 21-level report (about 5 minutes). If it is within the goal, nothing is written. A monthly recheck uses the same report |

The numbers are what the Spyder5, with its G2 correction file, reads. The report says so.

### A second pass: diagnose first, then keep or clear

Pal reads the after report and finds out why before acting:

| What the after report shows | Likely cause | What Pal does |
| --- | --- | --- |
| Readback differs from what was sent | The table didn't load | Push the same table again; if it fails twice, stop and undo |
| Every level moved the same way, white included | White itself moved (light, the meter shifted, the panel warming) | Re-read white and 50% to see whether it is the meter or the panel; re-centre or wait; then measure again with the same table |
| Better, but some levels over- or under-shot | The WOLED white sub-pixel: colours interact | Keep the correction and refine from the after report, with smaller steps at those levels |
| Better, but the dark levels wander | Meter noise | More reads there; don't chase what's within repeatability |
| Worse than before anywhere | The correction was wrong | Clear to the start, redo with the cause named in the report |

**The default is to build on the table in the TV:** read it back, measure the screen with it loaded, and add the correction for what is still off to that table (new table = table in the TV + correction). Each pass is a better version of the last one. Clearing back to the starting table happens only when a pass made a level worse than before.

## 10. Refining the grey flow: definition and rubric

**Refined means:** the flow reaches the goal (§9) every time, never leaves the TV worse, proves what it did, and does so in the fewest minutes, readings and owner actions. It explains each decision well enough that a calibrator would agree with it.

**How it is refined:** every run is scored against the rubric below. Change one thing at a time, aimed at the lowest score; run again; keep the change only if that score rises and none falls. Fault handling and repeatability are scored on the stand-in TV first, the result on the TV.

| # | Dimension | 3 (refined) | 2 | 1 | 0 | How it is scored |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Result** | Average ≤ 0.5, every level 10–100% ≤ 1.0, white within 2% | Average ≤ 0.8, max ≤ 1.5 | Better than before at every level | Not better | Final after report |
| 2 | **Never worse** (gate: must be 3) | No level worse than before beyond repeatability; any failure ends at the starting table and settings | — | — | Any level worse, or the TV left changed after a failure | Before vs after; the undo log |
| 3 | **Proof** | Readback equals the table sent, and the after report is measured in the viewing state (outside calibration mode) | Readback only | Measured only | Neither | Report |
| 4 | **Repeatability** | Two runs the same evening: averages within 0.2, every level within 0.4 | Within 0.3 / 0.6 | Within 0.5 / 1.0 | Worse | Two runs, scored by the report |
| 5 | **Time** | ≤ 15 min to the goal | ≤ 30 min | ≤ 60 min | Longer | Report timestamps (warm-up counted separately) |
| 6 | **Passes** | Goal met in 1 | 2 | 3 | Not met in 3 | Report |
| 7 | **Hands-off** | Owner: hang the meter, one press, remove the meter; nothing mid-run | One mid-run request with a clear reason | Two | More, or unclear requests | Count of prompts to the owner |
| 8 | **Diagnosis** | Every missed pass names a cause the evidence supports (readback, white shift, overshoot, noise), and the action follows §9's table | One cause unsupported | Causes named, actions don't follow | No diagnosis | Review of the report by me and the owner |
| 9 | **Fault handling** | All injected faults recovered or cleanly undone: meter stall, video drop, socket 500, table not loaded, meter moved | All but one | Half | Fewer | Stand-in TV with `FAKE_TV_*` faults |
| 10 | **Report** | Before/after per level, each pass with its reason, tables, meter and correction file, time, cost or usage; plain English | One part missing | Numbers only | No report | Checklist |
| 11 | **Usage** | Claude Code: a small share of the Pro plan's limits per run (later, API: under the per-run cap) | — | — | Run stopped by limits | Session usage; later the API's usage figures |

**First refinements to try, in the order the rubric is likely to ask for them:**

1. White first: set light so 100% reads the target before the grey correction (dimensions 1 and 6).
2. A warm-up and repeatability gate at the start (dimensions 2 and 4).
3. Gentler steps at levels that overshot in pass 1 (dimension 6).
4. A second reading only where a level is near its limit (dimensions 1 and 5).
5. The measured 100% red, green and blue fed into the correction maths (dimensions 1 and 6).
