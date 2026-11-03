# Elements of A — Player design-token findings

## Scope and evidence

Static extraction from the retained decoded `PlayerPage.js` in the effective OTA package, build `16.4.4.J34L139`. SHA-256: `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955`. Values below are observed declarations and call-site constants, not measurements of a live TV and not recommendations for Bloob.

Locations are **zero-based UTF-8 byte offsets in the retained decoded file**, not line numbers in a beautified copy. The minified variable name is a source identifier; names such as `player.transport` below are descriptive aliases, not claims that the reference exports a public token API.

### Units and unresolved references

- **S(n)** means the source's `yA(n)` screen-scaling helper. Preserve it as a scaled design value; do not treat it as an invariant physical pixel value. Plain numeric values and `S(...)` are kept distinct.
- Duration values are milliseconds unless explicitly stated otherwise.
- Semantic colour and typography references are preserved by their source names. Resolve them against the shared foundation inventory, rather than inventing RGB/font values here.
- Runtime-dependent dimensions, platform constraints and inherited style factories remain expressions.
- Vector icon path control points are artwork, not design tokens; icon frame dimensions are included. No complete extracted component source is reproduced.
- Playback buffer/codec settings and analytics constants are outside this design-token scope.

## 1. Transport bar foundation

Source: `PlayerPage.js`, module `34257`, exported style object `M` (local `k`), declaration at byte **300591**; consumed by the transport widget and thumbnail drawing methods.

| Token path | Observed value |
|---|---|
| `transport.containerHeight` | S(144) |
| `transport.containerHeightAudio` | S(200) |
| `transport.gradientStartColor` | `Black0` |
| `transport.gradientEndColor` | `Black60` |
| `transport.transportBarHeight` | S(14) |
| `transport.padding.left` | 0 |
| `transport.padding.right` | S(100) |
| `transport.padding.top` | S(90) |
| `transport.padding.bottom` | S(110) |
| `transport.topTimer.padding.top` | S(12) |
| `transport.colors.A` | RGB(244,244,244), alpha 0.56; source channels `244/255` |
| `transport.colors.B` | RGB(235,235,235), alpha 0.40; source channels `235/255` |
| `transport.colors.C` | RGB(236,236,236), alpha 0.26; source channels `236/255` |
| `transport.colors.CReducedTransparency` | RGB(128,128,128), alpha 1 |
| `transport.colors.playHeadColor` | `White` |
| `transport.cornerRadius` | S(7) |
| `transport.gapLiveBar` | S(2) |
| `transport.spinner.width` | S(24) |
| `transport.spinner.height` | S(24) |
| `transport.spinner.spacing` | S(14) |
| `transport.spinner.yOffset` | S(-2) |
| `transport.playHead.size` | S(20) |
| `transport.playHead.largeSize` | S(26) |
| `transport.playHead.color` | `White` |
| `transport.icons.fastForward` | Symbol width and height = `Standard.Body.size` |
| `transport.icons.rewind` | Symbol width and height = `Standard.Body.size` |
| `transport.icons.pause` | Symbol width and height = `Standard.Body.size` |
| `transport.icons.skip10Forward` | Symbol frame 30 × 32; Arabic/Hindi-specific artwork selected when custom numerals are enabled |
| `transport.icons.skip10Backwards` | Symbol frame 30 × 32; same numeral branching |
| `transport.iconsVerticalOffset.default` | S(4) |
| `transport.iconsVerticalOffset.skip` | S(1) |
| `transport.positionTextToImageSpacing` | S(14) |
| `transport.speedIndicatorSpacing` | S(10) |
| `transport.inset5` | S(5) |
| `transport.transportBarToTextBaseline` | S(32) |
| `transport.topBarPositionText.focused` | `Emphasized.Caption2`, `White` |
| `transport.topBarPositionText.unfocused` | `Emphasized.Caption2`, `White50` |
| `transport.currentPositionText.focused` | `Tabular.Emphasized.Caption2`, `White` |
| `transport.currentPositionText.unfocused` | `Tabular.Emphasized.Caption2`, `White50` |
| `transport.spacingBetweenLiveBadgeTransportBar` | S(24) |
| `transport.rectangularPlayheadWidth.focused` | S(2) |
| `transport.rectangularPlayheadWidth.unfocused` | S(1) |
| `transport.rectangularPlayheadHeight.focused` | S(16) |
| `transport.rectangularPlayheadHeight.unfocused` | S(14) |
| `transport.liveBadge.playhead.width` | S(2) |
| `transport.liveBadge.playhead.height` | S(16) |
| `transport.thumbnail.maxWidth` | S(400) |
| `transport.thumbnail.maxHeight` | S(225) |
| `transport.thumbnail.cornerRadius` | S(25) |
| `transport.thumbnail.borderWidth` | S(1) |
| `transport.thumbnail.borderColor` | RGBA(1,1,1,0.1), normalized channels |
| `transport.thumbnail.verticalOffset` | S(24) |
| `transport.thumbnail.playhead.width` | S(2) |
| `transport.thumbnail.playhead.height` | S(59) |
| `transport.thumbnail.playhead.color` | `White` |

