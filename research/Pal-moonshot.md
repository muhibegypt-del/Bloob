# Pal at ISF level

Rewritten 30 October 2026. The owner rejected the first moonshot as gimmicks and asked the real question: **how does Pal do everything an ISF calibrator does on this TV?** Design proposal; nothing here is built.

## 1. What an ISF calibrator delivers on an LG G2

1. **Consultation.** The room, when and what you watch, and which sources. On LG this decides two modes: *Expert (Bright room)* for day and *Expert (Dark room)* for night, which are ISF's day/night pair. Then the HDR10 and Dolby Vision modes.
2. **Set-up.** At least 30 minutes of warm-up; the meter placed flat and centred; the source chain checked (signal format, range, black-level matching).
3. **Basic controls.** Brightness (black level), contrast (white clipping), colour, tint and sharpness at their neutral values. All processing off: motion, noise reduction, smooth gradation, dynamic anything, energy saving. Real Cinema on and Just Scan, so there is no overscan.
4. **Readings before.**
5. **White balance and greyscale.** Two-point, then multi-point, to the chosen gamma for each mode. Usually 2.2 or BT.1886 at a brighter level by day; BT.1886/2.4 at about 100 cd/m² by night.
6. **Colour.** CMS, or on LG the AutoCal 1D and 3D LUTs.
7. **HDR10 and Dolby Vision.** PQ tracking, tone mapping and peak; the Dolby Vision configuration file.
8. **Verification.** Readings after: greys, ColorChecker, saturation and luminance sweeps, near-black and clipping. Then a viewing check with the owner on known reference material.
9. **Hand-over.** A report and a settings record, the modes left calibrated, and advice on when to check again.

## 2. Pal today against that list

| Step | Pal (3.3.8) |
| --- | --- |
| 1 Consultation | Gap: one mode (Expert 1), one target fixed in code (140 cd/m², BT.1886) |
| 2 Set-up | Part: the meter is checked; no warm-up, no stability gate. The chain is checked only through LG's own generator |
| 3 Basic controls | Part: processing off (3.3.3), Calman's start values (3.3.7). Black level and clipping are read but not acted on; sharpness, motion, noise reduction, Real Cinema and Just Scan are not touched |
| 4 Before | Have |
| 5 Greyscale | Have: 1D LUT, 26 levels |
| 6 Colour | Have: 3D LUT |
| 7 HDR | Simulator only; carrier clips not made |
| 8 Verification | Part: held-out readings; no sweeps reported, no viewing check |
| 9 Hand-over | Gap: a log, no report |

The maths is at professional level. The missing pieces are the steps around it, and they are what makes a calibrator an ISF calibrator.

## 3. What Pal must gain

- **P-ISF1: Consultation and two modes.**
  - Pal asks about the room, viewing times and sources, measures the light level where you sit if the meter can, and writes the manifest: Day = Expert 1 and Night = Expert 2, each with its target, plus HDR.
  - Test moves off Expert 2, because Expert 2 becomes the Night calibration: either Test runs once before Expert 2 is calibrated, or it uses a mode the owner doesn't use.
- **P-ISF2: Set-up gate.**
  - Warm-up, with a moving mid-grey pattern rather than a static one.
  - A stability test: white measured until its drift over five minutes falls inside the meter's repeatability.
  - Then the meter's own repeatability, which later decides when to stop.
- **P-ISF3: Basic controls, measured.**
  - *Black:* the near-black codes (64, 65, 66, 68, 72…) must rise from the first step above black; neither crushed nor lifted.
  - *White:* 940 must reach peak and stay below clipping in its range (3.3.7 measured clipping above 940).
  - Every neutral setting (sharpness, motion, noise reduction, gradation, Real Cinema, Just Scan, HDMI black level) is set from an allow-list and read back.
- **P-ISF4: Constant-APL patterns.** The patch sits on a grey background, so the screen's average light stays the same from patch to patch and OLED dimming doesn't enter the readings. Calman uses patterns like this on OLED; LG's generator already supports the background box.
- **P-ISF5: Verification through the real source.** An ISF calibrator checks with a test disc in the real player. Bloob does the same:
  - pattern clips (PLUGE, clipping, grey steps, colour bars, near-black, saturation sweeps) play through Bloob's own players, the engine and LG's player;
  - the meter reads them;
  - the result is compared with LG's generator.

  This proves films get the calibration, and catches a range or matrix fault in Bloob's own path.
- **P-ISF6: HDR10 and Dolby Vision.** The plan as written: carrier clips, PQ tracking to the measured peak, tone mapping, and the Dolby Vision configuration on USB.
- **P-ISF7: The viewing check.** Bloob plays short reference clips (skin, a dark scene, a bright scene, a grey ramp). Pal asks plain questions with the remote (crushed? tinted? banding?) and acts on the answers or explains them. The meter can't see these; the calibrator's eyes are part of the job.
- **P-ISF8: Hand-over report.**
  - Before and after for every mode: greys, gamma, ColorChecker, sweeps, near-black, clipping.
  - Every setting and table changed.
  - The meter and its correction file.
  - What the panel can't reach.
  - When to check again: ISF practice is periodic, and OLED compensation cycles move the panel.
- **P-ISF9: The playbook.** What an ISF calibrator knows is written as Pal's procedure for each step: the LG G2's specifics, what "good" means at each step, tolerances, and what to do when a step fails. Pal follows it and reasons with it; the solver and the gate stay as they are (`research/Pal-in-Bloob.md` §4).

## 4. The one thing Pal can't equal

An ISF calibrator brings a reference meter: a colorimeter profiled with a spectroradiometer on this panel. Pal has a Spyder5 with a correction measured on another G2. Pal can match the **process** and the **precision** (repeatable, verified, documented). The **absolute accuracy** is the meter's, and the report says so. The only route to full parity is profiling the Spyder5 once against a reference instrument on this TV.

## 5. Order

| Step | Gives |
| --- | --- |
| 1 | P-ISF2, P-ISF3, P-ISF4 and P-ISF8 inside today's Quick: a run an ISF calibrator would recognise, with a report |
| 2 | P-ISF1: Day and Night |
| 3 | P-ISF5: verification through Bloob's players |
| 4 | Pal (`research/Pal-in-Bloob.md` P1–P4) running these steps by the playbook (P-ISF9), with the viewing check (P-ISF7) |
| 5 | P-ISF6: HDR10, then Dolby Vision |
