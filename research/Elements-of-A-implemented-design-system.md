# Elements of A — Implemented Design System

Version studied: saved OTA build `16.4.4.J34L139`.

Study date: 14 October 2026.

This document describes the design system recovered from shipped UI code. It combines verified tokens, component structure and interaction rules into a designer-readable reference. It is a study of an implementation, not an official design-guidelines document or a complete original source repository. The saved update's activation on the TV was not observed during this pass.

**Reading convention:** sizes are logical design units unless stated otherwise; timer values are milliseconds. A code constant is not a measured performance result. Exact source locations and evidence are listed at the end.

## 1. How the interface is built

The UI is shipped JavaScript. It creates a widget tree containing cards, shelves, labels, menus, pages, focus relationships and animation state. Luna's native C++ runtime draws that tree and supplies input/frame events.

Content records and artwork arrive as data. UI components determine their layout, hierarchy, styling and behavior.

| Responsibility | Principal bundle |
| --- | --- |
| Shared cards/lockups, shelves, typography, colors, root navigation and presentation policy | `app.js` |
| Title details, season selectors, episode lists, hero/fold composition | `ProductPage.js` |
| Browsing shelves and filter menus | `SuperBrowsePage.js` |
| Transport controls, thumbnails, track menus and post-play | `PlayerPage.js` |
| Keyboard, hints and search results | `SearchPageFactory.js` |
| Library and show-library screens | `LibraryPage.js` |

The system has both reusable visual components and reusable behavior. Focus restoration, artwork readiness, accessibility, loading states and transition ownership are part of the component contract.

## 2. Canvas, scaling and safe areas

The shared sizing helper normalizes against a **1920 × 1080** reference canvas.

Its ordinary scale factor is:

```text
scale = sqrt((viewportWidth × viewportHeight) / 2,073,600)
scaledValue = round(designValue × scale)
```

The helper has options for fractional output and explicit height-based scaling. The ordinary rule above applies to the shared canvas sizing path. The viewport comes from native display properties and is refreshed when display information changes.

| Shared token | Reference value |
| --- | ---: |
| Horizontal safe-area inset | 80 |
| Vertical safe-area inset | 60 |
| Safe-area width | viewport width − 2 × scaled horizontal inset |
| Safe-area height | viewport height − 2 × scaled vertical inset |
| Leading/trailing padding | scaled 80 |
| Top/bottom padding | scaled 60 |

At a 1920 × 1080 viewport, scale is 1. At 3840 × 2160, scale is 2. These examples describe the helper; the physical panel's pixels may differ from the app's rendering viewport.

The code uses component-specific spacing tokens. This study does not establish a universal spacing grid or one radius for every card.

## 3. Typography

The standard typography table defines named roles with separate size and line leading:

| Role | Size | Leading |
| --- | ---: | ---: |
| Title1 | 76 | 96 |
| Title2 | 57 | 66 |
| Title3 | 48 | 56 |
| Headline | 38 | 46 |
| Callout | 31 | 38 |
| Body | 29 | 36 |
| Caption1 | 25 | 32 |
| Caption2 | 23 | 30 |
| Caption3 | 21 | 28 |

Standard and Emphasized style families appear in component definitions. Season pills, for example, use an Emphasized Callout role. The complete family/weight mapping has not been reconstructed in this document.

The fit-to-string helper defaults to a minimum font size of **23**. Individual callers can supply another minimum. Caption3 at 21 is therefore the smallest role in this standard table, not a global minimum for every rendered text element.

Font loading applies screen scaling where enabled. Accessibility settings also supply larger text tiers. No sofa-distance test results or rationale for these values are present in the inspected artifacts.

## 4. Semantic colors and surfaces

Colors are exposed through semantic roles and light/dark theme maps. Components ask for roles such as primary or secondary label rather than treating every text element as an arbitrary color.

### Label hierarchy

| Role | Light theme | Dark theme |
| --- | --- | --- |
| Primary label | Black, 100% | White, 100% |
| Secondary label | Black, 60% | White, 50% |
| Tertiary label | Black, 40% | White, 30% |
| Quaternary label | Black, 20% | White, 20% |
| Accessory | Black, 20% | White, 50% |
| Disabled button copy | Black, 20% | White, 20% |
| Settings-button platter | White, 20% | White, 10% |

