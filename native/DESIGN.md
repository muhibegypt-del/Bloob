# Bloob visual system 2.0: Glass

This is the design system for the native app. `src/theme.h` holds the tokens; `src/ui.cpp` holds the type scale, the label roles and the shared components (`draw_glass`, `draw_glass_button`, `draw_glass_badge`, `draw_card`). If you change a value, change it here and in the code at the same time. `GLASS-PLAN.md` records how Glass was built, phase by phase (G0–G8).

The renders below come from `scripts/ui-tour.sh`, which drives the real app on the desktop with scripted remote presses at 1920×1080. They are **desktop renders, not TV photographs**. The TV pass is in `TV-TEST.md` §8.

**Sources:**
- Apple's public tvOS Human Interface Guidelines for the shared tvOS conventions: type sizes, the safe area, the 5-column grid, focus lift.
- The Elements of A studies for **values as well as principles**. Owner's rule (15 October 2026): measured values are copied wherever the studies record them. That covers spacing, padding, sizes, radii, type sizes, opacities, colours, timings and geometry. Code, artwork, icons, fonts, written text and branding are never copied. Every copied value is listed with its source in `TOKENS.md`. Bloob picks its own value only where the studies record none.

## The constraint that shapes everything

Bloob draws with SDL2 and no shaders. Video plays in LG's hardware plane under the app's window, so the UI can never read or blur video pixels.

- **Frosted glass over artwork is baked, not live.** An image worker takes the backdrop as drawn and scales it to quarter size. It blurs it (box blur 12), crops the panel's rectangle at half resolution and enriches the colour (×1.3 saturation). It then presses bright areas down with a soft knee at luma 84, so white text stays readable. Finally it mixes in white (14%, plus up to 10% along the top edge, less over bright areas) and bakes the rounded corners. The result is cached by rectangle and picture, and fades in like any artwork. Panels stay put; anything that moves uses solid glass.
- **Over video, glass is smoked**: black at 45% with a 12% hairline. The video stays sharp under it.
- **Every glass has a solid fallback**: `#2C2C2E` at 94%. It is used in the Lite effects tier, when *Reduce transparency* is on (which also makes smoked glass solid), and while a new frost is still baking.
- **A changing backdrop holds its fade** until the frost for every panel used in the last 2 s is ready, so glass never flashes solid during a crossfade.

## Principles

1. **The picture leads.** Title screens sit on their own full-bleed artwork at full brightness. Controls float on glass. Scrims are adaptive: their strength follows the brightness of the artwork under the text, which is measured when the image is decoded (`scrim_for`).
2. **Quiet by default.** One text colour in label roles, no accent colour, and white for focus. Colour carries status only: green, amber and red, each always paired with words.
3. **Focus is unmistakable and immediate.**
   - Exactly one thing is white or lifted at a time.
   - Focus arrives in about 100 ms and leaves in about 200 ms.
   - The backdrop follows focus only after a 250 ms dwell.
4. **Play means play.** Play on Home, a title page, an episode or Up next starts the best version directly. The alternatives are one press away (How to watch, All versions); technical detail is one more (Release details).
5. **Every state is designed.** Loading, missing art, errors, empty results, returning to a screen, and reconnecting after the TV interrupts playback all have layouts. None shows a blank screen or a raw error string.
6. **Everything can be heard.** With LG Audio Guidance on, every focus is spoken as label, role and position.

## Colour and materials

| Token | Value | Use |
| --- | --- | --- |
| `black` | `#000000` | Ground behind artwork and video |
| `plain` | `#2C2C2E` [EoA C5] | Ground of pages without artwork (Search, Settings, About, Diagnostics) |
| Frost | baked crop (see above), hairline white 15% | Tab bar and toasts over artwork, How to watch, sheets, pairing |
| `smoke` | black 45%, hairline white 12% | Player bar, clock pill, track panels, Up next, player error |
| `solid` | `#2C2C2E` at 94% | Fallback, Search keyboard panel, Release details drawer |
| `candidate` | white 10% [EoA P5] (+ check mark on buttons) | Chosen but not focused: the current tab, the selected season, the best version |
| Focus | white fill, black label | Every focused control |
| `good` / `caution` / `problem` | `#5CD58C` / `#F5B942` / `#FF7A6B` | Status, always with words |

