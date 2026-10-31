# Pal playbook: SDR grey calibration

This is for **Claude Code on the owner's computer**, acting as **Pal**. Pal runs `bloob-cal`'s Pal commands on the TV over SSH, reads each result, decides the next step and explains every decision. The design is in `research/Pal-in-Bloob.md` §7–11. Bloob 3.4.0 carries the commands.

## Rules that never bend

- **Nothing on the TV is deleted.** Pal's files are only moved, by `pal-end` into `calibration/history/`.
- **Any failure ends in `pal-end undo`.** That covers anything Pal can't explain or fix. The TV is never left half-calibrated.
- **Pal never writes a table directly.** It sends adjustments; `bloob-cal` builds the table, the safety gate checks it, and the TV must read it back as sent.
- **No keys, passwords or stream addresses** go into notes, reports or commits.
- **One command at a time.** Wait for each command's `PAL {...}` line before sending the next.

## The goal

Grey ΔE2000 relative to the target white (D65, BT.1886, 140 cd/m² for Expert 1):

- **Average ≤ 0.5**, and **every level from 10% to 100% ≤ 1.0**. White within 2% of 140 cd/m².
- 0% isn't scored (OLED black). 5% is reported but not scored (too dark for the Spyder5).
- **Never worse:** no scored level may end worse than in the first report. `apply-1d` lists any in `worse_than_start`.
- **Stop** when the goal is met, when what's left is within the meter's wobble, or after **3 passes**.

## Talking to the TV

```text
TV=root@192.168.1.26
CAL=/media/developer/apps/usr/palm/applications/com.muhib.bloob/bin/bloob-cal
DIR=/media/internal/com.muhib.bloob/calibration/pal
ssh $TV $CAL <command>
```

If `$CAL` isn't there, `ls /media/developer/apps/usr/palm/applications/` shows where Bloob is installed.

Every command prints its result as one line beginning `PAL ` (JSON). The exit codes:

| Code | Meaning |
| --- | --- |
| 0 | Done |
| 2 | Failed |
| 3 | Another run is going |
| 4 | The gate refused the table; nothing was written |
| 5 | The TV did not load the table |
| 6 | A pre-flight check failed; nothing was changed, or it was undone |

Sending your own adjustment from Windows PowerShell:

```powershell
Get-Content adjust.json | ssh root@192.168.1.26 "cat > /media/internal/com.muhib.bloob/calibration/pal/pal-adjust.json"
```

An adjustment is `{"pct":[0,5,…,100], "r":[21 multipliers], "g":[…], "b":[…], "for_table":"<fnv from the report>"}`.
- A multiplier of 1 leaves the table as it is at that level; 0.98 lowers that channel's table value there by 2%.
- Multipliers must be within 0.5–1.5.
- `for_table` makes `apply-1d` refuse an adjustment made for another table.

## The flow

1. **The owner** hangs the Spyder flat in the middle of the screen and opens Bloob → Settings → Calibrate → **Pal session** (OK twice).
   - The screen turns see-through with a small card in the corner.
   - Nothing else is needed from them until the end.
2. **`pal-start`** (or `pal-start 140`). It:
   - checks Expert 1;
   - checks the meter is on the squares and centred (a window drawn beside it must stay dark);
   - switches off the processing that moves readings, and reads it back;
   - warms up until white holds steady;
   - starts from LG's factory data with the gamma switches off;
   - finds out which table entry each level uses;
   - sets the light so the corrected white will reach the target ("white first");
   - measures the meter's wobble.

   If it says **the squares are not reaching the meter**:
   1. Run `pal-screen video on`; the screen plays a black clip under the squares.
   2. Run `pal-start` again.
   3. Note which way worked: it settles whether LG's squares need a video under them on this TV (`research/LG-calibration-best-practices.md`).
3. **`grey-report`**: the before report, measured as the owner watches (outside calibration mode).
   - Read `summary`, then any level with `suspect: true` and its `note`.
4. **Decide.**
   - If `goal_met`, go to step 7.
   - Otherwise run **`pal-propose`** for the reference correction. Read its `notes`. If it says white can't reach the target, run `pal-light <cd/m²>` first, then a new `grey-report`.
   - Accept the proposal, change it (damp it, leave a doubtful level alone), or write your own.
   - Record why with `pal-note "<reason>"`.
5. **`apply-1d $DIR/latest-proposal.json`** (or your own file). It:
   - checks the table;
   - sends it;
   - confirms the read-back equals what was sent;
   - measures the after report, returned in `after`.
6. **Read the after report and diagnose before the next pass.**
   - The default is to build on the table now in the TV: the next `pal-propose` measures from it.
   - Clear back to the start (`apply-1d --start`) only when a pass made a level worse than at the start.
   - Use `damp` 0.8 for passes 2 and 3 (`pal-propose $DIR/latest-report.json 0.8`).

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

## Recording the run

Copy the session folder from the TV into the repository and commit it. It holds reports, proposals, tables and the log, with no keys:

```text
scp -r root@192.168.1.26:/media/internal/com.muhib.bloob/calibration/history/pal-<started> research/pal-runs/
```

Then score the run on the rubric (`research/Pal-in-Bloob.md` §10) and add the scores to the folder as `rubric.md`. Most of it comes from the reports:

| Dimension | Where the score comes from |
| --- | --- |
| 1 Result | The last report's `summary` |
| 2 Never worse | `worse_than_start` |
| 3 Proof | `loaded: true`, `in_calibration_mode: false` |
| 5 Time | `seconds` and `readings` |
| 6 Passes | The number of `applied-*.json` files |
| 7 Hands-off | Count your requests to the owner |

## Rehearsal on a desktop

`python3 native/tools/pal-sim/pal_sim.py native/build-host/bloob-cal` runs the same commands against a stand-in TV, with every fault the plan names. `native/tools/pal-sim/screen_test.py` drives Bloob's Calibrate screen through a Pal session.
