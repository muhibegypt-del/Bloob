# Bloob visual system 1.1

This is the design system for the native app. `src/theme.h` holds the tokens, and `src/ui.cpp` holds the type scale and shared components. If you change a value, change it in this document and in the code at the same time.

The renders mentioned below come from `scripts/ui-tour.sh`. That script drives the real app on the desktop with scripted remote presses, at 1920×1080. They are **desktop renders, not TV photographs**. The TV pass is in `TV-TEST.md`.

## Principles

1. **Artwork leads.** Every title screen sits on that title's own backdrop. A screen that needs to be read blurs and dims the backdrop instead of replacing it. The title logo appears in place of type wherever the catalogue has one.
2. **Quiet by default.** The ground is near-black. Text uses three alpha steps of one colour. Amber is the only accent, and it marks just two things: progress and "best match".
3. **Focus is unmistakable and immediate.**
   - Exactly one thing is white or lifted at a time.
   - Focus answers within one frame and settles in about 140 ms.
   - A backdrop follows focus only after a 250 ms dwell, so holding a direction key does not flicker artwork.
4. **One hierarchy everywhere.** The order is title, then essential context (year, genres, runtime, episode), then technical detail on request (release name, codec, seeders). Technical detail is always one key away and never on the first read.
5. **Films and shows share one page.** Details is a single page. Seasons peek at its foot, and pressing Down lifts the page into episodes. The title shrinks but stays in view.
6. **Every state is designed.** Loading, missing art, errors, empty results, the return to a screen, and the reconnect after the TV interrupts playback all have layouts. None of them shows a blank screen or a raw error string.

## Colour

| Token | Value | Use |
| --- | --- | --- |
| `ground` | `#0b0c0e` | Page ground, behind and under all artwork |
| `raised` | `#17181c` | Rows, keys, settings cells |
| `raised2` | `#202227` | Base for missing art |
| `primary` | `#f5f5f7` | Titles and focused labels |
| `secondary` | `#f5f5f7` at 73% | Meta lines, descriptions |
| `tertiary` | `#f5f5f7` at 50% | Labels, hints, eyebrows. Never below 19 px. |
| `hairline` | white at 10% | Dividers |
| `glass` | `#1c1e23` at 96% | Sheets and drawers over artwork. Nearly opaque, so artwork never ghosts through text. |
| `glass_light` | white at 15% | Unfocused buttons and pills |
| `focus` / `ink` | `#ffffff` / `#0b0c0e` | Focused fill and the text on it |
| `accent` | `#f5b942` | Bloob amber: progress bars, the "Best match" dot, eyebrows on Home |
| `good` | `#5cd58c` | Readily available, connected, watched |
| `caution` | `#f5b942` | Slow to prepare, very high bitrate. Always paired with a word. |
| `problem` | `#ff7a6b` | Failures. Always paired with a way forward. |

Colour never carries meaning alone. Every availability dot has words next to it.

## Type

Inter, in four weights. Glyphs are rasterised at their exact size, with 2× horizontal oversampling up to 48 px and a gamma lift so thin strokes stay solid on OLED. Kerning is cached.

| Style | Weight | Size / line | Use |
| --- | --- | --- | --- |
| Display | Bold | 76 / 84 | Title without a logo on Home and Details |
| Title1 | Bold | 48 / 56 | Screen titles, the best-match headline, the pairing PIN sheet |
| Title2 | SemiBold | 34 / 42 | Player title, panel headings |
| Headline | SemiBold | 28 / 36 | Shelf headers, row labels, settings rows |
| Body | Regular | 26 / 38 | Descriptions, at most 2–3 lines |
| Callout | Medium | 24 / 32 | Meta lines, episode titles, option rows |
| Caption | Medium | 23 / 30 | Episode summaries, secondary notes |
| Footnote | Medium | 22 / 28 | Episode numbers, badges, small print |
| Eyebrow | SemiBold | 22 / 28, capitals | Section labels such as BEST MATCH, OTHER VERSIONS, ABOUT |
| Button | SemiBold | 26 / 32 | Buttons and pills |

Text never goes below 22 px, the size TV 10-foot guidance treats as the floor for reading from a sofa at about 2.5–3 m on a 55″ panel. Long text is ellipsised or wrapped to a fixed number of lines. It never overlaps.

## Space and layout

- The grid is 8 px: `s1`–`s7` = 8, 16, 24, 32, 48, 64, 96.
- The title-safe margin is 96 px left and right (5%), with 54 px at the top. Nothing important sits outside it. Artwork bleeds to the edges.
- Tabs are a single centred pill at y 40, shown on Home's hero and on Search, Films, Series and Settings. They hide once the viewer moves into the rows.

## Shape, cards and focus

| Element | Rest | Focused |
| --- | --- | --- |
| Shelf card 384×216 (16:9), radius 14 | Artwork, soft shadow | Scales ×1.08 from its centre, deeper shadow, 4 px white ring with a 4 px gap. The label appears under it. |
| Episode card 400×225 | Artwork, number, title, 3-line summary | Lift and ring as above. The title turns `primary`. |
| Button, 72 h, radius 36 | `glass_light` fill, white label | White fill, ink label, ×1.05, shadow |
| Pill, 56 h | `glass_light` | White fill. The selected but unfocused season keeps a light fill. |
| Row (sources, settings) | `raised` / `glass_light` | White fill, ink text |
| Progress | 4 px track at white 25% | Amber fill. Watched episodes get a green check disc. |

