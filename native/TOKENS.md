# Bloob tokens: the Elements of A value ledger

**Owner's rule (15 October 2026):** wherever the Elements of A studies record a measured design value, Bloob copies it. That covers spacing, padding, sizes, radii, type, opacity, colour, timing and geometry. Code, artwork, icons, fonts, written text and branding are never copied. In `src/theme.h`, a value marked `[EoA Xn]` comes from row Xn below. Every other value in `theme.h` has no recorded reference value, and is Bloob's own until one is found.

**Sources:**
- `research/Elements-of-A-implemented-design-system.md` ("DS"), cited by section.
- `research/Elements-of-A-UX-learning-frameworks.md` ("UXL"), cited by section.
- `research/question-playback-findings.md` ("PF").

**Status values:**
- **Copied:** in Bloob, with the reference value.
- **Same:** Bloob already had this value.
- **Behaviour:** a timing or rule that belongs to a UX-PLAN contract, not to `theme.h`.
- **N/A:** Bloob has no such component yet.

**Getting the rest.** The studies are not a full token export; DS §15 says so. Button padding, most card sizes, corner radii, easing curves and font weights are unrecorded. `scripts/eoa-token-export.py` collects them. It is read-only and outputs values only, no code. Run it on the machine that holds the extracted files, then add the rows here.

## Canvas and safe area (DS §2)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| S1 | Horizontal safe inset 80 | `safe_x` | Same |
| S2 | Vertical safe inset 60 | `safe_y` | Same |
| S3 | 1920 × 1080 reference canvas; `scale = sqrt(w·h / 2,073,600)` | Fixed 1920×1080 canvas, scaled to the window | Same in effect |

## Type (DS §3)

| ID | Role | Size / leading | Bloob | Status |
| --- | --- | --- | --- | --- |
| T1 | Title1 | 76 / 96 | `Text::Title1` | Same |
| T2 | Title2 | 57 / 66 | `Text::Title2` | Same |
| T3 | Title3 | 48 / 56 | `Text::Title3` | Same |
| T4 | Headline | 38 / 46 | `Text::Headline` | Same |
| T5 | Callout | 31 / 38 | `Text::Callout` | Same |
| T6 | Body | 29 / 36 | `Text::Body` | Same |
| T7 | Caption1 | 25 / 32 | `Text::Caption1` | Same |
| T8 | Caption2 | 23 / 30 | `Text::Caption2` | Same |
| T9 | Caption3 | 21 / 28 | `Text::Caption3` | **Copied** (2.2) |
| T10 | Fit-to-string minimum | 23 | Caption3 only where text is fitted | **Copied** |
| T11 | Season pill text | Emphasized Callout | `Text::Pill` (Callout, semibold) | **Copied** (2.2); exact weight unrecorded |

`Text::Display` (104 / 112, hero titles) and `Text::Button` (29 / 36) have no reference value.

## Colour and surfaces (DS §4, dark theme)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| C1 | Labels: primary 100%, secondary 50%, tertiary 30%, quaternary 20% white | `label()` | **Copied** (2.2; was 100/65/40/30) |
| C2 | Increase contrast: 100 / 85 / 75 / 55% | `label()` | Same |
| C3 | Hairline: white 8% | `hairline` | **Copied** (2.2) |
| C4 | Hairline with Increase contrast: 32% | `hairline_high`, on buttons | **Copied** (2.2) |
| C5 | Background `#2C2C2E` | `plain`, `solid` | **Copied** for `plain` (2.2; was `#0B0B0D`); `solid` already the same |
| C6 | Light artwork gets a stronger dark blend: 0.55, versus 0.2 | Title-on-art cards (`images.cpp`) | **Copied** (2.2; was a flat 0.7). The light/dark threshold is unrecorded. |
| C7 | Settings-button platter: white 10% | `settings_platter` | **Copied** (2.2; was 8%) |
| C8 | Accessory: white 50% | — | N/A |
| C9 | Disabled button copy: white 20% | `Label::Disabled` | **Copied** (via C1) |

## Season pills (DS §6)

These are used for Bloob's season strip and the tab bar. They have the same states as the filter buttons.

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| P1 | Height 66 | `pill_h` | **Copied** (2.2; was 60 for seasons, 64 for tabs) |
| P2 | Radius 33 | `pill_radius` | **Copied** |
| P3 | Padding 30 each side | `pill_pad` | **Copied** |
| P4 | Focused: grows 8 wider | `pill_focus_grow` | **Copied** (was a 1.06 scale) |
| P5 | At rest: no fill, secondary label. Candidate: white 10%, primary label. Focused: white fill, black label. | `GlassButton::pill`, `candidate` | **Copied** (candidate was 20%) |
| P6 | Focus gain 100 ms, loss 200 ms | `pill_in_rate`, `pill_out_rate` | Defined. The season strip and tabs still use the shared focus timing (M1, M2) until pills get their own animation. |