### Derived transport geometry

Source: `drawToThumbnailDc` at byte approximately **271560**; long-live coordinate conversion methods follow it.

| Rule | Observed expression / bound |
|---|---|
| Thumbnail aspect handling | Uniform fit inside `thumbnail.maxWidth × thumbnail.maxHeight`; resulting width/height floored. The 400 × 225 box does not force every thumbnail to that aspect ratio. |
| Thumbnail horizontal position | Centre around preview position, clamped to the bar width. |
| Thumbnail indicator position | Independently clamp its width within the bar; its height is affected by corner/rectangular playhead geometry. |
| Long-live bar usable span | 0.95 of bar width, minus the rolling offset where applicable. |
| Long-live rolling leading offset | 0.02 of bar width. |
| Short-live wrap interval | 1,800 seconds. This is a live timeline presentation parameter, not VOD seek increment. |
| VOD timeline | Position / duration × bar width; absent duration produces position 0. |

## 2. Unified player layout and button references

Source: `PlayerPage.js`, module `54629`, memoized exported style factory `f` (local `S`), byte **342021**; `UnifiedPlayerWidget` at byte approximately **304500** consumes it.

| Token path | Observed value |
|---|---|
| `player.closeTab.paddingLeft` | S(24) |
| `player.closeTab.offsetX` | −S(20) |
| `player.transportBarToControlsSpacer` | S(22) |
| `player.transportBarToControlsSpacerTouch` | S(15) |
| `player.width` | S(900) |
| `player.widthPortrait` | S(640) |
| `player.headerFormat` | `Emphasized.Title2`, `Gray92` |
| `player.subheaderFormat` | `Standard.Caption1`, `Gray92` |
| `player.titleContainerAccessibilityPadding` | S(12) |
| `player.headerSubheaderPaddingLeft` | S(5) |
| `player.headerSubheaderPaddingBottom` | S(4) |
| `player.headerSubheaderSpacing` | S(12) |
| `player.transportBarHeight` | S(48); containing control row, distinct from S(14) drawn bar |
| `player.topContainer.paddingTop` | S(5) |
| `player.bottomContainer.offsetX` | 0 |
| `player.bottomContainer.offsetToTransportBar` | S(17) |
| `player.tabButton` | Shared `Rw.Regular.DarkPlayer()` from module `41457` |
| `player.tabButtonHovered` | Inherit DarkPlayer; override enabled Idle background to `Gray49` with alpha 0.4 |
| `player.tabsBackgroundWithReducedTransparency` | RGB(115,115,116), alpha 1 |
| `player.controlButton` | Shared `pz.RegularEmphasized.DarkPlayer()` from module `41457` |
| `player.tabsDeck.height` | S(302) |
| `player.tabsDeck.heightPortrait` | S(500) |
| `player.tabsDeck.offsetTabs` | S(-24) |
| `player.tabsDeck.transitionDistance` | S(100) |
| `player.spacingBetweenTab` | S(24) |
| `player.spacingBetweenControlsTouch` | S(50) |
| `player.spacingBetweenControlsTouchPortrait` | S(32) |
| `player.spacingBetweenControls` | S(25) |
| `player.paddingBottom` | S(50) |
| `player.controlsFloatingMenu.offset` | S(20) |
| `player.controlsFloatingMenu.style` | Inherit module `46261.g()`; override screen bounds |
| `player.controlsFloatingMenu.screenBounds` | Rect(safe horizontal, safe vertical, safe width + S(12), safe height) |
| `player.playerTabFloatingMenu.offset` | S(20) |
| `player.playerTabFloatingMenu.style` | Inherit module `46261.g()`; replace background with inherited `backgroundColorSolid` |
| `player.liveBadge.marginRight` | S(15) |
| `player.liveBadge.liveEdge.padding` | Horizontal S(11), vertical S(1) |
| `player.liveBadge.liveEdge.text` | `Rounded.Caption2`, `White` |
| `player.liveBadge.liveEdge.height` | S(30) |
| `player.liveBadge.liveEdge.background` | Normalized colour array `[1,80/255,0.4]`; three-channel form, not an explicit alpha declaration |
| `player.liveBadge.away.padding` | Horizontal S(11), vertical S(1) |
| `player.liveBadge.away.text` | `Rounded.Caption2`, `Black50` |
| `player.liveBadge.away.height` | S(30) |
| `player.liveBadge.away.background` | `White60` |

