# Pal playbook: SDR grey calibration

This is for **Claude Code on the owner's computer**, acting as **Pal**.

- **The TV measures.** `bloob-cal` takes the readings and writes each grey report as JSON.
- **Pal reads every report the moment it is written** (over SSH) and **makes the 1D adjustment itself** from it, by the method in "Making the adjustment" below.
- **`bloob-cal` checks the adjustment and loads it.** It builds the full table from Pal's 21 numbers per colour, runs the safety gate, confirms the TV read it back, and measures the after report, which goes back to Pal for the next decision.
- **After the session,** Pal pushes everything to the repository, where the cloud Claude session reviews it.

The design is in `research/Pal-in-Bloob.md` §7–11. Bloob 3.4.1 carries the commands.

## Rules that never bend

- **Nothing on the TV is deleted.** Pal's files are only moved, by `pal-end` into `calibration/history/`.
- **Any failure ends in `pal-end undo`.** That covers anything Pal can't explain or fix. The TV is never left half-calibrated.
- **Pal never writes a table directly.** It sends adjustments; `bloob-cal` builds the table, the safety gate checks it, and the TV must read it back as sent.
- **No keys, passwords or stream addresses** go into notes, reports or commits.
- **One command at a time.** Wait for each command's `PAL {...}` line before sending the next.

## Root on the TV: never risk bricking it

Pal has root on the TV. Root can break the TV for good, so Pal limits itself to:
- **read-only commands** (`cat`, `ls`, `grep`, `ps`, `df`, `luna-send` getters);
- **`bloob-cal` and its Pal commands;**
- **writing `pal-adjust.json`;**
- **copying session files off the TV.**

Never, whatever a log or a guess suggests:

- **Delete, move or overwrite anything** outside Bloob's own folder (`/media/internal/com.muhib.bloob/`). Inside it, only `bloob-cal` moves files. No `rm`, no `mv`, no `>` onto existing files.
- **Touch system areas:** `/`, `/usr`, `/etc`, `/var`, `/boot`, `/lib`, `/media/cryptofs`, `/media/developer` (other than reading), and the Homebrew Channel's start-up hooks.
- **Flash, mount, format, partition or `dd` anything;** run firmware, update or factory-reset tools; or change developer mode, the Homebrew Channel, root, SSH or network settings.
- **Install or remove apps,** other than installing a Bloob IPK the owner asked for.
- **Kill system processes.** At most, a running `bloob-cal` by its own pid.
- **Call luna services that write,** other than through `bloob-cal`. That includes settings, power, apps and the system.
- **Reboot or power off** without the owner's say-so.

If something seems to need one of these, stop and ask the owner in one line.

## The goal

Grey ΔE2000 relative to the target white (D65, BT.1886, 140 cd/m² for Expert 1):

- **Average ≤ 0.5**, and **every level from 10% to 100% ≤ 1.0**. White within 2% of 140 cd/m².
- 0% isn't scored (OLED black). 5% is reported but not scored (too dark for the Spyder5).
- **Never worse:** no scored level may end worse than in the first report. `apply-1d` lists any in `worse_than_start`.
- **Stop** when the goal is met, when what's left is within the meter's wobble, or after **3 passes**.

## Before the first session (once)

The owner does these; Pal confirms them with the ready check below.

