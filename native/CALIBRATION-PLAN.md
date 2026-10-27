# Bloob Calibration: technical plan (27 October 2026, revision 2)

One run, no questions: SDR, then HDR10, then Dolby Vision, on the owner's LG OLED55G2 (α9 Gen 5, webOS 9.2.2) with a Datacolor Spyder5. Planned as if engineering time were free; the order is set by risk and evidence. **Nothing here is built.** Revision 2 replaces the first draft, which named commands but not their formats, assumed things bscpylgtv's own guides contradict, and could not have been followed without guessing. Section 13 lists what changed.

Evidence labels:
- **[probe]**: the owner's Spyder USB Probe 0.6.1 (`org.codex.spyderprobe`), unpacked and read on 27 October 2026.
- **[bsc]**: bscpylgtv source and guides (MIT, github.com/chros73/bscpylgtv), read on 27 October 2026. Its calibration code was tested by its authors on 2018 α7/α9 and 2019/2021/2022 α9; its preset scripts name "a G2 (2022) running webOS v9 (2024)".
- **[owner]**: the owner's report.
- **[plan]**: a design decision.
- **[unknown]**: settled by the named experiment (section 11) before anything depends on it.

## 1. Goal and pass marks

| Mode | Picture mode | Target | Pass, on the verification set (never on calibration patches) |
| --- | --- | --- | --- |
| SDR | Expert (Dark) `expert2` | Rec.709 primaries, D65, BT.1886 (or 2.4 power), white 100 cd/m² unless the owner chooses otherwise | Grey ΔE2000 average ≤ 0.5, maximum ≤ 1.0; colour average ≤ 1.0, maximum ≤ 2.0 |
| HDR10 | Cinema `hdr_cinema` | PQ (ST 2084) to the measured peak, D65 grey; LG tone mapping set from the measured peak | Grey ΔE ITP (BT.2124) average ≤ 2 up to 90% of peak; monotonic highlights up to 1000 cd/m² mastering |
| Dolby Vision | Cinema (Dark) `dolby_cinema_dark` | Dolby's mapping fed the panel's real peak and primaries; grey corrected to D65 | Grey chromaticity within 0.003 Δu′v′ of D65 from 5% to 100%; black no higher than before |

ΔE2000 is computed relative to the target white; ΔE ITP uses BT.2124 (ΔE = 720·√(ΔI² + ΔT² + ΔP²), T = 0.5·Ct). The owner's previous tool reached about 0.8 ΔE on white balance [owner]; that is the figure to beat.

## 2. The TV interface, exactly

### 2.1 Transport

- **Calibration and pattern commands** go to `ssap://externalpq/setExternalPqData`; reads go to `ssap://externalpq/getExternalPqData`. Both are sent over the TV's second-screen socket `ws://127.0.0.1:3000`, after registering with LG's signed test manifest [bsc, probe].
  - The probe registers with `{type:"register", payload:{"client-key", forcePairing:false, manifest, pairingType:"PROMPT"}}`. It uses the gateway's own `ws` module (`/usr/palm/services/com.webos.service.secondscreen.gateway/node_modules/ws`), read-only, and stores its key with mode 600 [probe].
  - Bloob does the same with its own key file, `<data>/calibration/ssap-key.json` (mode 600, never logged, never in Git). It reads the manifest from the probe app's `itpg_manifest.json` at run time, so LG's signature is never committed.
  - **[unknown U1]** Bloob is root through the Homebrew Channel. If the bus service behind `externalpq` answers `luna-send` directly, no pairing is needed. Experiment C0.3 tries it first.
- **Picture settings** use `luna://com.webos.settingsservice/getSystemSettings` and `setSystemSettings` with `category:"picture"`. Changing the picture mode uses `{"pictureMode": …}`, which bscpylgtv's `set_system_picture_mode` states needs webOS 9 (the owner has 9.2.2). Bloob's service already holds a `webos-service` handle (`server.js`), and root commands go through `luna://org.webosbrew.hbchannel.service/exec`, as the probe and `main.cpp` already do.

### 2.2 Calibration messages [bsc `webos_client.py`, `cal_commands.py`]

- **Every write** has this form:

  ```
  {"command": C, "profileNo": 0, "programID": 1,
   "data": base64(little-endian bytes), "dataCount": N,
   "dataType": "unsigned integer16" | "float" | "unsigned char",
   "dataOpt": 1}
  ```

  `dataOpt` is 0 to apply, 1 to apply and save, 2 to reset to factory. A reset sends empty data with `dataOpt: 2`.