### Player control icon frames

Source: module `54629`, icon factory `h` (local `T`), immediately after the style factory.

| Token | Value |
|---|---|
| Subtitle/audio icon frame | 29 × 29 through `symbolWithSize` |
| Generated transport-control SVG view box | 64 × 64 |
| Generated transport-control SVG display size | S(64) × S(64) |
| Plus icon | Shared symbol using `Emphasized.Body` |
| Play, pause, forward, rewind, skip-ten, close, checkmark, jump-live artwork | Distinct paths within the common icon framing; path coordinates are intentionally not copied as tokens. |

## 3. Player timing and transitions

These are contextual call-site tokens, not one universal animation-duration scale.

| Token / context | Observed value | Source method / byte |
|---|---|---|
| Main controls reveal | Linear opacity to 1, 300 ms | `UnifiedPlayerWidget.show`, 331630 |
| Main controls hide | Linear opacity to 0, 300 ms; forced instant path multiplies duration by 0 | `UnifiedPlayerWidget.hide`, 330423 |
| Reduce other controls while scrubbing | Linear opacity, 250 ms | `updatePlayerUiWhileScrubbing`, 328907 |
| Open tab deck | 1 ms scheduling delay then `easeIn`, 300 ms | Tab focus callback, 313805 |
| Close/reset tab deck | `easeIn`, 300 ms | `UnifiedPlayerWidget.reset`, 329737 |
| Automatic/player auto-dismiss | 5,000 ms | `restartTimer`, 331740–332400 |
| Paused, tab item, control item, tip or opened control menu | 8,000 ms | Same |
| Opened tab/deck | 20,000 ms | Same |
| Screen reader / virtual-view auto-dismiss multiplier | ×2 when the call requests extended time | Same |
| Auto-dismiss suspension | Dragging, full-screen trickplay and thumbnail scrubbing suppress the timer; rechecked when timeout fires | Same |
| Seek feedback icon visibility | 450 ms | `UnifiedPlayerThumbnailController` constructor, 228650 region |
| Thumbnail hide delay | 3,000 ms | Transport thumbnail reaction, 264076 region |
| Thumbnail reveal | `easeOut`, 200 ms | Thumbnail animation reaction, 264802 |
| Thumbnail hide | `easeIn`, 200 ms; clears previous thumbnail on completion | Same, 264845 |
| Live badge reveal/hide | `easeOut` / `easeIn`, 200 ms | Transport live badge reaction, 262091–262275 |
| Left/right transport feedback icon reveal/hide | `easeInOut`, 140 ms | Transport icon reactions, 256104–256520 |
| Animated thumbnail preview delta | `easeInOut`, 240 ms | `UnifiedPlayerThumbnailController`, 236253 / 236298 |
| Transport inline spinner | Shared ExtraSmall preset, hold time 0, fade-in duration 0 | `TransportBar.showSpinner`, 265369–265405 |
| Skip banner reveal/hide | Linear, 400 ms | `PlayerPage` skip-widget animations, 179241, 179967, 189263–189358 |
| Rating-advisory display slot | 4,500 ms | Player disclaimer sequencing, 203000 region |
| First advisory initial delay | 4,000 ms | Same sequencing |
| Subsequent advisory initial delay | 2,000 ms | Same sequencing |
| Advisory line staggering | 200 ms × line index, or 0 when its animations are disabled | `PlayerDisclaimerOverlayWidget.start`, 14090 region |
| Photo-sensitivity/product-placement advisory hold | Session metadata duration × 1,000 | Player disclaimer sequencing, 203000 region |

