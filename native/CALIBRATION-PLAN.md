# Bloob Calibration: plan (27 October 2026)

One run, no decisions, SDR then HDR10 then Dolby Vision, on the owner's LG OLED55G2 with a Datacolor Spyder5. Planned as if engineering time were free: the order is set by risk and evidence, not by effort. Nothing here is built yet.

Evidence labels: **[probe]** the owner's Spyder USB Probe 0.6.1, read 27 October 2026; **[bsc]** bscpylgtv's documentation (MIT, chros73/bscpylgtv, read 27 October 2026); **[owner]** the owner's report; **[plan]** a design decision; **[unknown]** to be settled on the TV before anything depends on it.

## 1. The goal, measurably

The owner hangs the meter, presses **Calibrate**, and walks away. Bloob returns with every mode calibrated, verified on patches it did not calibrate on, and a plain report.

| Mode family | Picture mode written | Target | Pass (verification set) |
| --- | --- | --- | --- |
| SDR | Expert (Dark) `expert2` | Rec.709, D65, BT.1886 or 2.4 power (owner choice), white 100 cd/m² by default | Grey ΔE2000 average ≤ 0.5, max ≤ 1.0; colour average ≤ 1.0, max ≤ 2.0 |
| HDR10 | Cinema `hdr_cinema` | BT.2020 container, PQ (ST 2084) to the panel's measured peak, D65; LG tone mapping set to the measured peak | ΔE ITP (BT.2124) average ≤ 3, grey ≤ 2 up to 90% of peak |
| Dolby Vision | Cinema `dolby_cinema_dark` | Dolby's own mapping fed the panel's real primaries and peak; grey scale corrected | ΔE ITP grey ≤ 2, white within 0.002 xy of D65 |

ΔE ITP is the industry measure for HDR; ΔE2000 for SDR. The owner's previous tool reached about 0.8 ΔE on white balance [owner]; that is the floor to beat.

## 2. What is known

- **The meter works on the TV** [probe]. Argyll CMS 2.0.1 `spotread`, built for ARM, runs as root through the Homebrew Channel's `exec` service and reads the Spyder5 (USB `085c:0500`). The probe runs one `spotread -e -O` per patch, which re-initialises the meter every time.
- **The internal test pattern generator (iTPG) works** [probe]. The probe registers with the TV's local second-screen socket (`ws://127.0.0.1:3000`) using LG's signed test manifest, then sends `ssap://externalpq/setExternalPqData` with `PTN_CTRL` and `PTN_SINGLE_WINBOX_ATTR`: a full-screen black box plus a coloured window, in limited-range 10-bit values (64–940). The probe's report used browser-drawn patches instead; its iTPG path was never wired to the report.
- **Calibration API** [bsc], on LG α7/α9 sets, tested by its authors on 2019/2021/2022 α9 (the G2 is a 2022 α9):
  - `start_calibration <mode>` / `end_calibration`; every write happens between them.
  - `set_oled_light`, `set_contrast`, `set_brightness`, `set_color`.
  - 1D LUT (`upload_1d_lut`, `.cube` or Argyll `.cal`); 3D LUT, 33 points on α9, separate `bt709` and `bt2020` slots chosen by the signal's colour flags; Dolby Vision uses the BT.709 slot.
  - 3×3 gamut matrices, de-gamma and re-gamma (`set_1d_en_2_2`, `set_1d_en_0_45`) only with a unity or custom 3D LUT.
  - `set_tonemap_params <mode> <peak> <mastering1> <rolloff1> <mastering2> <rolloff2> <mastering3> <rolloff3>` for HDR10 modes. In calibration mode HDR10 tone mapping is bypassed.
  - Bypass ("DDC reset") per family, and `reset_factory_data_sdr|hdr10|dovi`.
  - Getters for 1D/3D LUTs and matrices (backup), "partially or completely broken depending on firmware".
  - `set_dolby_vision_config_data` is **not recommended on 2020+ models**. The safe route is `DolbyVision_UserDisplayConfiguration.txt` on a USB stick, accepted by a pop-up when Dolby Vision plays. That pop-up is disabled once calibration data has been written to that Dolby Vision mode, so config comes first.
  - Each family's modes are reachable only while the TV receives that family's signal: to calibrate HDR10 modes the TV must be playing HDR10; likewise Dolby Vision.
  - iTPG wants limited-range 10-bit in HDR10 (2020+), full-range 10-bit (0–1023) in Dolby Vision.
  - A warning, quoted for the record: "Messing with the calibration data COULD brick your TV in some circumstances". The documented commands are described as safe.