- **Session commands:**
  - Start: `{"command":"CAL_START","profileNo":0,"programID":1,"picMode":"expert2"}`.
  - End: `{"command":"CAL_END","profileNo":0,"programID":1}`.
- **Write commands:**

  | Command | Data | Meaning |
  | --- | --- | --- |
  | `BACKLIGHT_UI_DATA`, `CONTRAST_UI_DATA`, `BRIGHTNESS_UI_DATA`, `COLOR_UI_DATA` | uint16 ×1, 0–100 | OLED light, contrast, brightness, colour |
  | `1D_DPG_DATA` | uint16 ×3072, **planar**: 1024 red, then 1024 green, then 1024 blue; values 0–32767 (unity is a straight line from 0 to 32767) | 1D LUT |
  | `BT709_3D_LUT_DATA`, `BT2020_3D_LUT_DATA` | uint16 ×(33³×3); nodes ordered blue slowest, red fastest; each node R, G, B; values 0–4095 (unity node = index × 128, clipped to 4095) | 3D LUT. The slot is chosen by the signal's colour flags; Dolby Vision uses BT.709 [bsc README] |
  | `BT709_3BY3_GAMUT_DATA`, `BT2020_3BY3_GAMUT_DATA` | float ×9, −1024 to 1024 | 3×3 matrices |
  | `1D_2_2_EN`, `1D_0_45_EN`, `3BY3_GAMUT_EN`, `1D_DPG_EN` | uint16 ×1 (0 or 1) | Enable flags |
  | `1D_TONEMAP_PARAM` | uint16 ×7: peak (100–4000), then mastering peak and roll-off (0–100) three times: 1000, 4000, 10000 | HDR10 tone mapping, for the mode in session; the payload carries no mode name |

- **Getters** (`getExternalPqData`, `{"command": …}`) return `data`, `dataCount` and `dataType`:
  - `GET_1D_DPG_DATA` (3×1024), `GET_3D_LUT_DATA`, `GET_3BY3_GAMUT_DATA`, `GET_1D_2_2_EN`, `GET_1D_0_45_EN`.
  - They are "partially or completely broken depending on firmware" [bsc README]. **[unknown U2]**
- **Bypass**, as bscpylgtv defines it:
  - SDR: both enable flags off, both matrices identity, both 3D LUTs unity, and optionally a unity 1D LUT.
  - HDR10: a unity 1D LUT (the rest optional).
  - Dolby Vision: a unity 1D LUT.
- **Factory reset** is every SDR/HDR item reset with `dataOpt: 2`. For Dolby Vision, it is the 1D LUT only.
- **Side effects of `CAL_START`** [bsc guides]:
  - It sets colour temperature to **Warm 50**.
  - It sets colour gamut to **Wide**.
  - The HDR10 guide sets gamut back to `auto` afterwards; the Dolby Vision guide sets it to `native`. Otherwise colours are oversaturated.
  - Bloob records both values before `CAL_START` and sets them deliberately after `CAL_END`.
- **Other constraints:**
  - Calibration commands work only between `CAL_START` and `CAL_END`.
  - A mode family is only reachable while the TV receives that kind of signal.
  - HDR10 tone mapping is bypassed in calibration mode.
  - Uploading a 1D LUT greys out the white-balance and gamma controls.
  - Once a 1D LUT is uploaded to a Dolby Vision mode, the USB configuration pop-up is disabled for that mode [bsc README and guides].

### 2.3 Internal pattern generator (iTPG) [bsc, probe]

- **Window command:** `{"command":"PTN_SINGLE_WINBOX_ATTR","fillR","fillG","fillB","winId":0–9,"width","height","startX","startY","programID":1}`. Coordinates are panel pixels on a 3840×2160 grid, whatever the signal.
- **Control command:** `{"command":"PTN_CTRL","enable":"true"|"false","numOfBox":n,"ptnType":0,"programID":1}`.
  - `enable` is a string on current firmware.
  - Turning it off while already off returns error 20; the probe treats that as success.
- **Code ranges, 10-bit:**
  - HDR10 on models from 2020 uses limited range (64–940).
  - Dolby Vision uses full range (0–1023).
  - SDR is not documented. The probe uses limited range with a 64 background. **[unknown U3]**, settled by C1.2.