`Performance.otherAnimationsEnabled` can force the main fade instant; tab replacement motion is conditional on performance support and reduced motion. These gates mean the presence of a duration does not prove that every device executes that animation.

## 4. Standalone LIVE pill

This is a separate component from the compact live badge in the unified-player style above.

Source: `PlayerPage.js`, module `17663`, `s.View` / `s.Draw`, style locals `g`, `y`, `m`, byte **226577**.

| Token | Observed value |
|---|---|
| Horizontal padding | S(14) |
| Height | S(42) |
| Corner radius | S(17) |
| Border width | S(2) |
| Text base | `Standard.Caption1` |
| Text override | Size 23, Semibold |
| Shadow blur | S(12) |
| At-live text | `LabelPrimary()` |
| At-live background | `SystemPink()` |
| At-live border | White with alpha 0.2 |
| At-live shadow | RGB(255,45,85), alpha 0.2 |
| Away-from-live text | `Black50` |
| Away background | Light-appearance `SystemGray2`, alpha 0.9 |
| Away border | `White20` |
| Away shadow | `White20` |

## 5. Player details panel and actions

Source: style locals `em`, `eb`, `ev` immediately after byte **15326**; consumer `PlayerDetailsWidget_PlayerDetailsWidget` at byte **68475**.

| Token | Observed value |
|---|---|
| General title format (`em`) | `Emphasized.Headline`, `LabelPrimary`, paragraph-spacing-before S(-6) |
| General subtitle format (`em`) | `Standard.Body`, `LabelSecondary`, paragraph-spacing-before S(-38) |
| Details title (`eb`) | `Emphasized.Callout`, `LabelPrimary()`, paragraph-spacing-before S(-6) |
| Details subtitle | `Standard.Caption1`, `LabelPrimary()` |
| Details subtitle spacing | S(12) |
| Badge-to-text spacing | S(8) |
| Description box horizontal padding | S(12) |
| Description box vertical padding | S(2) |
| Description focused background, dark | RGBA(1,1,1,0.4) |
| Description focused background, light | RGBA(1,1,1,0.4) |
| Description unfocused background | RGBA(1,1,1,0) |
| Collapsed body | `Standard.Body`, `White50` |
| Collapsed ellipsis / More | `Emphasized.Caption1`, `White` |
| Expanded body | `Standard.Body`, `LabelPrimary`, paragraph-spacing-after 0 |
| Translucent details background | RGB(30,30,30), alpha 0.9 |
| Solid details background | RGB(38,38,38), alpha 1 |
| Declared details background radius | S(24) |
| Actual panel draw radius in constructor | S(48); local override, not interchangeable with declared S(24) |
| Artwork height | S(202) |
| Artwork width | Default floor(16 × height / 9); source aspect ratio replaces this when known |
| Artwork radius / accessibility outline radius | S(25) |
| Outer vertical/general padding local | S(23) |
| Leading padding local | S(50) |
| Title-to-description spacer | S(5) |
| Description-to-badges spacer | S(13) |
| Description collapsed maximum | Height of 3 body lines |
| Action column spacing | S(14) |
| Action column minimum width | S(360) |
| Action column positional padding argument | S(18), in the third positional slot of the source `setPadding` call |
| Main horizontal spacing | S(40) |
| Accessibility description cursor expansion | Left/right S(20), top S(15), height addition S(20) |

