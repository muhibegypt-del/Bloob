# Glass: implementation plan (Bloob 2.0)

The owner chose **Glass** from the concept canvas on 14 October 2026. Glass is full-bleed artwork, with controls floating on frosted panels, Inter type on Apple's public tvOS scale, and a calm, picture-first hierarchy. This file is the build plan. Work it phase by phase, cite the phase ID (for example **G3**) in commits, and tick the status table as phases land. `DESIGN.md` is rewritten for Glass in **G8**. Until then this file is the source of truth for the new system.

**Sources:**
- Apple's public tvOS Human Interface Guidelines (typography, layout, focus) for the shared tvOS conventions.
- The Elements of A design-system study and its 37-question set on `bloob-v1` (`3b51f94`, `6b668c1`) for principles only: semantic roles, focus states, adaptive contrast, effects tiers and player rules.
- Bloob picks its own values wherever a value is the reference app's own choice rather than Apple's public convention. No reference code, assets or one-off tokens are copied.

## The constraint that shapes everything

Bloob draws with SDL2 on the TV, with no shaders. Video plays in LG's hardware plane *under* our window, so the UI cannot read video pixels. Therefore:

1. **Frosted glass over artwork is baked, not live.** A panel's frost is a crop of the already-blurred backdrop for that panel's rectangle. It is tinted, has its rounded corners baked in, and is made once on an image worker thread. It is then cached and crossfaded like any artwork. Panels stay put, which is cheap. Moving panels use the solid fallback while they move.
2. **Over video, glass is smoked, not frosted.** It is a dark translucent tint with a light top edge and a hairline. The video stays sharp under it. This is a platform limit, not a choice.
3. **Every glass surface has a solid fallback.** The solid surface is `#2C2C2E` (Apple's dark grey) at high opacity. It is used:
   - in the Lite effects tier (measured slow, from 1.2);
   - when the new **Reduce transparency** setting is on;
   - while the frost for a new backdrop is still baking.

## Design system (tokens land in G0)

| Token | Glass value | Was (1.2) |
| --- | --- | --- |
| Safe area | 80 left and right, 60 top and bottom (tvOS HIG) | 96 / 54 |
| Type scale (size/leading) | Title1 76/96, Title2 57/66, Title3 48/56, Headline 38/46, Callout 31/38, Body 29/36, Caption1 25/32, Caption2 23/30. Display 104 for hero titles only. Floor 23. | 76/48/34/28/26/24/23/22 |
| Weights | Inter Bold for titles, SemiBold for headlines and buttons, Medium for body and meta | similar |
| Labels | Primary white 100%, secondary 65%, tertiary 40%. **High contrast:** 100 / 85 / 75 | 100 / 73 / 50 |
| Ground | `#000000` behind artwork; `#0B0B0D` on plain pages | `#0B0C0E` |
| Glass (frost) | Blurred backdrop crop, white tint 14%, 1 px top highlight at 30%, hairline at 15% | none |
| Glass (smoked, over video) | Black 45%, top highlight at 22%, hairline at 12% | none |
| Glass (solid) | `#2C2C2E` at 94% | `#1C1E23` at 96% |
| Radii | Pills and buttons fully round; cards 14; panels 32; small badges 6 | 14 / 28 |
| Focus, controls | White fill, black label, 1.06 scale, soft shadow | white fill |
| Focus, cards | 1.08 scale, shadow, diagonal sheen at 22% fading to 0 by 40% | 1.08, shadow |
| Candidate (chosen, not focused) | White at 20% plus a check mark | n/a |
| Disabled | Labels at 30% | n/a |
| Motion | Focus in ≈100 ms, out ≈200 ms; overlays 300 ms; page 320 ms; backdrop crossfade 350 ms after decode | similar |
| Scrims | **Adaptive.** Strength follows the luminance of the artwork under the text, measured at decode: a bright image gets the strong wash, a dark one the light wash | fixed |
| Cards | 16:9, 320×180 with 40 gaps (tvOS 5-column grid); posters 2:3 where a row is poster-led | 384×216, gaps 32 |

## Phases

| ID | Phase | What lands | Done when |
| --- | --- | --- | --- |
| **G0** | Tokens and type | Glass tokens in `theme.h`; tvOS type roles (temporary `TV*` names beside the old ones until G8); label roles with high contrast; safe area constants; new focus timings | Builds; old screens unchanged |
| **G1** | Materials | `Glass` primitives: frost bake (image worker: blur, crop, tint, round), smoked and solid fallbacks; adaptive scrim (artwork luminance measured at decode, exposed on `ImageRef`); card sheen texture; candidate and disabled states in `ui.cpp` | Unit test for crop and round; desktop render of a frost panel over artwork |
| **G2** | Shell | Floating glass tab bar at the 60 px safe line (frost over Home art, solid on plain pages); clock top right; toasts and the exit prompt on glass | Tour shots of every tab |
| **G3** | Home | Full-bleed hero, the focused or featured title at full brightness with adaptive scrims. Hero block: source line, logo or title in Display, one-line synopsis, meta with glass badges, Play and Details. Shelves of 320×180 cards with the focused title beneath. The hero follows shelf focus after the 250 ms dwell. Continue Watching cards show title, time left and progress inside the card. | Matches the Glass Home artboard; focus restore (F4) still holds |
| **G4** | Title page and How to watch | Full-bleed artwork with two fold states: (1) artwork, title and Play; (2) Down raises the **How to watch** glass panel (best version ticked, two alternatives) and, for series, seasons and episodes, while the artwork steps back. Ranking is fetched when the page opens, so Play starts the best version directly. The Sources screen remains only for *All versions*, file and quality questions, and resolve progress, restyled as a glass sheet. | Play reaches the player without passing the chooser; the alternatives are one press away |
| **G5** | Player | A smoked-glass control bar floating 56 px from the bottom: title, meta, track buttons (Audio, Subtitles) and the timeline inside. A smoked clock pill. Timers 5 s, or 8 s when paused or focused on a control; fade 300 ms; seek commit 400 ms; never seek into the last second. Subtitles sit above the bar (U7 positions recalibrated). Up next is a glass card. Startup and error screens on glass. | Player tour shots; timers verified in the log |
| **G6** | Everything else | Search (keyboard on solid glass, results in the 5-column grid), Settings (glass rows plus the new **Reduce transparency** and **High contrast** switches), pairing sheet, diagnostics, toasts, the welcome note | Every screen in the tour is Glass |
| **G7** | Spoken interface | Each focusable widget carries a label, role and position ("Play, button", "Sintel, 3 of 12"). When LG Audio Guidance is on, focus changes speak through `com.webos.service.tts` (interrupting stale speech). The service name and parameters are verified on the TV first. | The TV speaks focus changes with Audio Guidance on |
| **G8** | Finish | Remove the old type names and rename `TV*` to the final names; rewrite `DESIGN.md` for Glass; update the APP-MAP and QUALITY layers; extend TV-TEST (frost bake time, Lite fallback, Reduce transparency, TTS); version **2.0.0**; IPK | Clean build, tour, docs, IPK |

Order: G0 → G1 → G2 → G3 → G4 → G5 → G6 → G7 → G8. G3 and G4 change the most code. G4 also changes the flow (Play no longer stops at the chooser), so it gets its own TV check.

## Performance budget for Glass on this TV

These are design targets to measure, not known results:
- Frost bakes run on the image workers and never on the UI thread. One panel is about 1760×200×4 bytes, roughly 1.4 MB.
- The full-bleed hero reuses the 1920×1080 backdrop texture that 1.2 already decodes. There is no second full-size copy except the blurred quarter-resolution one.
- If render p95 goes over 25 ms in two windows, the 1.2 Lite step-down switches all glass to solid and drops the sheen. Check it with the `[perf]` lines on the TV.

## Risks

- **Baked frost lags a backdrop change** by one worker decode. The solid fallback crossfades into frost when it lands, so check on the TV that this reads as calm, not flickery.
- **The TTS service name and payload are unverified on webOS 24.** G7 starts with a read-only lookup and a single test utterance.
- **Folding Sources into the title page (G4) changes the core flow.** Keep *All versions* one press away, and keep the resolve progress and error states.

## Status

| Phase | State | Commit |
| --- | --- | --- |
| G0 | Done (desktop) | 66e82a4 |
| G1 | Done (desktop): frost bake, crossfade-safe panes, sheen, luminance | 66e82a4 |
| G2 | Done (desktop): tab bar and toasts on glass; the bar fades in place (a moving pane would need a new frost each frame) | 66e82a4 |
| G3 | Done (desktop): Home, Films, Series | 66e82a4 |
| G4 | Done (desktop, 5702679): title page with direct Play and a frosted How to watch panel (films); seasons and episodes in Glass; Play from Home, Details, episodes and Up next starts the best version with only the progress sheet showing (Cancel returns to where Play was pressed); All versions restyled as a glass sheet | 5702679 |
| G5 | Done (desktop): smoked bar (title, meta with stream badges and Paused/Seeking state, Play/Pause, Audio, Subtitles, Start over, Next episode above the timeline), smoked clock pill with "Ends", white seek bubble, track panels standing on the bar, Up next as a smoked card with a countdown line, startup and error on glass. Timers 5 s, 8 s when paused or on a control (paused no longer pins the bar), fades 300 ms, seek commit 400 ms (session test updated), the last second was already excluded. Up now also reaches the controls. Subtitle lift levels (U7) unchanged until measured on the TV | 23e08e6 |
| G6 | Done (desktop): Search (keyboard on solid glass, white focus keys, recent searches as glass buttons, results in the 5-column grid when the keyboard steps aside), Settings (glass rows, switches, new **Reduce transparency** and **Increase contrast** saved by the service as `reduceTransparency` and `highContrast`), About and Diagnostics in the Glass type, the pairing sheet on frosted glass over whichever screen opened it (`Ctx::glass`). Reduce transparency also makes smoked glass solid. Frost bake now presses bright areas down (soft knee at luma 84, less white wash) so white text stays readable on bright art | this commit |
| G7 | Planned | |
| G8 | Planned | |
