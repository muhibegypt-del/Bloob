# Agentic calibration for Bloob

Written 30 October 2026, in answer to the owner: "an agentic element could be a game changer." This note looks for why. It is a design proposal; nothing in it is built.

Labels: **[history]** what happened in Bloob 3.3.0–3.3.8; **[code]** measured in this repository; **[tv]** seen on the owner's G2; **[ext]** outside evidence; **[P]** proposal.

## 1. The owner is right, and why I was wrong

My first answer treated the agent as a replacement for the solver, and argued (correctly) that a language model should not compute table values. That answers the wrong question. Bloob's calibration has not failed on its maths once. **It has failed eight releases in a row on things nobody had written code for yet**, and each fix needed a round trip: the owner runs, pastes a log, I deduce, I build, the owner installs.

### Where the code is [code]

| Part | Lines | What it is |
| --- | --- | --- |
| Maths: grey, colour, LUT, colour model, HDR | 1,482 | Solves and fits. Proven on the simulator; never the cause of a TV failure |
| Plumbing and recovery: TV device, socket, meter, measurer, procedure, `bloob-cal`, Calibrate screen | 3,038 | A hand-written decision tree of failures someone anticipated |
| Simulator | 272 | |

Two thirds of the code is the part an agent does best, and it is the part that kept breaking.

### What each release fixed, and whether an agent at the TV would have needed it [history]

| Release | Failure on the TV | Found by | An agent with eyes and tools |
| --- | --- | --- | --- |
| 3.3.3 | No squares; meter read 18 cd/m² at x 0.347 y 0.543, then "measuring refresh rate failed" | Owner's eyes + my deduction a day later | Would see that a "white" square reading green-yellow at 18 cd/m² is not white, look at the screen, and stop within the first reading |
| 3.3.4 | "500 Application error" on the pattern command | Owner | Would try the other routes at once. The five-route test I wrote for 3.3.5 *is* what an agent does unprompted: an experiment |
| 3.3.5 | Root cause: LG's generator draws only while a video plays | The owner's observation of the 3.3.5 test | Same experiment, same session, minutes not days |
| 3.3.6 | Black readings ~20 s, screen said "0 readings", magenta flashes | Owner | Would say "dark patch, the meter is slow" as it happens, and notice the magenta on a camera |
| 3.3.7 | Spotread refresh calibration failing on OLED | PGenerator+ study | Would retry with `-y c`, `-N`, `-Y R:<Hz>` and keep the combination that works |
| 3.3.8 | Test 7/8: "before" read outside calibration mode, "after" inside it (white 60.5 → 54.4, xy 0.010 apart) | Owner's paste | Would recognise that shift as a colour-temperature change, not a failed reset, and compare properly |

Every row is an unanticipated environment fact. Code can only absorb such a fact after a human has met it; an agent can meet it during the run. **That is the game changer: the calibration stops being a program we ship and becomes a calibrator that is present.**

## 2. Seven things an agent changes

1. **Diagnosis during the run.** Above. The decision tree in the plumbing becomes judgment, and a new firmware quirk costs minutes, not a release.
2. **Experiments instead of a recipe.** Quick measures 26 fixed grey levels. A calibrator places readings where the error is: more near a kink at 5%, more reads where the meter is noisy, a window-size test when white drifts (is it ABL, heat or the table?). The simulator says where the fixed recipe struggles: a harsh meter passes 5 of 12 runs [code]. An agent that sees noisy residuals and switches to averaging is the obvious fix, and the right averaging depends on what it is seeing.
3. **Knowing when the model is wrong.** The solver assumes a locally linear panel and a WOLED kink. When reality departs from that (Broyden updates disagreeing with measurements, residuals with structure, a level that does not respond), the code clamps and carries on. An agent can stop, test a hypothesis and choose another strategy.
4. **Intent to target.** The owner said "a small lamp on in the corner"; we turned that into 140 nits by hand, and the BT.1886 versus gamma 2.2 question is still open. An agent can ask the owner what they watch and when, measure the room (spotread has an ambient mode for meters that support it [ext: Argyll spotread `-a`]; whether the Spyder5 supports it through the probe's spotread is untested), and choose luminance and gamma from that, explaining the choice.
5. **Verification like a person.** ΔE averages hide what eyes see: near-black steps, banding, a tint in one grey. With a camera on the screen (a phone or webcam) a vision model can confirm the patches really appear, catch notifications or the magenta, and look at a grey ramp the way a calibrator does.
6. **Memory across runs.** A record of every run: drift since last month, panel ageing, whether a recalibration is needed at all, or whether the last good table can be put back without measuring (PGenerator+ L11).
7. **Reach.** HDR10, Dolby Vision, a firmware update, a new TV. Today each is a porting project. An agent with the docs and what bscpylgtv, PGenerator+ and AVS threads already record can probe the API, find what works and write down what it learnt. There is precedent: Coscientist (Nature, 2023) used a language model as the planner over documentation search, code execution and real lab instruments, including optimisation from collected data [ext].

## 3. What stays deterministic, and why (not cost)

- **Table values come from the solver.** The agent calls it, with options, and can ask for a different fit or a refit of one region. Reason: precision and repeatability; the same readings should give the same table.
- **A gate checks every write.** Values in range, curves increasing and smooth, only allow-listed settings. A bad request is refused, whoever made it.
- **The agent is not in every reading.** Not to save money: a model turn adds seconds, and 1,900 readings would add hours of static patches on an OLED. It decides between passes and on anomalies (tens to a few hundred turns in a run).
- **Undo always works**, the drift sentinel and time limit stay, and the owner's rule holds: nothing on the TV is deleted.

## 4. Design [P]

```text
Owner ──talks──> Calibrate screen (Bloob) <──narration── Agent loop (service, Claude API, tool use)
                                                              │ tools
                    ┌─────────────────────────────────────────┼───────────────────────┐
                    │ bloob-cal serve (root, JSON lines)      │ Eyes (optional)       │
                    │  show_patch · read(n, averaging)        │  camera frame          │
                    │  get/set settings (allow-list)          │                       │
                    │  solve_grey / solve_colour (options)    │ Memory                 │
                    │  verify · undo · snapshot · video state │  calibration/history/  │
                    │  write gate                             │                       │
                    └─────────────────────────────────────────┴───────────────────────┘
```

- **Instruments:** `bloob-cal serve`, a long-lived mode of the program that exists, offering the primitives above as JSON commands. The procedure code becomes one tool among several; Quick stays as the fallback when there is no network.
- **Agent:** a loop in the Bloob service (`tv-app/service/`), which already runs on the TV with network access; the API key goes in `keys.json` with the others. It streams its reasoning in plain English to the Calibrate screen's card.
- **Journal:** every decision, with its reason and the readings behind it, saved as the run's record.
- **Eyes:** a phone on a stand sending frames, later if the TV's own capture works for the video plane (not validated).

## 5. Order of work, and how we will know [P]

| Step | What | Proves |
| --- | --- | --- |
| A0 | Claude Code on the owner's PC with SSH to the TV; I drive `bloob-cal` by hand as the agent | Which tools and observations matter, at no build cost |
| A1 | `bloob-cal serve` and the write gate | The instrument layer, tested against the stand-in TV |
| A2 | Agent loop in the service, narration on the Calibrate screen | An unattended run from Bloob |
| A3 | Eyes | Patches seen, not inferred |
| A4 | History and recalibration advice | Runs that build on each other |

Measured against Quick on the same TV: final grey and colour ΔE as watched, readings and minutes, unattended success, and failures recovered inside the run instead of by a release. If the agent does not beat Quick on at least the last two, the case is weaker than this note argues.