- **Bloob already plays HDR10 and Dolby Vision** through its own engine [owner reading: `hdrType DolbyVision`, 3840×2160, 23.976], so it can put the TV into each signal family itself, with clips it generates.

## 3. Architecture

```
Bloob app (Calibrate screen) ── progress, one button, report
        │  /api/calibrate/* (start, status, cancel, undo, history)
Bloob service
  calibrate/orchestrator   state machine, checkpoints, resume, timing
  calibrate/meter          one long-lived spotread session (root, via Homebrew exec)
  calibrate/patterns       iTPG over the local SSAP socket (Bloob's own pairing)
  calibrate/tv             calibration API + settings service, command allow-list
  calibrate/signal         asks the engine to play the HDR10 / Dolby Vision carrier clip
  calibrate/solver         panel model, 1D and 3D LUT synthesis, prediction (native helper)
  calibrate/verify         independent patch sets, ΔE2000 / ΔE ITP
  calibrate/store          backups, results, history, the meter correction
```

- **The service owns the run** [plan]: it outlives the app's screens, the app only shows progress, and a crash of the UI cannot leave the TV half-written.
- **Solver in C++** [plan], a small binary built with Bloob, called by the service: 33³ inversion, model fitting and smoothing are numeric work that should not run in the TV's JavaScript.
- **Argyll is called, never bundled** [plan]. `spotread` is AGPL; Bloob uses the copy the owner already installed with the probe app (`/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service/spotread`), and says so if it is missing. Nothing of Argyll goes into Git or Bloob's package.
- **LG's signed test manifest stays on the TV** [plan]. Bloob reads the one the probe app installed (`itpg_manifest.json`) at run time instead of committing LG's signature to a public repository. Bloob's own pairing key lives in its data folder, mode 600, never logged.
- **Carrier clips** [plan]: tiny black HEVC clips Bloob generates at build time (HDR10 with BT.2020/PQ flags and mastering metadata; Dolby Vision profile 8.1 with a generated RPU), shipped in the app. They exist only to switch the TV into the right family; iTPG draws the patches over them.

## 4. Measurement science

Every number the solver uses comes through these rules.

1. **One meter session.** Run `spotread` in its interactive mode and feed it a key per reading, instead of a new process per patch. That removes the start-up and meter initialisation on every patch; expected several times faster.
2. **Meter correction first.** A Spyder5 on a WOLED panel needs a spectral correction (CCSS) or a matrix (CCMX); without it the white point is consistently wrong. Argyll can use CCSS on the Spyder5 only with the calibration data from Datacolor's installer, extracted with `oeminst`. First-run setup: Bloob asks for that once; until it exists, Bloob refuses to calibrate and offers a measurement-only report. This is the single biggest accuracy factor, ahead of any algorithm.
3. **Settling and agreement.** After each patch change: wait for the panel (OLED is fast; the meter is not), then read until two consecutive readings agree within a tolerance that scales with luminance (tight above 5 cd/m², loose near black). Throw away outliers; record the spread with every sample.
4. **Near black.** Spyder5 noise dominates below about 0.5 cd/m². Use longer integration there, more repeats, and weight those samples by their measured noise in every fit.
5. **OLED dimming.** Constant small window (10% of the screen) on full black, fixed for the whole run. HDR peak measured at 10%, never full field. Average picture level never changes between patches.
6. **Drift.** A white sentinel every 20 patches. If white drifts, readings between sentinels are normalised to it; beyond 1% the block is re-measured.
7. **Warm-up.** 50% grey until three readings five minutes apart agree within 0.5%, up to 30 minutes.
8. **Placement and stray light.** Measure white and black at the start: white below expectation means the meter is off-centre or tilted (re-place and retry); black above 0.005 cd/m² on an OLED means room light is leaking in (warn, carry on unless severe).
9. **Interruptions.** Screensaver, power events or the TV switching mode are detected (foreground app and picture mode polled between patches); the block in progress is discarded and repeated.