Panel background switches to solid when transparent player controls are unsupported or reduced transparency is enabled. Bold-text policy selects alternate icon variants.

### Details action button states (`ev`)

| Token | Observed value |
|---|---|
| Height | S(76) |
| Corner radius | S(38) |
| Focus arrival | 100 ms |
| Focus departure | 200 ms |
| Horizontal expansion | S(8) |
| Enabled Idle background | `Black60` |
| Enabled Focused background | `White90` |
| Disabled Idle background | Dark appearance `White20`, otherwise `Black20` |
| Disabled Focused background | `White90` |
| Horizontal leading/trailing padding, all states | S(40) / S(40) |
| Leading icon spacing, enabled states | S(12) |
| Leading icon spacing, disabled states | S(20) |
| Leading opacity, all states | 1 |
| Enabled Idle text | `Emphasized.Callout`, `White`, page alignment 0.5 |
| Enabled Focused text | `Emphasized.Callout`, `Black`, page alignment 0.5 |
| Disabled Idle text | `Emphasized.Callout`, `White40`, page alignment 0.5 |
| Disabled Focused text | `Emphasized.Callout`, `Black`, page alignment 0.5 |
| Idle truncation | Fade, minimize enabled |
| Focused truncation | Rolling, minimize enabled |
| Shadow | Enabled for all declared states |
| Enabled Idle outline | Shared `borderColor(dark)` |

## 6. Skip / pre-roll promotional lockup

Source: local style `eV` at byte **18600**; `PlayerSkipWidget` at byte **20603**.

| Token | Observed value |
|---|---|
| Content vertical spacing | S(16) |
| Logo maximum height / width | S(114) / S(380) |
| Logo fallback | `Emphasized.Title2`, `LabelPrimary`, text alignment 0.5 |
| Metadata horizontal spacing | S(10) |
| Metadata bottom padding | S(12) |
| Metadata text | `Standard.Caption1`, `LabelPrimary`, text alignment 0.5 |
| Rating colour | `LabelPrimary` |
| Rating height | S(22) |
| Metadata maximum width | S(454) |
| Content-to-buttons spacing | S(30) |
| Button spacing | S(20) |
| Button style | Shared `JM.Regular.Default()` |
| Multi-button min/max width | S(217) / S(312) |
| Single-button min/max width | S(175) / S(350) |
| Logo image mode | ShrinkUniform, alignment [0.5,1] |
| Buttons/content alignment | Horizontal centre, 0.5 |

### Protection gradient

The declaration embeds a radial black protection gradient. The gradient is useful design data; the surrounding SVG markup is not reproduced.

| Property | Value |
|---|---|
| Raster declaration size | 240 × 135 |
| View box | 1920 × 1080 |
| Centre / focal point | 100%,100% / 100%,100% |
| Radius | 91.933% |
| Transform matrix | (0.5625,0,0,0.82,0.438,0.18) |
| Stop percentages | 0; 8.621; 16.56; 23.93; 30.845; 37.42; 43.767; 50; 56.233; 62.58; 69.155; 76.07; 83.44; 91.379; 100 |
| Corresponding opacity values | 0.6; 0.592; 0.569; 0.529; 0.482; 0.424; 0.365; 0.302; 0.235; 0.176; 0.118; 0.071; 0.031; 0.008; 0 |

## 7. Post-play and picture-in-picture

Source: local `ej` at byte **30570**, local style `e3` at byte **37068**, `e8` at byte **38863**; consumers `VideoWidget_VideoWidget`, `PostPlayLockup`, `PostPlayPage_PostPlayPage`.