- **Patch layout:** window 0 is a full-screen black background; window 1 is the patch. Both are re-sent on every patch change, then `PTN_CTRL` true with 2 boxes. bscpylgtv's measuring script sizes patches by percentage of screen area (default 5%).

### 2.4 Meter [probe]

- `spotread` is Argyll CMS 2.0.1 for ARM, with its X11 libraries in `libs/`. It runs as `LD_LIBRARY_PATH=<base>/libs /lib/ld-linux.so.3 <base>/spotread -e -O -x` through the Homebrew Channel's `exec`, as root.
- The result line is `Result is XYZ: X Y Z, Yxy: Y x y`.
- The probe detects the meter by USB ID `085c:0500` in `/sys/bus/usb/devices`.
- `exec` returns only when the command finishes, so it cannot hold an interactive `spotread` session open. That is why section 4.2 has its own helper.

## 3. Architecture and files

| Part | Files | Job |
| --- | --- | --- |
| Colour science | `native/cal/colour.{h,cpp}` | XYZ, xy, u′v′, CIELAB, ΔE2000, PQ, BT.1886, ICtCp, ΔE ITP, Rec.709/BT.2020 matrices, Bradford |
| LUT formats | `native/cal/lut.{h,cpp}` | Encode and decode LG's 1D and 3D LUTs (section 2.2), `.cube`, Argyll `.cal`; unity tables |
| Panel model | `native/cal/model.{h,cpp}` | Forward model from device RGB to XYZ, fitted to readings (section 6.2) |
| Solver | `native/cal/solve.{h,cpp}` | Grey-scale and colour inversion, gamut mapping, smoothing, prediction |
| Solver tool | `native/tools/bloob_cal.cpp` → `bloob-cal` | JSON in, LUT files and predictions out; run by the service |
| Meter helper | `native/tools/bloob_meter.cpp` → `bloob-meter` | Root helper holding one `spotread` session (section 4.2) |
| TV client | `tv-app/service/cal-tv.js` | Transport (luna, or ssap), the command allow-list, encoding, getters, iTPG |
| Meter client | `tv-app/service/cal-meter.js` | Starts and talks to `bloob-meter`; the reading rules of section 5 |
| Orchestrator | `tv-app/service/calibrate.js` | The state machine (section 7), checkpoints, timing; routes `/api/calibrate/{state,start,cancel,undo,history,report}` |
| Store | `tv-app/service/cal-store.js` | `<data>/calibration/`: backups, runs, raw readings (JSON lines), the meter correction, the key |
| Screen | `native/src/calibrate.cpp` | Settings → Calibrate: first-time setup, progress, report, Undo |
| Carrier clips | `native/package/cal/{hdr10,dv81}.mkv`, made by `native/scripts/make-carriers.sh` | Switch the TV into HDR10 or Dolby Vision (section 8.2) |
| Simulator | `tv-app/tests/service/cal-sim.mjs`, `native/tests/cal_sim.{h,cpp}` | The virtual G2 (section 10) |

**Rules for these parts [plan]:**
- **Service files are flat**, because `stage-service.mjs` copies only top-level `*.js` files. They contain no `?.` or `??`, for the TV's Node.
- **The two binaries are built for ARM** with the app and installed beside `bloob`.
- **Argyll is called, never bundled.** It is AGPL: Bloob uses the probe app's installed `spotread` and says so plainly if it is missing.
- **The service owns the run.** The app's screen only shows its state. While a run is active, the service refuses the app's usual `stop` on exit. It first closes calibration and restores settings (section 9.2), then stops.

## 4. Measurement engine

### 4.1 Correction for the meter

- A Spyder5 reads a WOLED panel's white noticeably wrong without a spectral correction. This is the largest single error source, so it comes before any algorithm.
- Argyll uses a CCSS correction with a Spyder5 only after `oeminst` has installed the Spyder's calibration data from Datacolor's installer.
- **First-run step:** Bloob checks for Argyll's Spyder data and for a WOLED `.ccss`, and passes it to `spotread` as `-X <file>`.
- Without the correction, Calibrate offers a measure-only report and does not write anything. **[unknown U4]**: where the owner's copy is, settled by C0.2.

### 4.2 `bloob-meter` (root helper)

- **Started** once per run through `exec` with `nohup … &`. It owns `spotread` in a pseudo-terminal, because `spotread` reads its keys from the console.
- **Protocol:** it serves one JSON-lines Unix socket, `/tmp/bloob-meter.sock`, mode 600:

  | Request | Reply |
  | --- | --- |
  | `{"op":"read"}` | `{"XYZ":[…],"ms":…}` |
  | `{"op":"status"}` | `{"ok":true,"instrument":"Spyder5"}` |
  | `{"op":"quit"}` | the helper exits |