Percentages here are alpha values. Their visible result depends on the surface underneath.

### High-contrast label hierarchy

| Role | Light theme | Dark theme |
| --- | --- | --- |
| Primary | Black, 100% | White, 100% |
| Secondary | Black, 80% | White, 85% |
| Tertiary | Black, 60% | White, 75% |
| Quaternary | Black, 50% | White, 55% |

Ordinary hairlines use black at 10% in the light map and white at 8% in the dark map. Their high-contrast alternatives use 40% and 32% respectively.

The shared named `Background` constant is RGB **44, 44, 46** (`#2C2C2E`). It is a reusable color constant, not proof that every page paints that exact background. Splash, artwork and specialized component surfaces have their own treatments.

Reduce Transparency affects semantic label selection and backdrop treatment. Some translucent label roles are promoted to stronger roles, and product-page blur can become a solid treatment.

### Contrast over imagery

The implementation combines gradients, shadows, dark overlays, themed text and artwork-derived backgrounds. One product-card path checks image luminosity and chooses a stronger dark blend for light artwork: **0.55 versus 0.2**. A title-page fold coordinates background protection with hero opacity and shelf position.

These are contrast-protection rules. They do not supply a measured contrast ratio for every image or establish a formal audit process.

## 5. Cards, lockups and shelves

Cards are composed widgets. Artwork, text, badges, playable metadata and focus treatments can be combined according to the card's role.

The shared bundle includes specialized card/lockup families for landscape content, showcases, search results, channels, people, chart entries and other content types. It also includes carousel, flow, grid and generic shelf families. Those names establish reusable roles; they do not imply that every variant is active in every page.

The visual states include idle, focus candidate, focused and disabled treatments. A candidate state can show contextual readiness while another child owns the actual focus.

List position and selection are separate from card object identity. When content refreshes, compatible state is transferred and the previous widget ID is preferred. If that item disappears, selection falls back to a clamped index. Scroll can survive replacement.

### Long-row behavior

Lists use uniform/nonuniform size trees to find items by index or scroll position. Drawing traverses the needed viewport range, with extra work where required for focus/navigation. Child descriptions are realized into full widgets as needed.

This establishes bounded traversal and lazy realization. A universal fixed-size recycled-card pool and immediate disposal of every offscreen card are not established.

## 6. Season selectors and episode rails

Season selection and episode selection are coordinated. Cumulative episode counts connect an episode index to its season. Navigation can move between the season rail and the episode rail, preserving the relationship between them.

### Verified season-pill tokens

| Token | Value |
| --- | ---: |
| Height | 66 |
| Corner radius | 33 |
| Leading/trailing padding | 30 each |
| Horizontal focus expansion | 8 |
| Focus-gain animation | 100 ms |
| Focus-loss animation | 200 ms |

| State | Background | Label |
| --- | --- | --- |
| Enabled idle | No background specified in this state block | Secondary label, Emphasized Callout |
| Focus candidate | White at 10% | Primary label, Emphasized Callout |
| Focused | White | Black, Emphasized Callout |
| Disabled | Separate disabled style definitions | Disabled treatment; full token mapping not reproduced here |

These are season-selector tokens, not a universal pill specification for the entire app.

### Episode loading

The rail reserves placeholders so selection can move before every episode detail has arrived. Nearby missing data is requested after a **500 ms** delay, roughly ten entries either side of selection. More distant loaded detail can be replaced with placeholders beyond wider margins of twenty; returned chunk length also affects the retained window.

The resulting behavior preserves spatial continuity while limiting nearby data work. It does not require the whole season to finish loading before navigation works.

## 7. Browse filters and menus

The browse page contains its own filter bar, choice menus and profile-menu variants. Expanded and compact states interpolate dimensions and padding. Menu state owns key input, optional pointer input and its transition.

### Verified filter-bar and menu tokens