| Token | Observed value |
|---|---|
| PIP corner radius | S(12) |
| PIP fade-out | 400 ms |
| PIP requested minimum video size | S(480) × S(270), adjusted by platform `getMinimumVideoSize` |
| PIP aspect-ratio adjustment | Height = width / supplied aspect ratio |
| PIP top | S(60) |
| PIP right position | Screen width − safe horizontal inset − actual PIP width |
| PIP transition duration | Reduced motion or unsupported video-size animation: 600 ms; otherwise 400 ms |
| PIP transition throttling interval argument | 100 ms when `throttleVideoSizeAnimation()` is true, otherwise 0 |
| Autoplay countdown default | 10,000 ms |
| Next-episode countdown default | 10,000 ms |
| Trailer countdown default | 5,000 ms |
| Explicit item autoplay delay | Payload delay in seconds × 1,000; overrides default path |
| Countdown extension | ×2 for the accessibility/virtual-view/gamepad condition used by `setAutoPlayEnabled` |
| Screen reader autoplay | AutoPlay widget disabled; duration extension alone must not be interpreted as permitting it |
| Slideshow auto-advance | 10,000 ms |
| Slideshow after-video advance | 2,000 ms |
| Slideshow fade-in / fade-out | 500 / 200 ms |
| Slideshow video start delay | 2,000 ms |
| Slideshow fullscreen delay | `undefined` in this override; inherited/default handling remains relevant |
| Slideshow video alignment | 0.5 |
| Slideshow chevrons offset | S(450) |
| Slideshow page-control offset | S(24) |
| Slideshow protection gradient height | S(616) |
| Slideshow protection gradient solid height | S(60) |
| Slideshow protection gradient midpoint opacity | 0.5 |
| Header text | `Emphasized.Body`, `LabelPrimary`, text alignment 0 |
| Header shadow | dx 0; dy S(2); `Black50`; blur S(5) |
| Header top-to-baseline | S(85) |
| Product logo frame | S(380) × S(114) |
| Channel logo frame | S(260) × S(114) |
| Featured logo frame | S(294) × S(88) |
| Logo fallback | `Emphasized.Title2`, `LabelPrimary`, alignment 0.5 |
| Logo fallback minimum size | `Emphasized.Title3.size` |
| Lockup width / height | S(454) × S(460) |
| Lockup button spacing | S(20) |
| Lockup button height | S(66) |
| Lockup pictogram height | S(26) |
| Subtitle area with featured logo | Rect(S(620), S(160), S(1220), S(700)) |
| Subtitle area without featured logo | Rect(S(620), S(160), S(1220), S(740)) |
| Binge body | Shared module `46827.qv.fonts.bodySmall`, 1 line |
| Upsell body | Shared `qv.fonts.bodyEmphasized`, 2 lines, width `qv.contentWidth` |
| Discovery body | Shared `qv.fonts.bodyRegular`, 3 lines |
| Discovery description | Shared `qv.fonts.description`, 1 line |
| Discovery metadata genre maximum | 1 |

The unusual 600 ms reduced-motion PIP branch is transcribed as implemented; it must not be flattened into a claim that reduced motion means all transitions become faster. The branch can use a different transition strategy.

## 8. InSight people and music cards

Source: locals `tU`, `tH`, `tN` at byte **79608**; associated InSight card consumers.

| Token | Observed value |
|---|---|
| InSight idle background | RGB(30,30,30), alpha 0.9 |
| InSight focused background | `Material.UltraLight` |
| Person card spacing | S(14) |
| Person card padding | S(20) |
| Person photo | S(116) × S(116) |
| Person photo shadow | offsetY S(8), blur S(12), `Black18` |
| Person photo outline | Shared `borderColor()`, width S(1) |
| Initials typography | `Rounded.Caption1` with size overridden to 58 |
| Initials colour / alignments | `LabelPrimary`, text 0.5, page 0.5 |
| Initials minimum size | Shared text format `minimumFontSize` |
| Initials background | Normalized RGB(0.5,0.5,0.5) |
| Initials outline | Shared `borderColor()`, width S(1) |
| Actor name | `Standard.Caption1`, `LabelPrimary()`; focused `LabelPrimaryReversed()` |
| Actor role | `Standard.Caption2`, `LabelSecondary()`; focused `LabelSecondaryReversed()` |
| Music card padding | S(24) |
| Music artwork | S(212) × S(212) |
| Music artwork radius | S(12) |
| Music artwork shadow | offsetY S(25), blur S(35), `Black18` |
| Music artwork background | `Gray85` |
| Song name | `Standard.Caption1`, `LabelPrimary`, alignment 0; focused `LabelPrimaryReversed()` |
| Artist name | `Standard.Caption2`, `LabelSecondary`, alignment 0; focused `LabelSecondaryReversed()` |
| Artwork-to-text spacing | S(24) |
| Explicit icon spacing | S(4) |
| Add/remove icon frame | S(40) × S(40) |
| Add/remove fade-in | 300 ms |
| Add/remove idle / focused background | `LabelSecondary()` / `LabelSecondaryReversed()` |
| Music icon height | S(17) |
| Music icon idle / focused colour | `LabelSecondary()` / `LabelSecondaryReversed()` |
| Shelf insert/remove scale | 0.5 + 0.5 × progress; insert/remove easeOut 500 ms |
| Shelf removal/reflow slide | easeOut 500 ms |
| InSight empty-state height | S(280) |
| InSight empty-state vertical text spacing | S(7) |
| InSight empty-state title | `Emphasized.Headline`, `LabelPrimary` |
| InSight empty-state body | `Standard.Body`, `LabelSecondary` |
| InSight empty-state alignment | (0,0.5), using the source alignment convention |
| InSight item body limit | 2 lines |
| InSight item icon radius | S(12) |

