# LG calibration best practices for Bloob

A running ledger of practices for calibrating LG OLEDs, as the owner collects them. Each entry gives the practice, its source, how Bloob applies it today, and the change it needs. The applied ones become rules in Pal's pre-flight gate and measuring rules (`research/Pal-in-Bloob.md` §11).

| # | Practice | Source | Bloob today | Change |
| --- | --- | --- | --- | --- |
| B1 | **Pattern delay 1.5–2.0 s.** When the pattern changes, LG's pipeline shows a brief pink or green flicker, or a brightness transition, before the pattern settles. Calibration software's default delay is usually too short | Owner, 30 October 2026 (calibration community guidance) | 0.4 s after a new square (`TvDevice::measure`), after 0.8 s of grey insertion. A reading can therefore start inside the transition | 2.0 s before the first reading of a new patch, configurable. This most likely explains the "magenta flashing between squares" seen on the TV (3.3.6). Cost: about 1.6 s per new patch, so about 35 s for the 21-level grey flow |

## Notes

- **The magenta background in the one-box `patterns` test (30 October) was a still background, not a flicker.** Bloob always draws the black background box, which avoids it. The flashes between squares match B1.
- **Planned for 3.4.0:** B1, and every later entry the owner adds.