| Token | Value |
| --- | ---: |
| Filter-bar top padding | 30 |
| Filter-bar bottom padding | 26 |
| Space between filter buttons | 16 |
| Menu spacing | 24 |
| Expanded vertical offset | −21 |
| Spring stiffness | 25 |
| Spring damping | 185 |
| Spring mass | 1 |

The filter bar uses the shared horizontal safe-area inset and centers its horizontal content. The menu surface uses white at 10%; its panel-gradient color is RGB **67, 67, 67** (`#434343`). A component-local default icon color is RGB **255, 127, 0** (`#FF7F00`); this is not established as the app's global accent color.

Related content can load after a **300 ms** focus debounce. Version tokens reject outdated responses. A related-shelf controller defaults to a **35-shelf** insertion limit and uses **seven initial glimmer slots** before content reveal. The slots are loading-stage structure, not a universal seven-card pool.

## 8. Screen composition and hierarchy

### Home and browsing

The UI combines a hero/stage, navigation and content shelves. Automatic hero progression requires foreground/controller permission and an enabled presentation feature. Home additionally checks that it is topmost and scrolled to the top.

The slideshow suppresses automatic advance for Reduce Motion. Home also suppresses it while the screen reader is enabled.

### Title details

The title page has an above-fold and below-fold composition. A shared fold value changes hero/title opacity, shelf transforms, header/logo visibility and background protection together.

Above the fold, artwork and the principal action dominate. Moving toward episodes and supporting information shifts visual emphasis toward those shelves. This is one coordinated hierarchy transition.

Some entry paths preload content imagery before revealing the page. Product artwork loading has a **7,000 ms** timeout in the inspected path. That is a loading fallback threshold, not a desired wait time.

### Search

Search is a focusable workspace with input, keyboard, hints and results. Left/Right can move between keyboard and content. The layout shifts as browsing results becomes the active task.

Landing, results and empty states are separate deck states. The query/landing context can survive while obsolete results are disposed. Keyboard motion can become immediate when auto-scroll animations are disabled.

### Modals and overlays

An overlay has its own focus ownership and dismissal. Some transitions idle the page underneath. The underlying page can retain selection and scroll for the return path.

## 9. Focus, selection and Back

Focus is an authoritative state in the widget system, with a visual treatment and accessible description. It is not inferred solely from where a card happens to be drawn.

| Situation | Implemented rule |
| --- | --- |
| Data replaces compatible widgets | Transfer compatible state; prefer previous widget ID |
| Previous item is absent | Clamp/fall back through selection policy |
| Season/episode navigation | Route between coordinated rails |
| A modal or overlay is open | That layer owns focus and dismissal |
| Back from a nonroot page | Restore the previous page context |
| Back on a root screen | May return to top or expand navigation before exit |
| Exit-key confirmation | Cancel is the default selected action |

The app has both confirmation and direct-close policies. This document does not claim every Back press produces a prompt.

## 10. Artwork and loading transitions

The artwork helper takes the intended width/height, source aspect ratio, crop policy and platform sizing policy. It formats a resource URL for that result.

| Request policy | Observed value |
| --- | --- |
| Default format | JPG |
| Default quality for a 720-high UI | 90 |
| Default quality otherwise | 80 |
| Default-accuracy UHD 1440p/4K request dimensions | ¾ of requested dimensions |
| Default-accuracy 8K request dimensions | ⅓ of requested dimensions |
| Other default-accuracy screen types | Scale 1 |

Resource reduction preserves the intended widget geometry through image scale. Precise requests and format overrides exist for callers such as logos. Quality values are URL/encoding parameters, not percentages of saved bytes.

The background keeps its current image while the next one loads. Ready or Error permits the next transition, with a designed fallback on failure. Crossfade work is subscribed while pending/active and canceled after completion.

Glimmer placeholders are a capability-gated loading treatment. They reserve the content's spatial role before real artwork/cards are revealed. Loading, ready, empty and failure are represented through specific page/component states; a universal four-state base class is not established.

## 11. Motion and presentation policy

The presentation profile has four levels: LOW, MEDIUM, HIGH and ULTRA.