## Filter bar and menus (DS §7)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| F1 | Bar top padding 30 | `bar_pad_top` | Defined; the tab bar keeps an 8 inset inside its glass capsule, which has no reference |
| F2 | Bar bottom padding 26 | `bar_pad_bottom` | As F1 |
| F3 | Space between filter buttons 16 | `control_gap`, for tabs, button rows, seasons and sheet buttons | **Copied** (2.2; was 6 for tabs, 12 for seasons, 20 for buttons) |
| F4 | Menu spacing 24 | `menu_gap` | Defined; Bloob has no menus yet |
| F5 | Menu surface white 10% | `menu_surface` | Defined, N/A |
| F6 | Menu panel gradient `#434343` | `menu_gradient` | Defined, N/A |
| F7 | Spring stiffness 25, damping 185, mass 1 | — | N/A: Bloob eases exponentially, with no springs yet |
| F8 | Expanded vertical offset −21 | — | N/A |
| F9 | Related content after a 300 ms focus rest; outdated responses rejected | `dwell_ms` 300 | **Copied** (2.2; was 250) |
| F10 | 35-shelf limit; 7 placeholder slots | — | Behaviour, UX-PLAN X7 / X9 |

## Motion (UXL §3.2, DS §11)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| M1 | Focus arrival 85 ms (shared default) | `focus_in_rate` | **Copied** (2.2; was 100 ms) |
| M2 | Focus departure 230 ms | `focus_leave_rate` | **Copied** (2.2; was 200 ms) |
| M3 | Press and release 80 ms | `press_rate` | Defined; press feedback not built yet |
| M4 | Reduce Motion: focus animations immediate; hero advance stops (and stops under the screen reader on Home) | `reduce_motion`, slideshow | Same |
| M5 | Related content focus debounce 300 ms | `dwell_ms` | **Copied** |
| M6 | Effects tiers LOW / MEDIUM / HIGH / ULTRA; immediate spinner below MEDIUM | Bloob's Full / Lite | Behaviour |

## Player (DS §12, PF)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| V1 | Hide after 5 s | `hud_timeout_ms` | Same |
| V2 | 8 s when paused, on a control or in a menu | `hud_paused_timeout_ms` | Same |
| V3 | 20 s with a tab or deck open | — | N/A: no deck |
| V4 | Screen reader doubles every delay | `hud_reader_factor` | **Copied** (2.2) |
| V5 | Show and hide fade 300 ms | `overlay_rate` | Same |
| V6 | Other controls reduce in 250 ms during trickplay | — | Behaviour, X10 |
| V7 | Seek steps accumulate 400 ms before one commit | `seek_commit_ms` | Same |
| V8 | Short seek feedback 450 ms | — | Behaviour, X10 |
| V9 | Thumbnail hide 3 s | — | N/A: no thumbnails |
| V10 | Subtitles: largest obstruction plus a 24 gap, clamped to the safe area | `player.cpp` subtitle lift | Same (U7) |

## Loading and lists (DS §6, §8, §10)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| L1 | Episodes: fetch 500 ms after selection, about 10 either side | — | Behaviour, X7 |
| L2 | Release loaded detail beyond about 20 | — | Behaviour, X7 |
| L3 | Product artwork load timeout 7,000 ms | `product_art_timeout_ms` | Defined; not yet enforced |
| L4 | Artwork JPG, quality 80 (90 for 720-high UIs); UHD requests at ¾ size | — | N/A: Bloob scales art itself |

## Accessibility (DS §13)

| ID | Value | Bloob | Status |
| --- | --- | --- | --- |
| A1 | Search suggestions announced after 1,500 ms | — | Behaviour, X12 |
| A2 | Semantic tree re-evaluated with a 500 ms debounce | — | N/A |
| A3 | Exit confirmation: Cancel is selected by default | Bloob asks for Back twice instead | Behaviour, X5 (owner to decide) |

## Not yet recorded anywhere (run the export script)

- Primary action buttons: height, padding and focus scale.
- Card and lockup sizes, corner radii and focus scale.
- Shadow depth.
- Episode card size.
- Hero layout offsets.
- Player bar geometry.
- Font weights per role.
- Easing curves.
- Tab bar geometry.