Missing art becomes a tone derived from a hash of the title over `raised2`, with the title set in Headline. It is never an empty box. Artwork is decoded on worker threads at exactly the size it is drawn, with corners and title shading baked in, and it fades in over about 200 ms. Logos are decoded once at 760×240 (Contain) and then scaled. A box that animates never triggers a re-decode.

## Motion

Values are eased by an exponential approach. A value settles in about 4.6 ÷ rate seconds.

| Rate | Settles | For |
| --- | --- | --- |
| focus 33 | ~140 ms | Focus lift, ring, button fill |
| fade 23 | ~200 ms | HUD, sheets, drawers, toasts |
| scroll 18 | ~260 ms | Shelves, lists, episode rail |
| page 14 | ~320 ms | Push/pop between screens (fade + 24 px rise), Details lifting into episodes |
| backdrop 13 | ~350 ms | Crossfade, which starts only after the new picture is decoded |

Transitions are brief and purposeful. Nothing bounces, nothing slides in from off screen, and nothing animates while the viewer is idle. The app redraws only while something is moving.

## States

- **Loading**:
  - Home shows skeleton shelves with a soft pulse.
  - Details holds the logo and meta from the shelf item, so the page is never empty, and shows a spinner where the buttons will be.
  - Spinners appear only after 400 ms, so fast loads never flash one.
- **Missing art**: a tone and the title (see above). A backdrop that fails to load keeps the previous picture instead of going black.
- **Errors**: one plain sentence and a primary action. Home has *Try again*. Sources keep the list and say what happened. Player offers *Try again*, *Choose another version* and *Exit*.
- **Return**: every screen stays alive under the next, so Back restores focus, scroll and backdrop without refetching or animating them back in.
- **Playback interrupted**: the LG quick menu or volume does nothing to playback. If the TV releases the pipeline, the player shows *Reconnecting* over the title art. It reopens at the same position, at most three times a minute.

## Remote rules

- Arrows move focus. OK acts. Back always goes one step out and is owned by exactly one thing at a time.
- Back order:
  - **Player:** track panel → seek preview → hide the controls → exit.
  - **Rows:** back to the hero → the tabs → Home → *Press Back again to close Bloob*.
- Up from the top of any screen reaches the tabs. Down from the tabs enters the content.
- Holding a direction key scrolls continuously. Backdrops and details wait for the dwell.
- Play, Pause, Fast-forward, Rewind and Stop act at once in the player. They do not wait for the controls to be shown first, and the controls appear alongside.

## Screens

| Screen | Render | Layout |
| --- | --- | --- |
| **Home: hero** | `01-home`, `02-home-focus-moves`, `20-back-to-home` | Featured title on full artwork, with a left scrim and a bottom scrim. Eyebrow (amber) at y 206, logo box 760×220 at y 232, meta, 2-line description, buttons at y 636, page dots at y 752. Left/Right past the buttons pages the hero. It advances after 9 s idle. Continue-watching items lead. |
| **Home: shelves** | `03`, `04`, `05` | Focused shelf header at y 520, rows 350 px apart, cards 384×216 with 32 px gaps. The backdrop follows the focused card after the dwell. Rows above the focused one fade out. |
| **Details** | `06-details-series`, `30-details-film` | Logo or Display title, then meta with IMDb rating, 3-line description, and buttons (*Resume S1 · E2* / *Play* / *Episodes* / *Start over*). Films show cast and director. For a series, the season pills peek at y 790. |
| **Seasons and episodes** | `07`, `08` | Down lifts the page: the logo shrinks to 420×120, the backdrop blurs and dims, the season pills rise to y 320, and episode cards sit below with number, title and summary. An *About* strip (meta, rating, cast) sits at the foot. |
| **Sources** | `09`, `10`, `11` | The blurred title backdrop. Left column: logo, CHOOSE HOW TO WATCH, the episode, and the AllDebrid status. Right column: the **Best match** card with a plain-language headline ("4K · Dolby Vision · Atmos"), picture and sound, one reason, a caution if any, and a play disc. Below it, OTHER VERSIONS rows give resolution, picture and sound words, and an availability dot with words. Right opens **Release details** (picture, sound, file, release name, warnings). Hints sit at the foot. |
| **Starting** | `12`, `13` | Blurred backdrop, centred title and episode, a delayed spinner, and "Starting · 4K Dolby Vision". |
| **Player** | `14`–`18` | Quiet during viewing. Any key shows the controls; they hide after 5 s while playing. Bottom gradient. Title2 title and an episode line, with format badges on the right. Full-width timeline with time and time remaining. Seeking shows a bubble with the target and the delta. Controls row: play/pause, *Audio · English 5.1*, *Subtitles · Off*, *Start over*, *Next episode*. Track panels open on the right. *Up next* counts down 10 s at the end. |
| **Search** | `22`, `23` | Keyboard left (6 columns), results right in a 3-column grid, recent searches when empty. |
| **Settings** | `32-settings` | Rows of 120 px: AllDebrid, preferred audio language (Left/Right), and diagnostics. Below them are the version and TV model. |

## What the reference informed, and what it did not

The "Elements of A" UI study (`research/Elements-of-A-UI-responsiveness.md` and `research/Elements-of-A-UI-UX-code-study.md`, brought over from `bloob-v1`; user observation plus static evidence) informed the *organisation* of Bloob. Specifically:

- The artwork-led title page.
- Focus that drives the backdrop after a dwell.
- Seasons continuing the title page.
- A player that hides until it is summoned.
- A seek that previews first and commits after a debounce.

Every visual, asset, string and line of code here is original. Bloob uses Inter and its own icons and layouts.
