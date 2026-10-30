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