**Labels** (white at an alpha by role) [EoA C1, C2]: Primary 100%, Secondary 50%, Tertiary 30%, Disabled 20%. **Increase contrast** raises them to 100 / 85 / 75 / 55%.

## Type

Inter, rasterised at exact sizes, on Apple's public tvOS scale. Nothing is smaller than 23 px.

| Style | Weight | Size / line | Use |
| --- | --- | --- | --- |
| Display | Bold | 104 / 112 | Hero title without a logo |
| Title1 | Bold | 76 / 96 | About |
| Title2 | Bold | 57 / 66 | Screen titles (Settings, Search landing), the best version's headline |
| Title3 | SemiBold | 48 / 56 | Sheet headings, keyboard keys, Up next |
| Headline | SemiBold | 38 / 46 | Shelf headers, How to watch, settings rows, player title |
| Callout | Medium | 31 / 38 | Meta lines, episode titles, option rows, tabs |
| Body | Medium | 29 / 36 | Descriptions, two or three lines |
| Caption1 | Medium | 25 / 32 | Secondary notes, the clock, timeline times |
| Caption2 | Medium | 23 / 30 | Episode numbers, badges, small print |
| Button | SemiBold | 29 / 36 | Glass buttons |

## Layout

- **Safe area:** 80 px left and right, 60 px top and bottom (tvOS). Artwork bleeds to the edges.
- **Tab bar:** a frosted capsule on the 60 px safe line, centred. The tabs inside are season-style pills [EoA P1–P6], 66 px tall with 30 px padding and 16 px apart [EoA F3]. The current tab is a candidate (white 10%), and turns white when the bar has focus. The bar fades in place rather than sliding.
- **Cards:** 16:9 at 320×180 with 40 px gaps, which gives the tvOS 5-column grid across 1760 px. Episode cards are 400×225.
- **Radii:** buttons and pills fully round, cards 14, glass panels 32, badges 6.

## Components

| Component | Rest | Focused |
| --- | --- | --- |
| Glass button (80 h; 60–72 in sheets) | white 14% fill with a hairline, white label | White fill, black label, ×1.06, soft shadow. Measured once and drawn from that, so the margins are equal; label centred on its capital height. 16 px between buttons [EoA F3]. |
| Pill (seasons, tabs) [EoA P1–P6] | no fill, secondary label; candidate white 10%, primary label | White fill, black label, 8 px wider |
| Card | Artwork, soft shadow. Continue cards carry the title, time left and a white progress line inside the art. | ×1.08 lift, deeper shadow, a diagonal sheen across the art; the title and context appear under the card |
| Row (versions, settings, track choices) | white 8–10% | White fill, black text, shadow |
| Switch | track white 20%, white knob | Inverted on the white row |
| Badge | white 16% with a 27% outline | none |
| Progress | white line on white 28% | none |

Missing art becomes a tone derived from a hash of the title, with the title set in type. It is never an empty box.

## Motion

Values ease by exponential approach and settle in about 4.6 ÷ rate seconds.

| Rate | Settles | For |
| --- | --- | --- |
| focus in 46 / leave 23 | ~100 / ~200 ms | Lift, shadow, sheen, white fill |
| overlay 15 | ~300 ms | Player controls, track panels, sheets |
| fade 23 | ~200 ms | Page transitions, toasts |
| scroll 18 | ~260 ms | Shelves, lists, episode rail |
| page 14 | ~320 ms | Push and pop, the title page lifting into episodes |
| backdrop 13 | ~350 ms | Crossfade, once the new picture (and its frost) is ready |

*Reduce motion* removes zooms, slides and the Home slideshow. Nothing animates while the viewer is idle; the app redraws only while something moves.

## Remote rules

- Arrows move focus. OK acts. Back always goes one step out and has exactly one owner.
- Back order:
  - **Player:** track panel → seek preview → hide the controls → exit.
  - **Rows:** back to the hero → the tabs → Home → *Press Back again to close Bloob*.
  - **Direct start:** Cancel or Back while starting returns to where Play was pressed.
- Up from the top of a screen reaches the tabs.
- In the player, Up or Down reaches the controls, which sit above the timeline. Down from the controls returns to the timeline.
- Play, Pause, Fast-forward, Rewind and Stop act at once in the player.

## Player