Insertion/removal animation call sites are `InSightShelf` at bytes approximately **92174–92902**. These card animations are separate from general selectable-widget defaults.

## 9. Catch Up to Live platter

Source: local `iv` at byte **139958**; consumer `CatchUpToLiveWidget`.

| Token | Observed value |
|---|---|
| Peeking distance | S(380) |
| Background blur | S(20) |
| Background fade-out / fade-in | 400 / 400 ms |
| Background fallback | `Black` |
| Foreground top/bottom padding | S(24) / S(24) |
| Current index text | `Emphasized.Caption2`, `LabelSecondary`, alignment 0 |
| Title width | S(300) |
| Title text | `Emphasized.Caption1`, `LabelPrimary`, alignment 0 |
| Jump-live button base | `JM.Regular.Default()` |
| Jump-live button departure | 2 × inherited default lost-focus duration |
| Jump-live idle override | Inherit base enabled Focused appearance into enabled Idle |
| Jump-live icon source frame | 64 × 64 |
| Platter width | S(460) |
| Platter background | `SystemGray5()` with alpha 0.94 |
| Platter radius | S(52) |
| Platter padding | S(28) |
| Platter fade-out / fade-in | 200 / 400 ms |
| Platter reload delay | 500 ms |
| Platter vertical/horizontal spacing | S(20) / S(20) |
| Team logo | S(92) × S(92) |
| Chevron/page-control fade | 200 ms |
| Title padding | S(2) |
| Background artwork decode request | Screen width/4 × screen height/4 |
| Auxiliary overlay reveal/hide | Linear opacity, 400 ms at bytes 151747 / 152988 |

## 10. Notifications and multi-view affordances

Source: `PlayerNotification` at byte **153156**, `ServerSideMultiViewOverlay` style `iV` at byte **166078**.

| Token | Observed value |
|---|---|
| Notification typography | `Standard.Caption1`, `LabelPrimary`, text alignment 1 |
| Notification text trailing padding | S(5) |
| Notification outer padding | Safe-area padding |
| Notification initial/exit scale | 0.8 |
| Notification visible scale | 1 |
| Notification initial/exit opacity | 0 |
| Notification visible opacity | 1 |
| Notification reveal | easeOut, 300 ms |
| Notification visible hold | 3,000 ms |
| Notification automatic exit | easeOut, 500 ms |
| Notification updated-content reveal | Cancel current animation; easeOut to visible in 10 ms; restart hold/exit |
| Notification explicit hide | easeOut, 300 ms |
| Notification animation gate | `Performance.notificationAnimationEnabled`; false forces instant animation |
| Multi-view overlay fade-in / fade-out | Linear, 250 / 250 ms |
| Multi-view overlay inactivity timeout | 10,000 ms |
| Multi-view outline Idle | `White15`, width S(1) |
| Multi-view outline Focused | `White`, width S(3) |
| Multi-view audio indicator size | S(`Regular.Caption3.size`) |
| Multi-view audio indicator padding | Trailing S(20), bottom S(20) |

## 11. Caption geometry and style references

