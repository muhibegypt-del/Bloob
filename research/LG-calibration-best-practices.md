# LG calibration best practices for Bloob

A running ledger of practices for calibrating LG OLEDs, as the owner collects them. Each entry gives the practice, its source, how Bloob applies it today, and the change it needs. The applied ones become rules in Pal's pre-flight gate and measuring rules (`research/Pal-in-Bloob.md` §11).

| # | Practice | Source | Bloob today | Change |
| --- | --- | --- | --- | --- |
| B1 | **Pattern delay 1.5–2.0 s.** When the pattern changes, LG's pipeline shows a brief pink or green flicker, or a brightness transition, before the pattern settles. Calibration software's default delay is usually too short | Owner, 30 October 2026 (calibration community guidance) | 0.4 s after a new square (`TvDevice::measure`), after 0.8 s of grey insertion. A reading can therefore start inside the transition | 2.0 s before the first reading of a new patch, configurable. This most likely explains the "magenta flashing between squares" seen on the TV (3.3.6). Cost: about 1.6 s per new patch, so about 35 s for the 21-level grey flow |
| B2 | **Cool down before visual checks.** A long run fires 10% windows at one spot; afterwards, show full-screen varied content for 10–15 minutes before judging the picture by eye | Owner, 30 October 2026 (calibrators' practice; untested here) | Not done. The after report is measured immediately | Adopt for any viewing check: Pal waits 10–15 min of normal content first. The measured after report stays immediate, because the grey flow is short (about 15 min at 140 nits) |
| B3 | **Leave calibration mode before judging shadows or real video.** With it active, the TV uses a different colour state | Owner (calibrators' practice); matches 3.3.7 on this TV | **Already:** calibration mode changes colour temperature and gamut here (Warm 50, Wide). Since 3.3.8 Bloob compares and verifies outside it, with both restored | Keep. Pal's after report is always taken outside calibration mode (§11 gate) |
| B4 | **Warm the meter up.** Power it on and let it sit about 20 minutes (against a dark surface) before measuring; filters drift as they warm, worst on bright HDR patches | Owner (calibrators' practice) | The panel warm-up gate exists in the plan; meter warm-up does not | Adopt: plug the Spyder in when the panel warm-up starts, so both warm together. `pal-start`'s repeatability check (white and 50% must agree over time) then covers both. A dark-surface wait isn't possible while it hangs on the screen; showing black during warm-up is the equivalent |
| B5 | **Small manual CMS changes only** (beyond ±3–4 units, LG's colour sliders cause banding) | Owner (calibrators' practice) | **Not applicable:** Bloob corrects with the 1D/3D tables, not the CMS sliders | The same lesson already holds in the tables: the write gate limits how far any value can move in one pass, and the table must stay smooth and rising |

### About the bscpylgtv description (30 October 2026)

- **What matches what Bloob does and has proven on this TV:**
  - a second-screen (SSAP) session the TV allows once;
  - LG's calibration mode opened and closed through `externalpq` (`CAL_START`/`CAL_END`, called start/end calibration in bscpylgtv);
  - 1D and 3D tables uploaded as raw data arrays and kept in the picture mode.
- **Not supported by any evidence here:** that this "suspends the interface renderer" or "maps memory inside the α9". What the TV demonstrably does in calibration mode: its own notice appears, and colour temperature and gamut change. Tables are sent as arrays of numbers, not as `.cube` or `.cal` files.

## Notes

- **The magenta background in the one-box `patterns` test (30 October) was a still background, not a flicker.** Bloob always draws the black background box, which avoids it. The flashes between squares match B1.
- **In 3.4.0:**
  - B1: a 2 s delay before every reading, all commands; `flashcheck` measures the flicker.
  - B3: Pal measures only outside calibration mode, with the viewing colour temperature and gamut put back.
  - B4: `pal-start` waits until white holds steady over three readings.

  B2 waits for a viewing check; B5 is held by the safety gate.
