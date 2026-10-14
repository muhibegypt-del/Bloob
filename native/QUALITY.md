# Bloob quality ranking

This file defines what "quality" means for Bloob, scores each layer of `APP-MAP.md` against that definition, and ranks the layers from weakest to strongest. Re-score a layer whenever an increment lands on it or is tested on the TV.

Scored on 13 October 2026, against **1.1.0**.

## Where the definition comes from

| Source | What it contributes |
| --- | --- |
| [ISO/IEC 25010](https://iso25000.com/index.php/en/iso-25000-standards/iso-25010), the international software product quality model (2023 revision) | Splits quality into functional suitability (complete, correct, appropriate), performance efficiency, interaction capability, reliability (faultlessness, fault tolerance, recoverability), security, maintainability (modularity, analysability, testability) and related characteristics. Interaction capability includes **inclusivity**: use by people of different abilities, ages and languages. ISO also has a *quality in use* model, where context coverage means "works in the real context of use". |
| [Nielsen, response time limits](https://www.nngroup.com/articles/response-times-3-important-limits/) | 0.1 s feels instant. 1 s keeps the user's flow. Beyond 10 s, attention is lost unless the app shows progress and offers a way to cancel. |
| [Nielsen, 10 usability heuristics](https://www.nngroup.com/articles/ten-usability-heuristics/) | Visibility of system status. Helping users recognise, diagnose and recover from errors. |
| Streaming quality of experience ([Mux](https://www.mux.com/articles/qoe); [Krishnan and Sitaraman 2012](https://www.researchgate.net/publication/268584515_Video_stream_quality_impacts_viewer_behavior)) | The metrics that predict abandonment are startup time, rebuffering ratio and playback failure rate. Viewers start leaving after **2 s** of startup, and each extra second costs about **5.8%** of them. Aim for a rebuffering ratio **below 1%**. |
| 10-foot UI guidance ([Apple tvOS focus and selection](https://developer.apple.com/design/human-interface-guidelines/focus-and-selection), [Android TV navigation](https://developer.android.com/training/tv/get-started/navigation), [Amazon Fire TV](https://developer.amazon.com/docs/fire-tv/design-and-user-experience-guidelines.html), [Spyrosoft TV UX](https://spyro-soft.com/blog/media-and-entertainment/8-ux-ui-best-practices-for-designing-user-friendly-tv-apps)) | **Every visible control must be reachable with the D-pad.** Focus must be instantly obvious. Back must be predictable, and Android advises against gating the exit with a confirmation. Text is read from about 3 m; body text should be **at least about 22 px**. Test on a real TV, from the sofa. |

## The heuristic: seven dimensions

Each layer gets a score from 0 to 4 on every dimension that applies to it. A dimension that does not apply is marked "–". **The layer's quality is the mean of the dimensions that apply.**

| Dimension | Question | Grounded in |
| --- | --- | --- |
| **C**: Correct and complete | Does it do its whole job without known bugs a viewer will hit? | ISO functional suitability |
| **R**: Reliable and recoverable | When something fails, does it survive, recover, and tell the viewer what to do? | ISO reliability; Nielsen heuristics 1 and 9 |
| **P**: Responsive and efficient | Does it meet 0.1 s focus, 1 s navigation, 2 s startup and under 1% rebuffering, and has anyone measured it? | Nielsen limits; streaming quality of experience |
| **I**: Easy from the sofa | Is every control reachable by D-pad, focus obvious, Back predictable, text legible at 3 m? | Apple, Android, Amazon and Spyrosoft TV guidance; ISO interaction capability |
| **A**: Inclusive | Does it work for every title's language and script, other abilities (text size, voice), and other remotes? | ISO inclusivity and accessibility |
| **M**: Maintainable and testable | Is it modular, and would a test fail if it broke? | ISO maintainability |
| **E**: Proven where it runs | How strong is the evidence that it works *on the TV*? | ISO quality in use (context of use) |

**Anchors for C, R, P, I, A and M:**

| Score | Meaning |
| --- | --- |
| 0 | Absent or broken. |
| 1 | Has defects or gaps that an ordinary viewer will hit. |
| 2 | Works on the main path, with notable gaps, or the property is not measured. |
| 3 | Solid, with minor gaps. |
| 4 | Meets the external benchmark, and that is proven. |

**Anchors for E:**

| Score | Meaning |
| --- | --- |
| 0 | Not run anywhere. |
| 1 | Desktop build or unit tests only. |
| 2 | Earlier version ran on the TV, or partial TV proof. |
| 3 | Owner reported it working on the TV. |
| 4 | Measured on the TV. |

Security (ISO) is folded into **C** for the layers that handle the AllDebrid key, links or the local API.

## Scores

Layers 23 (external services) and 24 (the legacy web app) are not scored. One is not our code, and the other is not the product track.

| # | Layer | C | R | P | I | A | M | E | **Quality** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 21 | Testing and tooling | 2 | 1 | – | – | – | 1 | – | **1.33** |
| 8 | Settings and diagnostics | 1 | 2 | – | 2 | 1 | 2 | 1 | **1.50** |
| 4 | Search | 2 | 2 | 2 | 2 | 1 | 1 | 1 | **1.57** |
| 5 | Details, seasons, episodes | 1 | 3 | 2 | 2 | 1 | 1 | 1 | **1.57** |
| 9 | Player engine (Buna) | 2 | 2 | 1 | – | 1 | 3 | 2 | **1.83** |
| 12 | Rendering engine | 1 | 3 | 2 | – | 0 | 3 | 2 | **1.83** |
| 1 | UX: journeys and remote rules | 2 | 3 | 2 | 2 | 1 | 2 | 1 | **1.86** |
| 2 | UI: visual system | 3 | – | 2 | 2 | 1 | 3 | 1 | **2.00** |
| 6 | Sources and pairing | 2 | 3 | 2 | 3 | 1 | 2 | 1 | **2.00** |
| 7 | Player screen | 2 | 3 | 2 | 3 | 1 | 2 | 1 | **2.00** |
| 22 | Observability | 2 | 2 | 1 | – | – | 2 | 3 | **2.00** |
| 3 | Home and browse | 3 | 3 | 2 | 3 | 1 | 2 | 1 | **2.14** |
| 10 | App shell and lifecycle | 3 | 2 | 1 | 3 | – | 2 | 2 | **2.17** |
| 17 | Sources and ranking (service) | 2 | 2 | 3 | – | – | 3 | 1 | **2.20** |
| 18 | AllDebrid pairing and resolving | 3 | 2 | 2 | – | – | 3 | 1 | **2.20** |
| 11 | Remote input | 2 | 3 | 3 | 2 | 1 | 2 | 3 | **2.29** |
| 13 | Artwork pipeline | 3 | 3 | 2 | – | – | 2 | 2 | **2.40** |
| 14 | Data, workers and bus | 3 | 2 | 2 | – | – | 2 | 3 | **2.40** |
| 19 | Progress and settings store | 3 | 3 | 3 | – | – | 3 | 1 | **2.60** |
| 16 | Catalogue | 3 | 3 | 3 | – | 2 | 2 | 3 | **2.67** |
| 15 | Service core and security | 3 | 3 | 3 | – | – | 2 | 3 | **2.80** |
| 20 | Platform and packaging | 3 | 3 | – | – | – | 3 | 4 | **3.25** |

## Re-score after 1.2.0 (14 October 2026)

1.2.0 built the consolidated implementation list (see `IMPLEMENTATION-UX.md`, *Status in 1.2.0*). Nothing in it has run on the TV, so **E stays where it was for every layer**, and **P only moves where something is now measured in the log**, not because it is faster. The table above is the 1.1 baseline; these are the layers whose scores changed.

| # | Layer | Changed dimensions | Why | 1.1 → **1.2** |
| --- | --- | --- | --- | --- |
| 8 | Settings and diagnostics | C 1→3, I 2→3 | Preferred audio now chooses the starting track; subtitles, autoplay, Reduce motion, Disconnect and About exist | 1.50 → **2.00** |
| 4 | Search | C 2→3, I 2→3 | Distinct empty state; the keyboard gives way to a 4-column result grid | 1.57 → **1.86** |
| 5 | Details, seasons, episodes | C 1→2, M 1→2 | Season strip scrolls (untested on a long show); the main button's choice is unit-tested | 1.57 → **1.86** |
| 9 | Player engine (Buna) | C 2→3, R 2→3, M 3 | Tracks restored after reconnects, preferences applied; `policyAction` handled; refused start option retried; QoE accounting unit-tested | 1.83 → **2.17** |
| 12 | Rendering engine | C 1→2, A 0→2 | Every Latin, Greek and Cyrillic glyph Inter has; CJK still missing; effects step down when slow | 1.83 → **2.33** |
| 1 | UX: journeys and remote rules | A 1→2, M 2→3 | Reduce motion; one tested owner for Play | 1.86 → **2.14** |
| 2 | UI: visual system | I 2→3, A 1→2 | 22 px floor; Reduce motion | 2.00 → **2.33** |
| 6 | Sources and pairing | C 2→3 | Warnings for what this TV cannot play (DTS, disc Dolby Vision, very high bitrate) | 2.00 → **2.14** |
| 7 | Player screen | C 2→3 | Clock, "Ends at", clearer track labels, legible title, reduced seek mode | 2.00 → **2.14** |
| 22 | Observability | C 2→3, P 1→2 | `[launch]`, `[perf]` and `[qoe]` lines exist; TV numbers still to collect | 2.00 → **2.40** |
| 10 | App shell and lifecycle | R 2→3 | Network status known; offline errors say so | 2.17 → **2.33** |
| 17 | Sources and ranking (service) | C 2→3 | TV profile table; parser claims for 3D, disc images, samples | 2.20 → **2.40** |
| 14 | Data, workers and bus | R 2→3 | Chunk overflow closed; timeout and not-responding errors told apart | 2.40 → **2.60** |

Unchanged: **21 Testing and tooling (1.33)** is now the clear weakest layer, because the QA items were deliberately skipped. **A** is still the weakest dimension (no CJK font, no text size, no voice or pointer), and **E** cannot move until `TV-TEST.md` §7 is run.

## Ranking, weakest first

Scores within about 0.15 of each other are effectively tied. The heuristic is a structured judgement, not a measurement, so read the tiers rather than the decimals.

### Tier 1: weakest (below 1.6)

**1. Testing and tooling (21): 1.33.**
- The tour takes pictures and checks nothing, so a broken screen still "passes".
- There is no CI.
- Screen logic has no tests: `choose_start`, next episode, model parsing.
- This is weak *quality infrastructure*. Every other layer's score is less trustworthy because of it.

**2. Settings and diagnostics (8): 1.50.**
- Its one preference, *Preferred audio language*, does not choose the audio when playback starts. It only changes which version is recommended. A setting that does not do what its name says is a correctness defect.
- No unpair, no subtitle language, no autoplay switch.

**3. Search (4) and Details (5): 1.57 each.**
- *Search:* typing with a D-pad keyboard is the slowest input on a TV, and there is no voice search. There are no tests.
- *Details:* **seasons whose pills do not fit across the screen are never drawn, so they cannot be reached.** That breaks the one rule every TV guideline shares (every visible control reachable by D-pad), and it also hides content. The resume and next-episode logic has no tests.

### Tier 2: weak (1.8 to 2.0)

**4. Rendering engine (12): 1.83.**
- Inclusivity is 0: the glyph atlas holds Latin-1 only.
- Any title or episode name in Polish, Turkish, Greek, Russian, Arabic, Japanese and other languages shows missing characters.
- This affects every screen that shows a title.

**5. Player engine (9): 1.83.**
- The three streaming metrics that predict abandonment (startup time, rebuffering ratio, failure rate) have **never been measured on the TV**, and rebuffering is not recorded at all.
- Tracks are lost after a reconnect, and an expired link is not re-resolved.
- The core is well built (platform-free and unit-tested). It is the proof and the edges that are missing.

**6. UX (1): 1.86.**
- No one has done the acceptance task on the TV.
- Back on Home is gated by "Press Back again to close Bloob". Android TV guidance says not to gate the exit. It is a mild gate (a toast, not a dialog), but it is a choice to make deliberately (**1.4**).

**7. UI (2), Player screen (7), Sources (6), Observability (22): 2.00 each.**
- *UI:*
  - The type floor is 19 px for footnotes, badges and eyebrows, and 21 px for captions. That is below the roughly 22 px the TV guidance recommends for reading at 3 m.
  - The tertiary text (50% white) is unverified on OLED.
- *Player screen:* no clock or "ends at" time, no thumbnails, no subtitle styling.
- *Sources:* the ranker is untuned on real results, and nothing shows whether a release is ready right away.
- *Observability:* no per-session streaming metrics.

### Tier 3: middling (2.1 to 2.4)

These are Home (3), the app shell (10), the source ranking service (17), AllDebrid (18), remote input (11), artwork (13), and data and bus (14). Each mostly works and is reasonably built. The low points:

- **Shell (10):** launch time is unmeasured.
- **AllDebrid (18):** persistence on the TV is unproven, and it was the owner's original complaint.
- **Remote input (11):** the Magic Remote pointer is ignored. The Magic Remote is LG's standard remote, so this matters more on an LG TV than the Apple guidance against pointers suggests.

### Tier 4: strongest (2.6 and up)

These are the progress store (19), catalogue (16), service core (15), and platform and packaging (20). Each is tested, simple, and atomic where it writes. Packaging is the only layer proven on the TV.

## Patterns across layers

| Dimension | Average | Reading |
| --- | --- | --- |
| **A**: Inclusive | **1.0** | The weakest dimension. Every screen inherits the Latin-1 limit, and there is no larger text, voice or pointer. |
| **E**: Proven where it runs | **1.8** | 1.1 has not run on the TV, so most of the UI is desktop-only. |
| **P**: Responsive | **2.1** | Designed for the right budgets, but almost nothing is measured on the TV. |
| **M**: Maintainable | 2.2 | Good structure; tests thin outside the session, service and ranker. |
| **C**: Correct | 2.3 | A few real defects, all listed above. |
| **I**: Sofa use | 2.4 | Strong focus model, held back by the season bug, the type floor and the pointer. |
| **R**: Recovery | 2.5 | The strongest dimension: designed error states and reconnect logic. |

## What to fix first

Quality alone is not priority. How often viewers meet a layer matters too:
- every session: Rendering, UX, UI, the player screen and engine, Home, Details;
- most sessions: Sources, Search;
- rarely: Settings, Testing (which no viewer sees).

Weighing that, the order that raises the most quality for the least work is:

1. **5.1:** season pills that scroll. A real defect, on a screen used every session.
2. **12.1:** draw every character Inter has. Lifts inclusivity everywhere.
3. **21.1 + 14.1:** a tour that fails on breakage, plus model-parser tests. Makes every later fix safer.
4. **9.2 / 8.1:** make *Preferred audio language* pick the audio. Fixes Settings' correctness defect and helps the player at once.
5. **The TV pass in `TV-TEST.md`:** moves **E** and **P** from guesses to evidence for nearly every layer, including first frame, rebuffering and launch time (**22.1**, **10.1**).
6. **2.4:** raise the type floor to 22 px. **1.4:** decide on exit gating.
