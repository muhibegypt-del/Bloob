# Bloob tokens: the Elements of A value ledger

**Owner's rule (15 October 2026):** wherever the Elements of A studies record a measured design value, Bloob copies it. That covers spacing, padding, sizes, radii, type, opacity, colour, timing and geometry. Code, artwork, icons, fonts, written text and branding are never copied. In `src/theme.h`, a value marked `[EoA Xn]` comes from row Xn below. Every other value in `theme.h` has no recorded reference value, and is Bloob's own until one is found.

**Sources:**
- **`research/Elements-of-A-design-tokens.md` ("Lnnnn" = line), the full extracted catalogue from `bloob-v1` `a505b7b`.** It supersedes the summaries below wherever they differ. Rows added in 2.3.0 cite it by line.
- `research/Elements-of-A-implemented-design-system.md` ("DS"), cited by section.
- `research/Elements-of-A-UX-learning-frameworks.md` ("UXL"), cited by section.
- `research/question-playback-findings.md` ("PF").

**Status values:**
- **Copied:** in Bloob, with the reference value.
- **Same:** Bloob already had this value.
- **Behaviour:** a timing or rule that belongs to a UX-PLAN contract, not to `theme.h`.
- **N/A:** Bloob has no such component yet.

**2.3.0 applied the full catalogue; see the last section.** **Getting the rest.** The studies are not a full token export; DS §15 says so. Button padding, most card sizes, corner radii, easing curves and font weights are unrecorded. `scripts/eoa-token-export.py` collects them. It is read-only and outputs values only, no code. Run it on the machine that holds the extracted files, then add the rows here.

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
| C5 | Background `#2C2C2E` | `plain`, `solid` | **Owner override (2.6):** `plain` is OLED black `#000000`; `solid` keeps `#2C2C2E` |
| C6 | Black blend 0.55 if luminosity > 0.5, else 0.2 (L927). **Correction (2.3):** this is the hero background rule, not a card rule; the 0.5 threshold is recorded. | Title-on-art cards (`images.cpp`), as a borrowed rule; the hero itself now uses H16 | Copied, provenance corrected |
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

## 2.3.0: values from the full catalogue (`research/Elements-of-A-design-tokens.md`)