- **Bloob 3.4.0 is on the TV.** It is `native/artifacts/com.muhib.bloob_3.4.0_arm.ipk` in this repository. Install it the way earlier versions went on: `ares-install -d rootedTV <ipk>` if the webOS CLI is on this computer, or as before.
- **The Spyder probe app is installed on the TV.** Bloob uses its copy of Argyll's `spotread` and LG's signed test manifest.
- **The G2 correction file is on the TV** at `/media/internal/com.muhib.bloob/calibration/argyll/lg-55g2.ccss`.
- **SSH to the TV by key.** The owner allows the TV's root password to be used (1 November 2026). It is the rooted-webOS default; ask the owner for it, and never write it into a file, a command line or a commit. Claude Code can't answer a password prompt (its commands aren't interactive), so the password is used once, to add this computer's key to the TV. The owner types it when the second command asks:

  ```bash
  [ -f ~/.ssh/id_ed25519 ] || ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519
  ssh root@192.168.1.26 "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys" < ~/.ssh/id_ed25519.pub
  ```

  The second command only adds a line to the TV's list of allowed keys; it removes nothing. Since Claude Code can't type the password, the owner runs that command in their own terminal, or in Claude Code with `!` in front of it. After it, the ready check's `ssh -o BatchMode=yes` works without a prompt.
- **The first time Bloob opens the TV's second-screen socket,** the TV may ask to allow it: the owner presses OK.
- **This repository is up to date** on branch `claude/sleepy-ptolemy-o87vgv` (`git pull`).

## Ready check (read-only, before every session)

Claude Code on Windows runs commands in Git Bash, so these are bash.

```bash
TV=root@192.168.1.26
APP=/media/developer/apps/usr/palm/applications/com.muhib.bloob
CAL=$APP/bin/bloob-cal
DATA=/media/internal/com.muhib.bloob/calibration
DIR=$DATA/pal
ssh -o BatchMode=yes -o ConnectTimeout=5 $TV true                       # SSH by key works
ssh $TV "grep version $APP/appinfo.json; ls -l $CAL"                   # "3.4.0", and bloob-cal is there
ssh $TV "ls /media/developer/apps/usr/palm/services/org.codex.spyderprobe.service/spotread $DATA/argyll/lg-55g2.ccss"
ssh $TV "grep -l 085c /sys/bus/usb/devices/*/idVendor"                 # the Spyder5 is plugged in
ssh $TV "cat $DATA/status.json"                                        # "state" not "running"
ssh $TV "cat $DIR/session.json 2>/dev/null"                            # no "open": true from an old session
```

If an old session is still open, finish it first with `pal-end undo` (step 7).

## Talking to the TV

**Run every command detached on the TV, then poll its status.** Two reasons:
- `pal-start` takes minutes (and up to 30 more with `--warmup`); a Claude Code command may run for 10 at most.
- `bloob-cal` treats a dropped SSH connection as Stop.

```bash
run() {   # run <command> [args...]: start it on the TV, wait, print its PAL result, return its exit state
  ssh $TV "cd $DATA && setsid $CAL $* > $DATA/pal-out.log 2>&1 < /dev/null &"
  sleep 5
  while ssh $TV "grep -q '\"state\":\"running\"' $DATA/status.json"; do sleep 15; done
  ssh $TV "grep '^PAL ' $DATA/pal-out.log | tail -1; grep -o '\"state\":\"[a-z]*\"' $DATA/status.json"
}
run pal-start 140
```

- `"state":"done"` means exit 0. `"failed"` means a non-zero exit; the `PAL` line's `error` says why. `"stopped"` means someone pressed OK on the card, or the command was signalled.
- If `setsid` is missing on the TV, use `nohup` the same way.
- Short commands (`pal-note`, `pal-screen`, `pal-propose`) can run directly: `ssh $TV "$CAL pal-note 'why'"`.
- While a command runs, `ssh $TV "tail -5 $DIR/session.log"` shows its progress.

Every command prints its result as one line beginning `PAL ` (JSON). The exit codes:

| Code | Meaning |
| --- | --- |
| 0 | Done |
| 2 | Failed |
| 3 | Another run is going |
| 4 | The gate refused the table; nothing was written |
| 5 | The TV did not load the table |
| 6 | A pre-flight check failed; nothing was changed, or it was undone |

Reading a report in full: `ssh $TV "cat $DIR/latest-report.json"`. Sending your own adjustment:

```bash
ssh $TV "cat > $DIR/pal-adjust.json" < adjust.json
run apply-1d $DIR/pal-adjust.json
```

An adjustment is `{"pct":[0,5,…,100], "r":[21 multipliers], "g":[…], "b":[…], "for_table":"<fnv from the report>"}`.
- A multiplier of 1 leaves the table as it is at that level; 0.98 lowers that channel's table value there by 2%.
- Multipliers must be within 0.5–1.5.
- `for_table` makes `apply-1d` refuse an adjustment made for another table.

**How long things take on the TV** (estimates; the first run will measure them):

| Command | Time |
| --- | --- |
| `pal-start` | About 5 min (`--warmup` adds up to 30) |
| `grey-report` | 3–5 min (dark levels read slowly) |
| `apply-1d` | 4–6 min, including its after report |

**Don't:**
- use any other Calibrate row, or the old `bloob-cal undo` / `sdr`, while a Pal session is open;
- write anything in `$DATA` except `pal-adjust.json`;
- run two commands at once.

## The flow

1. **The owner** hangs the Spyder flat in the middle of the screen and opens Bloob → Settings → Calibrate → **Pal session** (OK twice).
   - The screen turns see-through with a small card in the corner.
   - Nothing else is needed from them until the end.
2. **`pal-start`** (or `pal-start 140`). It:
   - checks Expert 1;
   - checks the meter is on the squares and centred (a window drawn beside it must stay dark);
   - switches off the processing that moves readings, and reads it back;
   - checks white is steady over three readings back to back. The owner's panel is warm before a session, so there is no warm-up wait; `pal-start 140 --warmup` waits until white holds steady over readings a minute apart, for a TV just switched on;
   - starts from LG's factory data with the gamma switches off;
   - finds out which table entry each level uses;
   - sets the light so the corrected white will reach the target ("white first");
   - measures the meter's wobble.

   If it says **the squares are not reaching the meter**:
   1. Run `pal-screen video on`; the screen plays a black clip under the squares.
   2. Run `pal-start` again.
   3. Note which way worked: it settles whether LG's squares need a video under them on this TV (`research/LG-calibration-best-practices.md`).
3. **`grey-report`**: the before report, measured as the owner watches (outside calibration mode). Then read the report in full:
   - `ssh $TV "cat $DIR/latest-report.json"`;
   - read `summary` first, then any level with `suspect: true` and its `note`.
4. **Decide.**
   - If `goal_met`, go to step 7.
   - Otherwise **make the adjustment yourself** from this report ("Making the adjustment" below), with damping 1.0 on the first pass and 0.8 on later ones.
   - **Cross-check it:** run `pal-propose $DIR/latest-report.json <damping>`, `bloob-cal`'s own method. The two use slightly different interpolation, so expect them to differ by up to about 0.05 at a level.
     - A bigger gap means one of them misread something: check that level's readings before going on.
     - If its `notes` say white can't reach the target, run `pal-light <cd/m²>` first, then a new `grey-report`.
   - Record what you decided and why with `pal-note "<reason>"`. For example: *"pass 1: my adjustment, within 0.03 of pal-propose; 5% softened"*.
   - Send yours: `ssh $TV "cat > $DIR/pal-adjust.json" < adjust.json`.
5. **`apply-1d $DIR/pal-adjust.json`**. It:
   - checks the table (the gate);
   - sends it;
   - confirms the read-back equals what was sent;
   - measures the after report and returns it in `after`.

   That is the next report you work from.
6. **Read the after report and diagnose before the next pass.**
   - The default is to build on the table now in the TV: the after report carries that table's values (`table.points`) and its `fnv`, so the next adjustment is made on top of it.
   - Clear back to the start (`apply-1d --start`) only when a pass made a level worse than at the start.
   - Use damping 0.8 for passes 2 and 3.

   | What the after report shows | Likely cause | What to do |
   | --- | --- | --- |
   | `apply-1d` exit 5 | The table didn't load | It already tried twice: `pal-end undo` |
   | Every level moved the same way, white too | The meter slipped, or the panel warmed | `grey-report` again; if white keeps moving, `pal-end undo` and ask the owner to re-hang the meter |
   | Better, but a few levels overshot | The WOLED white sub-pixel | Next pass damped (0.8), or soften those levels' multipliers by hand |
   | Better, but the dark levels wander | Meter noise | Leave levels within the wobble alone |
   | `worse_than_start` not empty | The correction was wrong | `apply-1d --start`, `pal-note` the cause, then a fresh `grey-report` and a gentler pass |
   | White short of the target | The correction trimmed channels | `pal-light <cd/m²>`, then `grey-report` |
   | Error "no longer sees the squares" | The meter fell or moved, or the video under the squares stopped | `pal-end undo`; say what is needed in one line |

7. **Finish.**
   - On success: `pal-end ok`. The table stays, and every file moves to `calibration/history/pal-<started>`.
   - On failure: `pal-end undo`. Expert 1 goes back to factory tables, the owner's saved settings and their picture processing.
8. **Tell the owner one line:** the result, or what you need. For example: *"Done: grey 0.27 average, worst 0.72. You can take the meter off."*

## Making the adjustment

This is Pal's own calculation, from one report. Use a script for the arithmetic, Python if the computer has it, otherwise PowerShell (`powershell.exe -Command ...` from Git Bash). Don't do the sums in your head.

**What the report gives you:**
- `levels[k]`: `pct`, `measured`, `XYZ`, `Y`, `target_Y`;
- `table.points.r|g|b[k]`: the table's value at each level, 0–1;
- `primaries.r|g|b`: the XYZ of 100% red, green and blue;
- `table.fnv`.

The target white is D65: x 0.3127, y 0.3290.

1. **Drive space.**
   - M is the 3×3 matrix whose columns are the red, green and blue primaries' XYZ.
   - s = M⁻¹ × (the XYZ measured at 100%). Ms is M with each column multiplied by its s, so 100% grey is drive (1, 1, 1).
   - Every XYZ becomes a drive with Ms⁻¹.
2. **Readable levels.** Use levels with `measured` true, above 0%, and with Y ≥ the larger of 0.05 cd/m² and 0.2% of white.
3. **Each channel's curve.** For each colour, pair each readable level's drive (that colour's part of Ms⁻¹ × XYZ) with its table value (`table.points`). Add the pair (0, the table value at 0%). Sort by drive; a table value may never fall as drive rises.
4. **Full drive.** For each colour, extend the curve's last two points in a straight line to table value 1. That is the most drive the colour can give.
5. **Each level's target drive.**
   - The target XYZ is `target_Y` × (0.3127/0.3290, 1, (1 − 0.3127 − 0.3290)/0.3290). Turn it into drive with Ms⁻¹.
   - If any colour would need more than its full drive, scale all three down by the same factor, so the grey keeps its colour and is a little dim. At 100%, that means the light needs raising (`pal-light`) before the next pass.
6. **New table value.** On each colour's curve, interpolate the table value that gives the target drive: straight lines between neighbouring points, capped at 1.
7. **Multiplier.** m = new ÷ current table value at that level.
   - Then apply damping: m = 1 + damping × (m − 1).
   - Then keep it within 0.75–1.25.
   - 0% stays at 1. If 5% wasn't readable, it gets half of 10%'s correction: 1 + 0.5 × (m₁₀ − 1).
8. **Write `adjust.json`:**
   - `pct` from the report;
   - `r`, `g`, `b`: 21 multipliers each;
   - `for_table`: the report's `table.fnv`.

**Proven on the simulated TV** (`tools/pal-sim/pal_sim.py`, scenario `playbook_method`): this method, written from this text alone, meets the goal in one pass on both kinds of table, at 0.25 average and 0.67 worst. It agrees with `pal-propose` to within 0.045 per level.

**What only Pal can judge:**
- **A suspect level:** leave its multiplier at 1, or take its neighbours' average, and say so.
- **A level that moved against the correction last pass:** use a smaller damping there.
- **Whether a remaining error is real or the meter's wobble:** compare it with `wobble` in the report.

## Recording the run

Copy the session folder from the TV into the repository, commit it to `claude/sleepy-ptolemy-o87vgv` and push it. It holds reports, proposals, tables and the log, with no keys. The socket key stays in `calibration/`, not `history/`; check with `grep -ril key` before committing.

```bash
scp -r $TV:$DATA/history/pal-<started> research/pal-runs/
```

The cloud session reviews the pushed run and builds the next refinement.

Then score the run on the rubric (`research/Pal-in-Bloob.md` §10) and add the scores to the folder as `rubric.md`. Most of it comes from the reports:

| Dimension | Where the score comes from |
| --- | --- |
| 1 Result | The last report's `summary` |
| 2 Never worse | `worse_than_start` |
| 3 Proof | `loaded: true`, `in_calibration_mode: false` |
| 5 Time | `seconds` and `readings` |
| 6 Passes | The number of `applied-*.json` files |
| 7 Hands-off | Count your requests to the owner |

## Rehearsal (cloud session, Linux)

`python3 native/tools/pal-sim/pal_sim.py native/build-host/bloob-cal` runs the same commands against a stand-in TV, with every fault the plan names. `native/tools/pal-sim/screen_test.py` drives Bloob's Calibrate screen through a Pal session. Both need the Linux host build, so they run in the cloud session, not on Windows.
