# Pal moonshot

Written 30 October 2026, in answer to the owner: "an absolute moonshot to improve Pal; no limitations, unlimited budget, but a solid foundational offering, no gimmicks." Builds on `research/Pal-in-Bloob.md`. Design proposal only.

**North star:** Pal stops being an event that leaves a table behind. It becomes a measured, maintained, end-to-end colour truth for this TV, this room and this viewer, with every claim carrying its uncertainty.

## 1. A physical twin of this one panel, and experiments chosen by information

- **Model.** A physics-grounded model of this G2, fitted from its own measurements, with uncertainty on every parameter:
  - WRGB mixing (how LG adds the white sub-pixel as level rises);
  - near-black behaviour;
  - ABL as a function of window size, APL and time;
  - warm-up and thermal drift;
  - per-channel ageing.
- **Use.** The correction is optimised on the twin, and the TV is used only to test it.
- **Readings.** Each next reading is the one expected to remove the most uncertainty from what matters (Bayesian experimental design). Readings go to where the panel is unknown, not to a fixed 26-level ramp. Expected effect: far fewer readings for a better result, and a run that knows its own confidence.
- **Builds on:** the virtual G2 that already exists, the Jacobian probes, and the simulator fleet. The fleet becomes panels sampled from the real panel's posterior, so every code change is tested against thousands of plausible versions of *this* TV.

## 2. Spectral truth instead of tristimulus guesses

- **Instruments.** A reference spectroradiometer (CS-2000 or CR-300 class) measures each sub-pixel's spectrum on this panel once. The Spyder5 stays as the fast meter, with a correction made on this TV instead of another G2.
- **Meter.** The meter itself is characterised: linearity, near-black noise, temperature drift.
- **Uncertainty.** Every result is propagated to a ΔE with a confidence interval (GUM). Pass or fail is a statistical decision, not a threshold on one noisy number.
- **Observers.** Spectra let any observer be computed later (section 6) without measuring again.

## 3. An imaging colorimeter: the whole screen at once

A calibrated luminance and colour camera (ProMetric class) sees every pixel at once:
- uniformity and banding;
- ABL at every window size;
- confirmation that the pattern drawn is the pattern meant, which removes the "squares never appeared" class of failure;
- the magenta flashes, logged.

It is a measuring instrument, not a webcam guess.

## 4. The whole chain, because Bloob owns the player

No calibrator controls playback; Bloob does, so the calibration can extend past the TV's LUT:
- **The engine checks every film's signalling:** primaries, transfer, matrix, range, and mastering metadata against what the TV reports engaging (`videooutput/getStatus`). It catches mis-flagged files (wrong matrix, wrong range, HDR10 base instead of Dolby Vision) and corrects or reports them.
- **A per-film record:** played in the calibrated profile, the signal it arrived as, and what the TV did with it.
- **Bloob's own interface** is drawn in calibrated colour too.

The result is a guarantee about films as watched, not about test patches.

## 5. The room as part of the target, by the standards' own terms

- **Measure the room:** ambient illuminance and spectrum, and the screen's reflectance at your seat.
- **Use the real black.** BT.1886 is written in terms of the screen's black level. On OLED, a zero black makes BT.1886 collapse into a pure 2.4 power curve, which darkens shadows in a lit room. Pal uses the *effective* black (panel plus reflected room light), which is the honest answer to the open BT.1886-or-2.2 question.
- **Profiles for measured conditions,** such as dark room and lamp on. Bloob switches between them from a measured ambient reading, with each profile verified in the condition it is for. This is not an auto-brightness feature.

## 6. Calibrate for the viewer: observer metamerism

- **The problem.** WOLED's narrow-band primaries make "D65" look different from one person to another: a reading that matches perfectly can still look tinted to a particular viewer. Reference OLED monitors offer a white-point offset for this reason, and the CIE 2006/2015 physiological observer and individual-observer models (Asano and Fairchild) describe it.
- **Pal's offer:**
  - **Reference profile:** the standard observer, which is what content is mastered to.
  - **Personal profile:** a short, properly designed matching task with the remote (forced choice, staircases) estimates your own colour matching. White and grey are then corrected to match D65 as you see it.

Both profiles are reported, and the choice is yours.

## 7. Calibration as a maintained state

- **Monthly health check.** A 60-second read of white, three greys and black, done while you're away, after LG's panel compensation has run.
- **Drift model.** Pal tracks drift against the twin and forecasts when the picture will leave tolerance.
- **Targeted recalibration.** It recalibrates only what moved, warm-started from the twin, in minutes rather than hours.
- **History.** Every past state can be restored, and every change is signed and logged.

## 8. Perception as the last check, done as science

Below about 0.05 cd/m² a Spyder5 can't settle what you will see. So the final check includes short psychophysics tests from the couch, with the remote:
- near-black step visibility at your distance;
- gradient banding;
- the grey that looks neutral.

Each uses forced choice with a proper statistical test, never "does this look good?".

## 9. Foundations underneath all of it

- **A formal TV model.** Every quirk learnt (generator needs video, CAL_START sets Warm 50 and Wide, no 3D readback) is written as a state-machine specification. The stand-in TV is generated from that specification, and every write is proven reversible against it.
- **Pal's decisions are replayable.** Same readings, same journal, same tables. A run can be re-simulated exactly to explain or audit it.
- **The engine stays deterministic and tested.** Pal plans; the twin and solver compute; the gate refuses anything unsafe.

## Not in this moonshot (gimmicks)

- AI "enhancement" or a house look.
- A personality or voice.
- Several agents debating.
- A crowd-sourced panel database that replaces measuring this panel.
- Anything that changes the picture without a measurement behind it.

## If only three

1. **The twin with information-driven measurement (1):** better results from fewer readings, and honest confidence.
2. **The whole chain (4):** only Bloob can do it, and it is the step from "the TV is calibrated" to "the film you watch is right".
3. **Maintenance (7):** an OLED changes, and a calibration that isn't kept is a snapshot.