- **Quiet during viewing.** Any key shows the smoked bar 56 px above the bottom:
  - title, then meta with stream badges and the *Paused* or *Seeking* state;
  - the controls on the right: Play/Pause, audio (*English 5.1*), subtitles, *Start over*, *Next episode*;
  - the timeline below, with elapsed and remaining time.
- A smoked pill top right shows the time and *Ends 21:46*.
- **Timers:** the bar hides after 5 s, or 8 s when paused or when focus is on a control. Paused does not pin the bar. Fades take 300 ms.
- **Seeking** from a clear picture shows the timeline alone. The target appears in a white bubble above the knob and is committed 400 ms after the last press, never into the last second.
- **Track panels** stand on the bar at the right.
- **Up next** is a smoked card at the bottom right with a 10 s countdown line.
- **Startup** shows the soft backdrop, the title and a delayed spinner. Errors show a smoked sheet with *Try again*, *Choose another version* and *Exit*.

## States

- **Loading:**
  - Skeleton shelves.
  - The title page keeps the logo and meta, with a spinner where the buttons go.
  - How to watch shows *Finding versions…*.
  - Spinners appear only after 400 ms.
- **Errors:** one plain sentence and a way forward. A failed version shows the other versions.
- **Return:** every screen stays alive under the next, so Back restores focus, scroll and backdrop without refetching.
- **Playback interrupted:** the LG quick menu leaves playback alone. A released pipeline shows *Reconnecting* and reopens at the same position, at most three times a minute.

## Spoken interface

When LG Audio Guidance is on, the focused control is spoken as label, role and position, for example "Play, button, 1 of 2" or "Popular films. Sintel, 3 of 24".

- Speech waits 150 ms for focus to settle, and new speech interrupts stale speech.
- The running clock is never read out, and the Home slideshow pauses.
- The setting comes from settingsservice `getSystemSettings` (option `audioGuidance`), and speech goes through `com.webos.service.tts/speak`. The app declares `accessibility.supportsAudioGuidance`.
- This has not yet been heard on the TV; see `TV-TEST.md` §8.

## Screens

| Screen | Render | Layout |
| --- | --- | --- |
| **Home** | `01`–`05`, `20` | Full-bleed hero with adaptive scrims. Context line, logo or Display title, Callout meta, a 2-line Body description, glass buttons at y 690, and page dots. Shelves of 320×180 cards below. The hero follows shelf focus after the dwell. |
| **Title page** | `06`, `30`, `30a` | Full-bleed artwork, logo, meta with an IMDb badge, a 3-line description, and buttons: *Play* or *Resume*, *Start over*, *All versions*. Films get the frosted **How to watch** panel at y 790: the best version ticked as a candidate, two alternatives, and a status line. Down enters the panel; OK plays that version. |
| **Seasons and episodes** | `07`, `08`, `19` | Down lifts the page. Glass season buttons (the selected one is a candidate), episode cards with sheen, number, title and summary, and an *About* strip with cast. |
| **Starting** | `09` | Only the frosted progress sheet over the dimmed backdrop: steps, the AllDebrid download percentage, and Cancel. |
| **All versions** | `10`, `11`, `11a` | Left: logo, *All versions*, the episode, AllDebrid status. Right: the frosted best-version card (white when focused) and rows of other versions with availability words. Right opens the solid *Release details* drawer. |
| **Player** | `13`–`18` | See above. |
| **Search** | `22`, `23` | A keyboard on solid glass on the left, results in 3 columns beside it, 5 columns once the keyboard steps aside. Recent searches are glass buttons. |
| **Settings** | `32` | Glass rows with switches: AllDebrid, languages, autoplay, *Reduce motion*, *Reduce transparency*, *Increase contrast*, About, Diagnostics. |
| **Pairing** | `31` | A frosted sheet over the screen that opened it: the address, the PIN in white, and the renewal countdown. |

## What the reference informed, and what it did not

The Elements of A UI studies (`research/`) informed the *organisation* of Bloob:

- the artwork-led title page;
- a backdrop that follows focus after a dwell;
- seasons continuing the title page;
- a player that hides until summoned;
- a seek that previews before it commits;
- semantic label roles, effects tiers and adaptive contrast as principles.

Every asset, string and line of code here is original. Bloob uses Inter and its own icons. **Values are a different matter:** measured values from the studies are copied (see Sources above and `TOKENS.md`). Before 2.2, Glass used its own spacing even where a reference value existed. That was a misreading of the rule, now being corrected.