| ID | Value | Line | Bloob | Status |
| --- | --- | --- | --- | --- |
| T12 | Emphasized roles Bold, Standard roles Medium; "Regular" family Semibold | L170–L257, L365 | Title1–3 and Headline Bold; Callout, Body, Captions Medium; Button Semibold | **Copied** |
| T13 | Shelf header Emphasized.Callout (31 Bold) / LabelSecondary | L955 | `Text::CalloutBold`, shelves and season label | **Copied** |
| H1 | Hero logo box max 420 × 126; small 283 × 85 | L768–L769 | Home hero and title page | **Copied** (was 760 × 200) |
| H2 | Title → metadata 13; metadata → body 18; body → buttons 32 | L786–L790 | Home hero and title page | **Copied** |
| H3 | Hero metadata Standard.Caption1 / LabelPrimary | L815 | Home, title page | **Copied** |
| H4 | Description Standard.Caption1 / LabelSecondary; 570 / 3 lines (hero), 667 / 4 (title page) | L794–L798, L818 | Home, title page | **Copied** |
| H9 | Hero headline Emphasized.Headline / LabelSecondary | L811–L814 | "Popular film", "Up next" line | **Copied** |
| H10 | Hero artwork crossfade 300 ms | L931 | `backdrop_rate` | **Copied** |
| H11 | First shelf at 742 | L926 | Home | **Copied** (was 790) |
| H12 | Shelf header block 72 (text 20 down), → items 14, bottom 55; pitch 321 | L939–L965 | Home `ROW_H` 321 | **Copied** (was 330, header 58) |
| H15 | Hero bottom gradient stops 0 @64% … 0.68 @100% | L980 | `draw_scrims` | **Copied**. Stretched to solid at the bottom edge because Bloob's art is full-bleed |
| H16 | Hero overlay Black 0.55 at luminosity ≥ 0.9, else 0.2 | L928 | `draw_hero_overlay` | **Copied** |
| B1 | Buttons Regular 66 high (Small 50, Large 84), radius h/2 | L442 | `button_h` | **Copied** (was 80) |
| B2 | Padding 30 / 30 at every size | L449 | `button_pad` | **Copied** (was 48) |
| B3 | Icon-to-label 12 (Small 8); hero play icon 26 | L442, L840 | `button_icon_gap`, 26 | **Copied** |
| B4 | Focus: grow 8 wide, no scale | L448, L456 | `button_grow` | **Copied** (was ×1.06) |
| B5 | Focused fill White90, label black | L460 | `button_focus` | **Copied** |
| B6 | Button focus 75 / 150 ms | L447 | `button_in_rate`, title page | **Copied** |
| B7 | Between title-page and hero buttons 24 | L1523 | `title_button_gap` | **Copied** (was 16) |
| B8 | Settings and modal stacks 14 | L1718 | `stack_gap` | **Copied** |
| B9 | Hero and title buttons min 220, max 420 | L836–L839 | `title_button_min_w`, `title_button_max_w` | **Copied** |
| B10 | Dark variant: Black50 idle with a White16 outline, candidate White20 | L461 | `ButtonStyle::Dark` (artwork) | **Copied** (was White14) |
| B11 | Default variant: White10, candidate White60 | L460 | `ButtonStyle::Default` (sheets, search) | **Copied** |
| B12 | DarkPlayer variant: RGB(30,30,30) 90%, candidate Gray49 80% | L462 | `ButtonStyle::Player` | **Copied** |
| B14 | Shadow when focused or candidate | L455 | `draw_glass_button` | **Copied** |
| E1 | Shared shadow at elevation 1: opacity 0.4, blur 25, y 20 | L427–L430 | `shadow_*`: buttons, rows, keys | **Copied** |
| K1 | Card radius 20 (grid D) | L407 | `card_radius` | **Copied** (was 14) |
| K2 | Card focus grows 50 wide (not ×1.08) | L416 | `card_grow` | **Copied** |
| K3 | Episode card 410 × 231 (4-column item, inferred) | L406 | `episode_w`, `episode_h` | **Copied** (was 400 × 225) |
| K4 | Card shadow idle dy 4, blur 12, Black18; focused dy 40, blur 45, Black30 | L5208–L5215 | `draw_card` | **Copied** |
| K5 | Lockup text padding 14 (grid D) | L3808 | title on art | **Copied** |
| K6 | Progress inset 18 / 12 | L5225–L5226 | card progress | **Copied** |
| K7 | Progress LabelTertiary under LabelPrimary (inferred) | L5023–L5024 | `progress_back` | **Copied** |
| K8 | Under-card text: separation 10 when focused, after 14 of growth | L5203, L5263 | Home 24, episodes 4 / 24 | **Copied** |
| K9 | 16:9 lockup with description: eyebrow Caption3, title Caption1, description Caption2 × 5, 2 apart (inferred episode card) | L5243–L5254 | episodes | **Copied** |
| C10–C12 | SystemGreen 50,215,75; SystemRed 255,69,58; Orange #FFA500 (no warning role is recorded) | L161–L162, L119 | `good`, `problem`, `caution` | **Copied** |
| C13 | Outline on art, logos and buttons White16 | L461, L782 | `frost_hairline` | **Copied** (was 15%) |
| C14 | Artwork placeholder 29,29,29 | L718 | Home skeleton | **Copied** |
| M7 | Easing curves: linear p, ease-in p², ease-out p(2−p), ease-in-out | L503–L506 | `Anim` timed mode | **Copied** (was one exponential curve) |
| M8 | Shared focus arrival ease-in, departure ease-out | L433 | `focus_in_rate`, `focus_leave_rate` | **Copied** |
| M9 | Scroll springs: damping 30, stiffness 150, mass 1 | L975, L1019 | `scroll_rate`, `page_rate` | **Copied** |
| M10 | Page crossfade 300 ms linear, no movement | L4679 | screen transitions | **Copied** (was 200 ms with a 28 px rise) |
| M11 | Artwork fade-in 240 ms | L4754 | `image_fade_rate` | Defined |
| P6 | Pill focus 100 / 200 ms | L1268–L1269 | season strip | **Copied**, now wired |
| P7 | Pill label Emphasized.Callout, Bold | L1273 | `Text::Pill` | **Copied** |
| N1 | Notification: 200–600 wide, min height 112, radius 30, SystemGray5, hairline, shadow (0,15) blur 36 α0.15, padding 30 / 40, Caption1, hold 3000 | L6354–L6388 | toast, top right | **Copied** |
| N2 | Segment platter: padding 9, buttons 8 apart | L1641–L1643 | tab bar | **Copied**. The surface stays frosted glass (Bloob's Glass) |
| R1–R5 | List buttons: 120 multiline, radius 33, padding 30, White90 focused, Regular.Body title and Regular.Caption1 subtitle 5 apart | L476–L486 | Settings rows | **Copied** |
| R6 | Settings title Emphasized.Title2 / LabelSecondary | L1699–L1701 | Settings | **Copied** (left-aligned, not centred) |
| S1–S6 | Keyboard 432 wide; keys 54–55 × 64, pitch 74, rows 76; focused White platter radius 20 growing 6; idle no platter, secondary; keys Standard.Title3; field 85; input to content 112; query after 200 ms | L1010, L2054–L2139, L1556, L1027 | Search | **Copied** |
| V13 | Player controls DarkPlayer 66 high, Emphasized.Body, 25 apart | L2678, L2686 | transport | **Copied** (was 60, Caption1, 14) |
| V14 | No panel; transport gradient Black0 → Black60 | L2585–L2586 | player | **Copied**. Glass's smoked bar is gone |
| V15 | Bar 14 high, radius 7, 110 above the bottom, right padding 100; time-label baseline 32 below | L2587–L2603 | player | **Copied** |
| V16 | Header Emphasized.Title2 Gray92; subheader Caption1 Gray92, 12 apart | L2665–L2669 | player | **Copied** |
| V17 | Playhead 20, large 26 | L2604–L2605 | player | **Copied** |
| V18 | Time labels Emphasized.Caption2, white focused, White50 not | L2620–L2621 | player | **Copied**. Tabular figures not available in Inter as drawn |
| V19 | Track colours A 244 α0.56, C 236 α0.26 | L2593–L2595 | played and remaining | **Copied**; the mapping is Bloob's |
| V20–V22 | Floating menu 454 wide, radius 54, RGB(30,30,30) 90%, shadow Black30 r30 (0,10), header Caption2 White50 59 high, padding 21, rows 66 radius 33 4 apart, focused Gray92, row title Caption1, check 20, 20 above the controls | L548–L593 | track menu | **Copied** (was 620 wide, white rows) |
| V23 | Post-play lockup 454 × 460, title Emphasized.Title2, buttons 66 | L2914–L2916, L610 | Up next | **Copied** |
| V24 | Autoplay background Black50 (fades in over 250 ms) | L5555–L5556 | Up next | **Copied** |

**Still not recorded:** player clock, seek time bubble, seek-commit debounce (Bloob keeps 400 from the earlier study), toggle switch, card progress bar height, hero page dots and auto-advance, the Home tab bar. The reference uses a sidebar.

**Kept as Bloob's on purpose:**
- Frosted glass on the tab bar, How to watch and sheets. The owner chose Glass.
- The left text scrim on full-bleed art, which stands in for the reference's radial gradient.
- The settings list layout.
