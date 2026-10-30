# PGenerator+ and LG calibration: what Bloob can learn

Studied 30 October 2026: [BigShoots/PGenerator-Plus](https://github.com/BigShoots/PGenerator-Plus) at `835b9b9`, read only, the Raspberry Pi side ignored. PGenerator+ is GPL; Bloob takes facts and methods from it, never code.

Labels: **[static]** read in PGenerator+'s source (file:line at `835b9b9`); **[their capture]** PGenerator+'s comments state it was checked against captured Calman traffic or on a named TV (not verified by us); **[Bloob]** Bloob 3.3.6 today; **[tv]** seen on the owner's G2.

## 1. How PGenerator+ talks to an LG TV

- **One helper owns the TV.** `usr/sbin/pgenerator-lg` (Perl, 7,829 lines) holds every conversation; `usr/share/PGenerator/lg.pm` calls it one request at a time behind a lock, "One TV, one conversation at a time" (lg.pm:16, 1043) [static].
- **Transport: the second-screen socket only.** `wss://<tv>:3001` first, then `ws://<tv>:3000` (pgenerator-lg:728–739); registers with LG's public remote-app manifest (`com.lge.test`, pgenerator-lg:762–800) or an unsigned one on webOS 11+ (749–760) [static]. No luna-send, no root on the TV.
- **Calibration commands:** `ssap://externalpq/setExternalPqData` and `getExternalPqData` (3056, 3061) [static]. Picture settings: `com.webos.settingsservice/setSystemSettings` and `resetSystemSettings` with a `dimension` of `{input, pictureMode, _3dStatus}` where it can, so a write lands on one input's one mode (1700–1707, 1967) [static].
- **Patterns come from its own HDMI source, never from the TV.** There is no `PTN_` command anywhere in the helper or the autocal scripts [static]. The TV is on an HDMI input playing the Pi's patches, so its video path is always live.

## 2. Rules it follows that Bloob does not (yet)

| # | PGenerator+ | Evidence | Bloob 3.3.6 |
| --- | --- | --- | --- |
| L1 | Every `externalpq` command carries `picMode`, `profileNo: 0`, `programID: 1`, not only `CAL_START` | pgenerator-lg:3011–3016, "hw-verified 2026-07-23: CAL_START, CAL_END, and the data commands in between all repeat the same picMode" [their capture] | `picMode` on `CAL_START` only (`tv_device.cpp` `pq`) [Bloob] |
| L2 | `CAL_START`, the data and `CAL_END` on **one socket**. Never `CAL_END` from another connection; an unconfirmed `CAL_END` means restart the TV | 2608–2650 [static] | `CAL_START`/`CAL_END` and small tables by `luna-send` (a new process each), patterns and the 3D LUT by the socket: one session spread over many connections [Bloob] |
| L3 | Right after `CAL_START`, the gamma switches off: `1D_2_2_EN` = 0 and `1D_0_45_EN` = 0, "matching Reference's relay capture" | 3707–3716, 4217, 4351, 4520 [their capture] | Factory reset (`dataOpt` 2) sets them back to factory, which may be on; never set to 0 [Bloob] |
| L4 | SDR resets the **BT.709** 3×3 matrix to identity every cycle: a matrix left in BT.2020 by an HDR run makes every 1D change go through it and the grey solve "spins forever" | 4069–4080 [their capture, C2] | Factory reset only [Bloob] |
| L5 | Panel controls by `UI_DATA` inside `CAL_START`; on the C2 `setSystemSettings(backlight)` is refused outside it ("Permission denied") | 2127–2135, 4523–4534 [their capture, C2] | Light by `BACKLIGHT_UI_DATA` inside the session (already right); Undo puts saved settings back with `setSystemSettings`, outside it [Bloob] |
| L6 | SDR start values Calman writes: brightness 50, contrast 85, light 80, colour 50 (then light is dialled to the target) | 4523–4536 [their capture] | Keeps the owner's values [Bloob] |
| L7 | `dataType` is the string `"unsigned integer16"` or `"float"`; "C2 firmware reports 'not a string'" for the numeric code | 3022–3024 [static] | Strings already [Bloob] |
| L8 | Every 3D LUT upload is read back after 0.35 s and compared; where the getter is unavailable (as on the owner's G2) it records "write accepted, readback unavailable" instead of claiming success | 3780–3815 [static] | Activation check by measurement (stronger), no readback [Bloob] |
| L9 | `CAL_START` acceptance is judged strictly (`type: response` and `returnValue: true`); an explicit rejection means no session was opened, so no `CAL_END` is sent | 2608–2640 [static] | `CAL_END` on leave whenever `in_session_` [Bloob] |
| L10 | Dolby Vision: the wire mode is `dolby_cinema_dark` (not `dolby_hdr_cinema_dark`); `errorCode 20` "Driver error" on DV `CAL_START`/`CAL_END` is expected on some firmware while data commands still work | 2989–2999, 2653–2677 [their capture] | DV not yet run on the TV [Bloob] |
| L11 | Calibration history: each final 1D DPG and DV profile is archived with the model and can be re-uploaded | lg.pm:2787–3140 [static] | `.cube` of the 3D LUT only [Bloob] |

## 3. The meter: the lessons that match the owner's failures

PGenerator+ runs one long-lived `spotread` (`usr/bin/meter_session.sh`) and builds its command in `build_sr_cmd` (782–823) [static]:

- **Display type from the correction file.** `-y c` (refresh) when the CCSS says `DISPLAY_TYPE_REFRESH "YES"`, or the technology is OLED, plasma or CRT; `-y l` otherwise (`webui.pm` 589–607, 491–511) [static]. Bloob passes no `-y` [Bloob].
- **A fixed refresh rate, `-Y R:<Hz>`.** Comment at 816–821: spotread's own "read an 80% white patch to calibrate refresh frequency" step "on a sample-and-hold OLED … is unreliable (it fails without averaging, leaving the meter uncalibrated so every subsequent read fails)" [static]. **This is the owner's 3.3.3 failure exactly:** "Spyder: measuring refresh rate failed", then no reading within 45 s [tv].
- **`-N` after the first start for the Spyder5 (`085c:0500`):** its dark calibration (sensor covered) is done once per boot; later processes reuse it (meter_session.sh 73–82) [static].
- **Low-light averaging per read:** `-Y a`, `-Y aa`, `-Y aaa` for dark patches, by respawning spotread with the flag (782–795, 1198–1225) [static]. On the owner's TV a black reading took ~20 s [tv].
- **Settle before reading:** 400 ms after a patch by default (`meter_lg_autocal.pl` 5096–5113); long settles after a table commit (25 s, 12 s, 6 s; 18007–18034, 18516, 18693) [static]. Bloob: 0.8 s grey, 0.4 s settle, no settle after a table change [Bloob].
- **Bring-up cost:** "the 35–90 s OLED bring-up" is avoided by keeping one session and respawning only spotread when flags change (826–836) [static]. Bloob keeps one session too.

## 4. The pattern question, settled differently

PGenerator+ never asks the TV to draw (section 1). Bloob does, through LG's iTPG, which on the owner's TV draws only while a video plays [tv], hence 3.3.6's black video. The two routes measure the same thing (the picture mode of the live input), but an external source behaves like every film, and the iTPG has firmware-specific faults (bscpylgtv: it "can be broken in many different ways"). Bloob keeps the iTPG because the TV must calibrate itself with no second box; PGenerator+ is evidence that a video under the patterns is the normal state for LG calibration, not a workaround.

## 5. What Bloob should take, in order

Status: 1 (except the refresh rate, which waits for a reading, and dark-patch averaging, which waits for a check of what the probe's spotread supports), 2, 3, 4 and the settle in 5 are in Bloob 3.3.7, proven against the stand-in TV (every write over the socket with `picMode`; Test 8/8; a Quick run and Undo). None of it has run on the TV yet.


1. **Meter (most likely fixes the owner's stall):** `-y` chosen from the CCSS as PGenerator+ does (read the owner's DisplayCAL G2 file on the TV for `DISPLAY_TYPE_REFRESH` first), an explicit `-Y R:<Hz>` (the rate to use is not known yet: one spotread on a white patch, with the rate it reports, settles it), `-N` after the first start of a boot, `-Y a` for patches below about 1 cd/m², and a status line while a slow dark read runs.
2. **One socket for the whole session (L2):** `CAL_START`, every table, every pattern and `CAL_END` over the same second-screen connection; luna-send only for reads and settings.
3. **`picMode` on every command (L1).**
4. **SDR bypass as Calman does it (L3, L4, L6):** after `CAL_START`, `1D_2_2_EN` 0, `1D_0_45_EN` 0, BT.709 3×3 identity, unity 1D and 3D; decide with the owner whether to start from brightness 50 / contrast 85 / colour 50 (contrast and brightness move the result; light is dialled anyway).
5. **Strict `CAL_START` handling (L9),** and a settle after each table commit.
6. **History (L11):** keep each run's 1D table as well as the 3D LUT, so a good result can be put back without measuring again.