| Feature family | Gate |
| --- | --- |
| General/focus/auto-scroll/notification/floating-menu animations, hero advance, live player-position updates | Above LOW |
| Menu animations | Above MEDIUM |
| Page transitions, rich product cards, real-time blur, some background/portrait video treatments | ULTRA |
| Glimmer loading | HIGH or above |
| Immediate spinner | Below MEDIUM |

The inspected LG class inherits ULTRA. This is a static policy default, not evidence of a runtime benchmark or automatic degradation when the TV becomes busy.

Accessibility supplies additional rules:

- Focus animations can become immediate under Reduce Motion or disabled focus-animation policy.
- Automatic hero advance stops under Reduce Motion; Home also stops it for screen-reader use.
- Product blur falls back to solid treatment under Reduce Transparency or unsupported blur.
- Some scroll paths adjust their spring behavior under Reduce Motion rather than eliminating every movement.

Completed animations stop scheduling work. Stable screens can use the native renderer's dirty-tree idle path. Animation duration is distinct from remote-input latency.

## 12. Player design

The player page owns visual controls; a playback session supplies authoritative media state. Actual media time, requested seek time, drag position and thumbnail time are distinct values.

### UI modes

| Mode | Role |
| --- | --- |
| Full | Normal complete control presentation |
| Reduced | Less chrome during relevant interaction/trickplay |
| Deck | Tab/deck-oriented player interaction |

These named modes are implemented states. Their exact visible arrangement still depends on flags, available controls and content.

### Visibility and feedback tokens

| Rule | Value |
| --- | ---: |
| Ordinary/automatic hide delay | 5 seconds |
| Pause, focused control/tab item, tip or control menu | 8 seconds |
| Open tab/deck | 20 seconds |
| Screen reader or always-on virtual-view requirement | Double the applicable delay |
| Player show/hide fade | 300 ms, unless forced immediate |
| Other controls reduce during trickplay | 250 ms |
| Short seek feedback | 450 ms |
| Thumbnail hide delay | 3 seconds |

Dragging, full-screen trickplay and thumbnail scrubbing suppress automatic dismissal. The callback rechecks these states before hiding. The paused state selects an eight-second default; it is not an indefinitely visible-controls rule.

### Seek and preview

Relative seek steps can accumulate for **400 ms** before one backend seek commits. The requested position remains separate while the backend acknowledges it. Thumbnail images represent available preview-fragment times and may not exactly match the pointer's requested time.

Thumbnail generation is cancellable and bounded. The controller adjusts request rate under pressure. The HLS adapter supports previews; the inspected progressive adapter returns no thumbnail image.

### Audio and subtitle menus

Menus consume actual track collections and persistent identifiers. Supplied rendition names are localized; language/track name is the fallback. AD, CC and SDH have dedicated labeling treatments. Codec/channel badges are separate from track-row names.

A menu's capabilities depend on its media backend. The inspected progressive adapter has empty audio/subtitle switching methods. Displaying a menu alone does not establish functioning track switching.

### Subtitle-safe geometry

Subtitle placement responds to the current controls and menus. Ordinary padding uses the larger relevant obstruction plus a scaled **24-unit** gap, clamped to the safe area. Thumbnail scrubbing includes transport/thumbnail geometry. Menu size or child changes trigger recalculation.

This makes captions part of the player layout model rather than a permanently fixed overlay position.

## 13. Accessibility as component behavior

Custom-drawn widgets expose:

- Accessible label and container label.
- Role/type, enabled state and focusability.
- Position, index and count where relevant.
- Relationships to surrounding focus containers.

The app-owned screen reader responds to focus, content, visibility, page/input changes and announcements. It creates speech requests and calls native text-to-speech, with cancellation/queue rules and callbacks.

The enabled state comes from device accessibility properties. Search suggestions use a **1,500 ms** announcement delay in the inspected path.

An optional semantic AX-tree bridge exists, with a **500 ms** reevaluation debounce and identical-tree suppression. The inspected LG configuration does not enable that bridge; its supported static path appears to use direct TTS. No live screen-reader session was captured in this study.

## 14. Principles expressed by the implementation

The following are interpretations of observed mechanisms, not quotations from a formal guideline:

| Observed mechanism | Design principle it expresses |
| --- | --- |
| Named text/color roles | Give information a consistent visual hierarchy |
| Shared safe areas and scaled dimensions | Maintain a stable TV composition across viewports |
| Focus ID/index/scroll restoration | Preserve the user's place when content changes |
| Placeholder episode rails | Keep navigation available before every detail arrives |
| Artwork-ready transitions | Present a stable visual state while assets load |
| Coordinated title-page fold | Shift emphasis as the user's task changes |
| Activity-dependent player chrome | Keep viewing calm while supporting active decisions |
| Subtitle obstruction calculation | Let content and controls coexist without collisions |
| Accessibility semantics alongside drawing | Make the same focus model available through speech |
| Motion/capability gates | Preserve the task when effects are reduced |
| Bounded list work and finite animations | Keep UI work proportional to the active interaction |

## 15. What is not established by this document

The inspected files do not contain an explicit written design-principles or human-interface guide. This document reconstructs their implemented rules.

It is not a complete token export: every card dimension, corner radius, font family/weight, icon, palette entry, easing curve and component variant has not been enumerated. It does not invent one universal spacing grid, fixed card-recycling pool or minimum text size.

The study also does not establish user-testing outcomes, measured readability, contrast ratios, input latency, frame rate, the live device's selected flags or physical output state. Source offsets identify this saved build; they are not stable across future updates.

## 16. Evidence ledger

Offsets below are zero-based UTF-8 byte offsets in the original effective update files.

| Finding | Source location |
| --- | --- |
| Shared canvas scale | `app.js`, byte 1866034 |
| Shared safe-area calculation | `app.js`, byte 1866530 |
| Light semantic color map | `app.js`, byte 2129068 |
| Dark semantic color map | `app.js`, byte 2130173 |
| Named background constant | `app.js`, byte 2127922 |
| Standard typography / Caption3 anchor | `app.js`, factory 3038, byte 2196578 |
| Fit-to-string default minimum | `app.js`, byte 2195805 |
| Season-pill style | `ProductPage.js`, byte 17348 |
| Filter-bar spacing | `SuperBrowsePage.js`, byte 754 |
| Browse menu/spring tokens | `SuperBrowsePage.js`, byte 899 |
| Artwork helper | `app.js`, factory 62062, bytes 71966–78632 |
| Focus/state restoration | `app.js`, factory 44379, bytes 298054–300002 |
| List size trees and bounded traversal | `app.js`, factory 96184, bytes 2688398–2710391 |
| App-owned screen reader | `app.js`, factory 8824, bytes 1936395–1961632 |
| Player dismissal rules | `PlayerPage.js`, bytes 331740–332400 |
| Subtitle padding | `PlayerPage.js`, method near byte 327710 |

Fresh focused REA parent Evidence IDs:

- UI pages: `ev_cf4d94434d7ff45c629d8a8f1934f6a44262ba7d5e3b5f80fff1f53eb86d63ae`.
- Exact UI-core factories: `ev_6d2b67babca8676bb9ce5d712d864af7711d8d78150c340fd996dcf51ec54b95`.
- Player subset: `ev_da453b30e11a0ce4399b181bbb343e7e31ba6ea45b884a9fb01665acc7f062fb`.

Those analyses parsed the selected sources without execution. Their inventories are complete within the selected inputs; semantic coverage has unresolved facts and player analysis resource limits. The new token checks above used focused direct inspection of the same source bytes.

Source identities:

| File | SHA-256 |
| --- | --- |
| `app.js` | `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4` |
| `ProductPage.js` | `3cf224603413db5d9be5324efa7f9c11102086e800f6f8972155747e0f6d1870` |
| `SuperBrowsePage.js` | `77c182bc42387e5c4c4344e9253f60f4f8ff51ce8ca1586f34fb5ba64508cc25` |
| `PlayerPage.js` | `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955` |

Detailed studies: [UI evidence](question-ui-findings.md), [player evidence](question-playback-findings.md), [all 37 questions](Elements-of-A-engineering-questions.md), and [JetPack structure](jetpack-analysis-2026-10-14.md).