Source: `PlayerPage` at byte **174055**, caption-style observer at **182022**, `_setSubtitleVisibleArea` at approximately **210500**, `UnifiedPlayerWidget.calculateSubtitlesBottomPadding` at **327737**.

| Token / rule | Observed value or expression |
|---|---|
| Caption renderer | Shared module `50836` supplied current/upcoming cue collections and selected caption style; not a separate literal font-size declaration in PlayerPage |
| Selected caption style fields observed | `uniqueId`, `font`, `size`, `textColor`, `backgroundColor`, `backgroundOpacity`, `textOpacity`, `edgeStyle`, `highlightColor`, `highlightOpacity` |
| Minimum bottom clearance | At least vertical safe area |
| Normal control obstruction | Main container height while visible or animating toward visible, otherwise 0 |
| Floating-menu clearance | Height below topmost menu + S(24), if a menu intersects the screen |
| Thumbnail-scrub clearance | Main container height − title-bar container height + `thumbnail.maxHeight` |
| Clearance upper clamp | Screen height − vertical safe area |
| Caption renderer origin | Safe horizontal inset, safe vertical inset |
| Caption renderer width | Safe-area width |
| Caption renderer height | Screen height − bottom clearance − vertical safe area |
| Caption style menu management icon | S(38) × S(38) |
| Caption style preview cue | Horizontal/default orientation, snap-to-lines false, line 10, line-align start, position auto, size 100, align centre, active indefinitely |

The preview cue's `size:100` is a WebVTT cue-region percentage, **not a font-size token**. Exact caption fonts, size mapping, edge colours and user presets belong to shared caption modules and settings; PlayerPage explicitly defers to those values. Menu size/child changes recompute caption clearance.

### Audio/subtitle menu local overrides

Source: `t9` / `ie` On/Off buttons at bytes **108807 / 109219**; track-menu builder `ir` at byte **112624**; inline label helper `ia` around **111480**; styles submenu at **122815–124483**.

| Token | Observed value |
|---|---|
| Subtitle On/Off menu title limit | 2 lines |
| Actual track-entry title limit | 2 lines |
| Styles parent menu title limit | 1 line |
| Manage Styles entry title limit | 2 lines |
| Audio menu local title limit | 2 lines at byte 128378 |
| Track label typography | `Standard.Caption1` |
| CC / AD inline badge aspect ratio | 38 / 22 |
| SDH inline badge aspect ratio | 42 / 22 |
| Inline track badge height | S(`Standard.Caption1.size`) |
| Inline track badge width | Rounded height × corresponding aspect ratio |
| Inline track badge leading padding | S(8) |
| Selected indicator | Shared checkmark symbol; selected accessibility state separately declared |
| Floating-menu base geometry/material | Shared module `46261.g()`, with the control/tab overrides documented in section 2 |

## 12. Other page-level presentation values

Source: local `ey` at **15326**, `PlayerPage` state and helper methods.

| Token | Observed value |
|---|---|
| Shelf/tab top padding | S(16) |
| Video attribute badge colour | `Gray85` |
| Page background | `Black` |
| Dolby Atmos icon height | S(22) |
| Empty player tab height | S(246) |
| Page protection gradient colour | `Black50` |
| Page protection gradient first/second stop | S(384) / S(968) |
| Player header maximum lines | Portrait 3; landscape 2 (`subscribeForData`, 334000 region) |
| Full-page spinner content height | Page height − 120 (unscaled literal), `PlayerPage.showSpinner` |

## Coverage and limitations

This document enumerates the player-specific style objects, their principal constructor overrides, explicit UI transition/timer constants, caption obstruction geometry and supporting post-play/InSight/live/multi-view components in the retained effective `PlayerPage.js`. Shared typography, material, global button and floating-menu definitions are referenced rather than duplicated; consult the shared-token extraction for their values.

The raw AST inventory should be consulted alongside these semantic tables for additional literal call sites and all source offsets. Geometry generated by server-supplied metadata, accessibility settings and native font metrics remains runtime-dependent. Static declaration presence establishes available code paths, not active feature flags, live rendered output or measured responsiveness.