## 5. The run

### 5.1 Preflight (fails fast, writes nothing)

1. Meter visible and correction present. Pattern generator answers. Calibration API answers `start_calibration` / `end_calibration` on a mode Bloob is not going to touch, as a liveness test.
2. TV model and chip match an allow-list (OLED55G2, α9 Gen5, 33-point LUT). Anything else: stop.
3. **Back up everything Bloob will touch**: current picture settings of each target mode (settings service), and the 1D/3D LUTs and matrices through the getters where they work. Stored under `/media/internal/com.muhib.bloob/calibration/backup-<time>/`.
4. Switch off what ruins measurement for the duration: energy saving, AI brightness and picture, dynamic contrast, screensaver, auto power off. Each original value is saved and restored at the end.

### 5.2 Warm-up

As in 4.7.

### 5.3 SDR (`expert2`)

1. `start_calibration expert2`. Contrast 85, brightness 50, colour 50 (LG's reference values), gamma and gamut set so the pipeline is neutral.
2. **Bypass** (`set_bypass_modes_sdr` with a unity 1D LUT): measure the raw panel, not LG's factory correction layered under ours.
3. **Light level**: binary search on `set_oled_light` until white hits the target within 1%.
4. **Characterise** the panel: a dense grey ramp (64 steps, finer near black), per-channel ramps (32 each), and a 3D sampling of the cube (about 600 patches, more near neutral and near the gamut edges). Time is not a constraint; accuracy of the model is.
5. **Model**: per-channel response curves plus a smooth 3D residual fitted in a perceptual space, with each sample weighted by its measured noise.
6. **Synthesise**: for every node of the 33³ grid, the input that the model predicts will produce the target colour (Newton inversion). Colours the panel cannot reach are mapped towards the gamut along constant hue. The LUT is smoothed so no step between neighbouring nodes exceeds what the model can justify (no banding), then quantised to the TV's format. A 1D LUT carries the grey scale; the 3D LUT the colour.
7. **Predict** the result in the model before writing, and refuse to write anything the model predicts to be worse than the bypass baseline.
8. **Write** 1D LUT and the BT.709 3D LUT. Leave the BT.2020 slot of this SDR mode at unity.
9. **Close the loop**: re-measure the grey ramp and a subset of the cube through the new LUTs; fit the residual; write a correction pass. At most three passes, stopping as soon as a pass stops improving.
10. **Verify** on patches not used anywhere above (ColorChecker-style set, saturation sweeps, a second grey ramp offset from the first). Score ΔE2000. `end_calibration`.

### 5.4 HDR10 (`hdr_cinema`)

1. Engine plays the HDR10 carrier (BT.2020, PQ, MaxCLL/mastering metadata set to 1000 cd/m²). Confirm the TV reports HDR10.
2. `start_calibration hdr_cinema` (tone mapping is bypassed while calibrating). Bypass to the raw panel.
3. Measure **peak luminance** on a 10% window, and the panel's native primaries.
4. Characterise as in SDR, in PQ: grey ramp concentrated near black and in the roll-off region, colour samples bounded by the panel's measured gamut (about P3) inside the BT.2020 container.
5. Synthesise and write the 1D LUT and the **BT.2020** 3D LUT: PQ tracking up to the measured peak, colour corrected within reach.
6. **Tone mapping**: `set_tonemap_params hdr_cinema <measured peak> 1000 <r1> 4000 <r2> 10000 <r3>`. The roll-off points follow a BT.2390-style curve for the measured peak; Bloob computes them rather than copying presets.
7. Close the loop and verify as in SDR, scoring ΔE ITP against PQ targets up to 90% of peak, then a check outside calibration mode with tone mapping active (expected curve, not a pass/fail). `end_calibration`.

### 5.5 Dolby Vision (`dolby_cinema_dark`)

1. **Config before data** [bsc]. Bloob writes `DolbyVision_UserDisplayConfiguration.txt` with the panel's measured primaries and peak (from 5.4) to a USB stick the owner leaves in the TV, then plays the Dolby Vision carrier; the TV shows its update pop-up and the owner presses OK. This is the one possible interaction in the run, and only if the stick is present; without a stick the step is skipped and the report says so. Never `set_dolby_vision_config_data`.
2. Engine plays the Dolby Vision carrier (profile 8.1). Confirm the TV reports Dolby Vision.
3. `start_calibration dolby_cinema_dark`. iTPG values in **full range** here.
4. Grey scale: measure, correct through the 1D LUT, close the loop. Colour is left to Dolby's own mapping (the BT.709 3D LUT slot stays at unity unless measurement shows a clear, repeatable error).
5. Verify grey and white. `end_calibration`.

### 5.6 Finish

Restore every setting switched off in preflight. Leave the TV in the calibrated SDR mode. Save results, the LUTs written, the panel model and all raw readings under `calibration/run-<time>/`. Show the report.

## 6. Safety

1. **Allow-list of commands.** Only the documented calibration commands above, only on the three target modes. No service-menu writes, no `set_configs`, no `set_dolby_vision_config_data`, nothing outside calibration mode except reading.
2. **Always close.** Every write path ends with `end_calibration`, including on error, cancel, crash recovery and the service restarting.
3. **Never worse.** A mode whose verification is worse than its baseline is rolled back to its backup (or factory data) automatically.
4. **Undo.** One button: `reset_factory_data_*` for the modes Bloob wrote, plus the saved user settings.
5. **Simulated first.** The whole run executes against a simulated TV and meter (section 7) before it ever runs on the real one; each real-TV stage writes only to one mode until it is proven.

## 7. How it gets proven

1. **A virtual G2.** A panel model (primaries, response curves, near-black noise, ABL, drift) plus a simulated calibration API and iTPG that enforce the real rules (modes per family, value ranges, calibration-mode gating). The full run, interruptions and failures included, runs against it on the desktop in tests.
2. **Replay.** Every real run saves its raw readings; the solver is re-run on them in tests whenever it changes.
3. **Bring-up on the TV, one stage at a time**, each a separate command the owner runs:
   - **C0** read-only: meter session, iTPG patches, calibration getters, settings read. Writes nothing.
   - **C1** patterns in each family: SDR, HDR10 carrier plus iTPG, Dolby Vision carrier plus iTPG. Writes nothing.
   - **C2** a write and a reset in one spare SDR mode (`expert1`): unity 1D LUT, then `reset_factory_data_sdr`. Proves the round trip.
   - **C3** full SDR on `expert2`, report, undo, run again.
   - **C4** HDR10. **C5** Dolby Vision config by USB, then grey scale.
   - **C6** the one-button run end to end.

## 8. The experience

- Settings → **Calibrate**. First time only: the meter correction step, and the TV's one-off "allow" prompt for the pairing.
- Then one screen: hang the meter (a picture shows where), press OK. A quiet progress view: the current mode, the step, a live reading, time remaining, and a small curve filling in as the grey scale is measured.
- No questions mid-run. If the meter slips or the room gets light, Bloob pauses with one sentence and continues when it can.
- At the end: a three-line verdict (SDR, HDR10, Dolby Vision, before → after), the detail behind a button, **Undo**, and the run's date. Later runs show how the panel has aged.

## 9. Open questions to settle on the TV

1. The calibration API's behaviour on webOS 9.2.2 (the owner's firmware, newer than bscpylgtv's documented tests).
2. Whether the getters return usable backups on this firmware.
3. Whether iTPG works over Bloob's own HDR10 and Dolby Vision carriers.
4. Whether `spotread`'s interactive mode works through the Homebrew exec service, or needs a small root helper holding the session.
5. Where the meter correction comes from in practice (Datacolor installer via `oeminst`; a WOLED CCSS), and whether the owner has a reference to build a CCMX.
6. Whether a USB stick can stay in with the Spyder (the G2 has three USB ports).

## 10. Order

Calibration follows **real Dolby Vision for profile 7 remuxes**, which comes first in the build queue. Within calibration: virtual G2 and solver (7.1, 7.2), then C0–C6 in order; HDR10 and Dolby Vision only after SDR passes its targets on the TV.