- **Lifetime:** it exits by itself after 10 minutes without a request.
- **Command line:** `spotread -e -x [-X ccss]`, without `-O`, so the instrument is initialised once. Each `read` sends a key and parses `Result is XYZ:`.
- **[unknown U5]** whether `spotread` behaves in a pseudo-terminal on this build (C0.4).
- **Fallback:** one `spotread -O` per reading, as the probe does. It is correct, but every reading pays the meter's start-up time.

### 4.3 Reading rules (`cal-meter.js`)

- **Settle:** after a patch change, wait 400 ms (OLED switches fast; this covers the iTPG's own latency), then take readings.
- **Agreement:** read until two consecutive readings agree within a tolerance that scales with luminance:
  - |ΔY|/Y ≤ 0.5% and Δxy ≤ 0.0015 above 5 cd/m²;
  - |ΔY| ≤ 0.02 cd/m² below it.
  - At most 5 readings. Keep the median and the spread.
- **Near black (Y < 0.5 cd/m²):** at least 4 readings, the median is kept, and the spread becomes the sample's weight in every fit (weight ∝ 1/variance).
- **Sentinel:** the patch-size white is measured every 20 patches. Drift beyond 0.5% rescales the readings between two sentinels linearly. Drift beyond 1.5% means that block is measured again.
  - LG's static-image dimming is the expected cause. Bloob keeps the picture changing, and the user setting *Adjust logo brightness* is switched off for the run. Bloob never touches the service menu.
- **Window:** 10% of the screen area, centred, on a black background, fixed for the whole run.
  - HDR peak is read on that same 10% window, because full-field white is limited by the panel's power management.
  - The black background code is 64 (limited range) or 0 (full range), matching the mode.
- **Placement check at the start:**
  - White below 70% of the expected value means the meter is off-centre or tilted. Bloob pauses and says so in one sentence.
  - Black above 0.005 cd/m² on this OLED means stray light. Bloob warns, and reruns that block if the light changes.
- **Warm-up:** 50% grey until three readings five minutes apart agree within 0.5%, up to 30 minutes.
- **Interruptions:** between patches Bloob polls the foreground app, the picture mode and the HDR state of the video output (`com.webos.service.videooutput/getStatus`, the call the owner already used). A change means the patch block is discarded and repeated. Five failures in a row pause the run with a plain message.

## 5. Patch sets

Codes are 10-bit; *L(p)* is the limited-range code for fraction *p*: 64 + round(876·p).

| Set | Used for | Content |
| --- | --- | --- |
| G | Grey characterisation | 33 steps of signal, spaced so half lie below 25% |
| G±| Local channel sensitivity | At 16 grey levels: grey, then grey with +δ on R, G and B in turn, δ = 8 codes |
| C | Colour characterisation | A 9×9×9 grid (729), plus 12 steps along each primary and secondary to full saturation (72), plus 100 near-neutral points |
| V | Verification only | 21-step grey offset by half a step from G; the 24 ColorChecker colours in the target space; saturation sweeps 25/50/75/100% for six colours; 10 random colours inside the target gamut. Never used in any fit |
| H | HDR grey | PQ codes for 0.005, 0.01, 0.05, 0.1, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 300, 400, 500, 600, 700, 800 and 1000 cd/m², plus the measured peak |
| P | Primaries | 100% R, G, B, C, M, Y and white, in the mode's range, for the panel's native gamut |

## 6. Solver

### 6.1 Targets

- **SDR:**
  - The signal is RGB′ in [0,1] from the limited code.
  - Its luminance follows BT.1886 with the measured black Lb and target white Lw (or a pure 2.4 power curve).
  - Chromaticity comes from Rec.709 primaries and D65 (x 0.3127, y 0.3290).
  - Target XYZ = M709 · EOTF(RGB′) · Lw.
- **HDR10:** grey target Y = PQ⁻¹(code) up to 90% of the measured peak, with D65 chromaticity. Colour is not targeted; see 7.4.
- **Dolby Vision:** the grey target keeps the measured luminance at each step and moves chromaticity to D65 (white balance only, as bscpylgtv's Dolby Vision guide does), with black no higher than the uncorrected black.

### 6.2 Panel model

- **Why not a matrix model:** a WOLED panel is not additive, because it has a white sub-pixel. A 3×3 matrix model with three tone curves therefore has errors of several ΔE.
- **Layer 1 (baseline):** per-channel tone curves (monotone PCHIP through the channel ramps, normalised) and a 3×3 matrix fitted by weighted least squares.
- **Layer 2 (residual):** a smooth 3D correction of the baseline error, fitted to every reading.
  - Thin-plate radial basis functions in device RGB, with a smoothing term chosen by leave-one-out cross-validation.
  - The residual is fitted in XYZ divided by white.
- **Uncertainty:** the leave-one-out error at each reading gives a confidence map. Nodes in low-confidence regions get extra patches in the next pass (section 7.2, step 8).

### 6.3 Grey (1D LUT)

1. For each grey level *g* in G, the local 3×3 sensitivity *J(g)* = ∂XYZ/∂RGB comes from the G± readings.
2. The correction Δ*(g)* solves *J(g)·Δ = target(g) − measured(g)* by damped least squares in CIELAB relative to the target white. ITP is used for HDR.
3. Corrections between grey levels are interpolated with monotone PCHIP per channel, then applied to the 1024-entry curves.
4. Each channel's curve must stay monotonic and within 0–32767.
5. At most 4 passes, re-measuring G each time. Stop when the grey average stops improving by more than 0.05 ΔE.

### 6.4 Colour (3D LUT, SDR)

1. **Out-of-gamut targets:** a node's target XYZ that the panel cannot reach is replaced by the reachable point on the line toward the neutral of the same lightness, keeping hue and lightness in CIELAB. It is found by bisection against the model's gamut.
2. **Solve each node:** find the device RGB that minimises ΔE2000 between the model's prediction and the target. Use damped Gauss–Newton with an analytic Jacobian of the baseline plus a numeric one of the residual, starting from the baseline's inverse. Clamp to [0,1].
3. **Smooth:** a Laplacian over the 33³ grid, weighted by the model's confidence (strong where data are thin, none where they are dense). Then check every node's predicted ΔE again; no node may get worse by more than 0.3.
4. **Keep the grey axis identity** if the 1D LUT handles grey (section 7.2), so the two tables never fight.
5. **Quantise:** 12-bit values, written as section 2.2 specifies.

### 6.5 Prediction gate

- Before any write, the solver predicts the verification set (V) through the model and the new tables.
- If the prediction is worse than the bypass baseline, nothing is written and the run reports why.

## 7. The run

### 7.1 Preflight (writes nothing)

1. Meter visible (USB `085c:0500`), the correction present, `bloob-meter` answering.
2. TV transport answering. Model allow-list: the model name read from the TV must be OLED55G2 with a 33-point chip; anything else stops the run.
3. **Backups**, saved to `<data>/calibration/backup-<time>/`:
   - every picture setting of the target modes, from the settings service;
   - the getters' 1D LUT, 3D LUT and matrices, wherever the getters work. Their checksums are compared with a second read; disagreement means that backup is marked unreliable, and Undo uses the factory reset for that mode.
4. **Measurement settings:** energy saving, AI picture and brightness, dynamic contrast, peak brightness, logo brightness, the screensaver and auto power-off are recorded and switched off for the run. All are restored at the end, and on cancel or failure.

### 7.2 SDR (`expert2`)

1. Bloob is in the foreground, showing a black page. Set `pictureMode: expert2` and confirm it was applied.
2. `CAL_START` `expert2`, then contrast 85, brightness 50 and colour 50 (bscpylgtv's defaults).
3. **Bypass:** both enable flags off, matrices identity, both 3D LUTs unity, 1D LUT unity.
4. **Pipeline order test (once, C2.3):** does the 1D LUT act before or after the 3D LUT? This decides the method below.
   - **If 1D acts after 3D** (expected: "DPG" sits on the panel side): the 1D LUT carries grey and white balance, and the 3D LUT carries colour with its grey axis identity.
   - **If not:** the 3D LUT carries everything and the 1D LUT stays unity. This is the method bscpylgtv's SDR guide uses.
5. **Light:** binary search on `BACKLIGHT_UI_DATA` (0–100) until 100% white is within 1% of the target.
6. **Warm-up** (4.3).
7. **Grey:** sets G and G±, then section 6.3 passes.
8. **Colour:**
   - Measure set C through the grey solution.
   - Fit the model and add up to 200 patches where it is least certain.
   - Solve (6.4).
   - Gate (6.5).
   - Upload `BT709_3D_LUT_DATA` and leave the BT.2020 slot unity.
9. **Closed loop:** measure V and a 100-patch random subset of C. Refit the residual from these readings only and solve again. At most 3 passes; stop when V stops improving.
10. `CAL_END`. Set colour gamut to the value the C2.4 experiment proves keeps the uploaded LUT in charge.
11. **Final verification** in the viewing state, outside calibration mode: set V again. If it fails its pass marks or is worse than the baseline, the mode is restored (9.1).

### 7.3 Native gamut (for Dolby Vision, measured once)

- Within the SDR bypass at step 3, set P at 75% stimulus gives the panel's native primaries (xy).
- Dolby Vision's configuration needs these.

### 7.4 HDR10 (`hdr_cinema`)

1. Engine plays `hdr10.mkv` on loop. Confirm `getStatus` reports HDR10.
2. `pictureMode: hdr_cinema`, `CAL_START hdr_cinema`, then a unity 1D LUT (bscpylgtv's HDR10 bypass).
3. **Peak:** 10% window at code 940; then step down until readings rise again with the code, which finds the panel's clip point. Peak = the highest Y before the clip.
4. **Grey:** set H through section 6.3, in ITP, up to 90% of peak. 1D LUT only: bscpylgtv's HDR10 method corrects white balance and leaves the 3D LUT alone [bsc guide].
5. **Tone mapping:** `1D_TONEMAP_PARAM` = [measured peak, 1000, 75, 4000, 60, 10000, 50]. The roll-offs are the guide's worked example; LG does not document what they mean.
6. `CAL_END`. Colour gamut back to `auto`, as the guide does.
7. **Verification outside calibration mode** (tone mapping now active):
   - grey ΔE ITP up to 90% of peak;
   - codes for 600, 700, 800, 900 and 1000 cd/m² must measure strictly increasing (no clipped highlights in 1000 cd/m² masters).
   - Failure restores the mode.
8. **Optional experiment X1:** a BT.2020 3D LUT for HDR colour. Adopted only if it improves the colour verification average by at least 1 ΔE ITP with no colour worse by more than 0.5. Off by default until proven on this TV.

### 7.5 Dolby Vision (`dolby_cinema_dark`, picture-mode number 2)

1. **Configuration first, because a later 1D LUT disables it:**
   - If a USB stick is present (the owner's choice in first-time setup), write `DolbyVision_UserDisplayConfiguration.txt` to its root in the 2019 format:

     ```
     # Dolby Vision User Display Configuration File
     [PictureMode = 2]
     Tmax = <measured peak, 4 decimals>
     Tmin = <measured black, at least 0.0001>
     Tgamma = 2.2
     TPrimaries = xr yr xg yg xb yb 0.3127 0.3290
     ```

     Lines end with CRLF. Primaries come from 7.3, peak and black from 7.4.
   - Play `dv81.mkv`; the TV offers its update pop-up and the owner presses OK. This is the run's only interaction, and only with a stick present. Without one, the step is skipped and the report says so.
   - Bloob never sends `DOLBY_CFG_DATA`, which bscpylgtv advises against on 2020 and later models.
2. Engine plays `dv81.mkv` on loop. Confirm `getStatus` reports Dolby Vision. `pictureMode: dolby_cinema_dark`, `CAL_START`, then a unity 1D LUT.
3. iTPG in full range (0–1023). Grey: set G in full-range codes, white balance only (6.1), through section 6.3.
4. **Black check:** the 5% and lower steps must not rise above their uncorrected values. If they do, the first entries of each 1D curve are pulled down until they stop rising (bscpylgtv's raised-black fix).
5. `CAL_END`. Colour gamut `native`, as the guide does. Verify grey outside calibration mode.

### 7.6 Finish

1. Restore the settings recorded in 7.1.4.
2. Return to the calibrated SDR mode and stop `bloob-meter`.
3. Write the report.

## 8. Signals and patterns

### 8.1 Patterns

All patches come from the iTPG (2.3) drawn over Bloob's own black page, so the browser and the app's renderer never take part in measurement.

### 8.2 Carrier clips (`make-carriers.sh`, run on the development machine)

- **`hdr10.mkv`**
  - 3840×2160 HEVC Main10, 23.976 fps, 60 s of black. Encoded with x265 on the development machine; only the output file ships.
  - VUI: BT.2020 primaries, PQ transfer, BT.2020 non-constant matrix.
  - Mastering display SEI (P3-D65, 1000/0.0001 cd/m²); content light level SEI (1000/400).
- **`dv81.mkv`**
  - The same video, with a profile 8.1 RPU made by `dovi_tool generate` (MIT) from a static configuration and injected with `dovi_tool inject-rpu`.
  - This also exercises the profile 8.1 path the real-Dolby-Vision work needs.
- **Size:** black frames compress to a few hundred kilobytes, so both ship in the IPK.
- **Looping:** the engine gets a `loop` flag, for calibration only.
- **[unknown U6]** Does the iTPG draw over the engine's HDR10 and Dolby Vision playback? Settled by C1.3 and C1.4.
- **Fallbacks:**
  1. The same file through LG's player (`UmsPlayer`).
  2. If that fails, the owner plays any HDR10 or Dolby Vision title and the run continues once `getStatus` confirms it.

## 9. Safety

### 9.1 Rules

1. **Command allow-list in `cal-tv.js`:**
   - Allowed: exactly the commands in 2.2 and 2.3, and only with `picMode` in {expert1, expert2, hdr_cinema, dolby_cinema_dark}. `expert1` is used only for the C2 round-trip test.
   - Everything else throws before it reaches the socket. That includes `DOLBY_CFG_DATA`, the service menu, `setConfigs` and any other luna write.
2. **Data checks** before each send: shape, type, range, monotonic 1D curves, and no NaN.
3. **Always close.** Every path that opens a session ends with `CAL_END`: success, error, cancel, timeout, and service restart (9.2).
4. **Never worse.** A mode whose final verification is worse than its baseline is restored automatically: from its backup if the getters were reliable, otherwise by factory reset of that mode.
5. **Undo** applies the factory reset of each mode Bloob wrote, then puts back the saved settings.

### 9.2 Interruption and resume

- **Checkpoints:** the state machine writes `run.json` after every step (phase, mode, step, pass, results), and every reading goes to `readings.jsonl`.
- **Service start:**
  1. If `run.json` says a session may be open, the first action is `CAL_END` (harmless when none is open).
  2. Then the recorded settings are restored.
  3. The run is offered as *Resume*: completed modes stay, and the mode in progress restarts from its last completed step.

### 9.3 Proven before it touches the TV

The full run, failures included, runs against the simulator (section 10).

## 10. Tests

| Test | Where | Asserts |
| --- | --- | --- |
| Colour science | `native/tests/cal_test.cpp` | ΔE2000 against Sharma's 34 published pairs; PQ, BT.1886 and ICtCp round trips; ΔE ITP against BT.2124 examples |
| LUT formats | `cal_test.cpp`, `tv-app/tests/service/cal-tv.test.mjs` | Unity 1D and 3D tables byte-identical to bscpylgtv's (`unity_lut_1d`, `unity_lut_3d`); `.cube`/`.cal` round trips; base64 payloads match fixtures made with bscpylgtv |
| Allow-list | `cal-tv.test.mjs` | Every forbidden command and mode is refused before sending |
| Model | `cal_test.cpp` | On a synthetic WOLED (white sub-pixel, non-additive), the fitted model predicts held-out patches within 0.5 ΔE2000 |
| Solver | `cal_test.cpp` | On synthetic panels with known errors (wrong white, gamma 2.2 instead of 2.4, oversaturated green), the solved tables bring set V within the pass marks of section 1; the solver never writes NaN, non-monotonic curves or out-of-range values |
| Meter rules | `tv-app/tests/service/cal-meter.test.mjs` | Agreement, near-black weighting, sentinel rescaling and re-measure, placement and stray-light warnings, all against a scripted fake meter |
| Whole run | `tv-app/tests/service/calibrate.test.mjs` with `cal-sim.mjs` | The virtual G2 enforces the rules of 2.2 (sessions, mode families, ranges, `CAL_START` side effects, the disabled Dolby Vision pop-up after a 1D upload). Runs: clean; meter slips; foreground app changes; service restart mid-run (asserts `CAL_END` comes first, then resume); verification worse than baseline (asserts restore); getters broken (asserts factory reset is used) |
| Replay | `calibrate.test.mjs` | Real runs' `readings.jsonl` (no personal data in them) re-solved whenever the solver changes |

## 11. Bring-up on the TV

Each step is one command the owner runs, with a pass criterion. Each step must pass before the next one starts.

| Step | Writes | What happens | Passes when |
| --- | --- | --- | --- |
| C0.1 | Nothing | Read the model name, firmware and the target modes' picture settings | Model is OLED55G2 |
| C0.2 | Nothing | Find Argyll's Spyder data and a WOLED `.ccss` | Both found, or the owner is told exactly what to add **(U4)** |
| C0.3 | Nothing | Try `getExternalPqData GET_1D_DPG_DATA` by luna as root; else pair over ssap (the TV asks once) | A reply with `dataCount` 3072 **(U1, U2)** |
| C0.4 | Nothing | Start `bloob-meter`, 10 readings of the current screen | Readings agree within 1% **(U5)** |
| C1.1 | Nothing | iTPG patches in SDR | Each colour appears where expected |
| C1.2 | Nothing | Measure SDR codes 940 and 1000, then 64 and 0 | Equal pairs mean limited range **(U3)** |
| C1.3 | Nothing | Play `hdr10.mkv`, check `getStatus`, draw one patch | HDR10 reported and patch visible **(U6)** |
| C1.4 | Nothing | Same with `dv81.mkv` | Dolby Vision reported and patch visible **(U6)** |
| C2.1 | `expert1` only | `CAL_START expert1`, then a unity 1D LUT, then `CAL_END` | Measured greys unchanged within noise |
| C2.2 | `expert1` only | A 1D LUT that halves red, then a factory reset of `expert1` | Red drops, then returns within noise; the getter matches what was written |
| C2.3 | `expert1` only | Pipeline order: a 3D LUT that swaps red and green, plus a 1D LUT that zeroes red; measure input red | The colour seen shows which stage acts last |
| C2.4 | `expert1` only | After `CAL_END`, try each colour gamut setting with a non-unity 3D LUT uploaded | Shows which setting keeps the LUT in charge |
| C3 | `expert2` | Full SDR, report, Undo, run again | Pass marks met; Undo returns to the C0.1 readings |
| C4 | `hdr_cinema` | Full HDR10 | Pass marks met |
| C5 | `dolby_cinema_dark` | USB configuration, then grey | Pass marks met; the pop-up appeared before the 1D upload |
| C6 | All three | The one-button run, end to end | Section 1 met with no interaction beyond the optional pop-up |

## 12. Work packages, in order

| # | Package | Done when |
| --- | --- | --- |
| W1 | Colour science and LUT formats | The `cal_test` colour and format tests pass |
| W2 | Panel model, solver, `bloob-cal` | The solver tests pass on all synthetic panels |
| W3 | Simulator | Rules of 2.2 enforced; used by W4 and W5 |
| W4 | `cal-tv.js` with allow-list and both transports | `cal-tv.test.mjs` passes against the simulator |
| W5 | `bloob-meter` and `cal-meter.js` | `cal-meter.test.mjs` passes; desktop run with a fake `spotread` |
| W6 | `calibrate.js` and `cal-store.js` | The whole-run tests pass, interruption cases included |
| W7 | Carrier clips and the engine's `loop` flag | Both clips play on the desktop engine stand-in; `dovi_tool info` shows profile 8.1 |
| W8 | Calibrate screen | Desktop tour: first-time setup, progress, report, Undo |
| W9 | TV bring-up C0–C2 | All passes recorded in `native/TV-TEST.md` |
| W10 | TV runs C3–C6 | Section 1 met; results and readings saved |

The build queue puts calibration after real Dolby Vision for profile 7 remuxes. W7's profile 8.1 clip overlaps with that work.

## 13. What revision 2 corrected

1. **HDR10 colour by 3D LUT** was presented as the method. bscpylgtv's own HDR10 guide changes only the grey scale, so HDR colour is now an optional experiment (X1), adopted only on measured improvement.
2. **`CAL_START`'s side effects** (Warm 50, Wide gamut) were missing. They are now recorded and handled, and the final verification runs outside calibration mode, in the real viewing state.
3. **Data formats** were unspecified. They are now exact (2.2, 2.3) and tested byte-for-byte against bscpylgtv.
4. **The meter session** assumed `spotread` could stay open through Homebrew `exec`. It cannot (`exec` returns at exit), so the root helper `bloob-meter` holds it.
5. **The pipeline order** of the 1D and 3D LUTs was assumed. It is now measured (C2.3), and the method follows the result.
6. **The Dolby Vision configuration** format and picture-mode number are now exact. Primaries come from a measurement step (7.3).
7. **SDR iTPG range, transport and getters** are now named unknowns with the experiments that settle them.
