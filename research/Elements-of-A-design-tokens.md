# Elements of A — Extracted Design Tokens

**Saved webOS build:** 16.4.4.J34L139

**Extraction date:** 15 October 2026

**Evidence:** decoded shipped JavaScript, inspected as inert text; no TV or application execution.

This is a comprehensive catalogue of the recoverable design declarations in the saved reference app: foundations, shared components, page overrides, player controls, captions, and language-specific keyboard adjustments. It combines readable token matrices with a source-addressed declaration inventory. It expands the earlier implemented-design-system summary substantially.

**Scope of “all”:** all entries in the explicitly inventoried foundation tables and scoped declaration objects below. It is not a claim that every drawing constant in every native binary, every arbitrary inline layout calculation, or every server-generated visual value has been recovered. Computed rules, inherited values and unavailable runtime inputs remain identified rather than assigned invented constants. This is an extracted implementation catalogue, not an official design specification.

## Contents

- [A. Foundations and shared behaviour](#a-foundations-and-shared-behaviour)
- [B. Pages and component overrides](#b-pages-and-component-overrides)
- [C. Player and playback presentation](#c-player-and-playback-presentation)
- [D. Additional shared component declarations](#d-additional-shared-component-declarations)
- [E. Language-specific keyboard tokens](#e-language-specific-keyboard-tokens)
- [F. Evidence, extraction coverage and remaining boundaries](#f-evidence-extraction-coverage-and-remaining-boundaries)

## How to read the values

| Notation | Meaning |
| --- | --- |
| Reference units | Design-space dimensions before the app's screen-scaling helper; not invariant physical display pixels. |
| S(n), scaled(n), scale(n) | Presentation notation for verified imports of factory 46491's yA scaling helper. Source aliases are retained where unresolved. |
| Time | Milliseconds unless the row explicitly says seconds. A configured duration is not measured latency or frame rate. |
| RGB / alpha | Normalized constructor channels, 0–255 helper channels, and opacity are distinguished in their sections. Alpha is on 0–1. |
| Semantic colour | A role resolved through theme/accessibility, not a single universal hex value. |
| Reference / (spread) | Resolve or inherit the referenced style first, then apply the local override. |
| Conditional / computed | The rule or original source locator is retained; final output needs runtime input. |
| Original names | Factory IDs and minified variables identify the shipped source. Human-readable grouping names are editorial labels. |
| Source byte | Zero-based UTF-8 byte offset in the original decoded named bundle. Object, property and value offsets are labelled separately. |

### Important interpretation rules

1. Shared defaults do not replace component overrides. A season pill, standard button and player button can use different focus timings.
2. Use the scaling formula and semantic colour roles together with the numbers. Do not flatten the app into one spacing step, radius, focus scale or background colour.
3. Declared variants are not proof that they were active on the TV. This includes accessibility tiers, alternate page variants and capability-dependent animation paths.
4. Artwork colours, material rendering, text shaping and viewport geometry have external inputs. Their dependency rules are part of the catalogue; their final pixels are not constants.
5. Icon frame sizes, scrim gradients and caption edge rules are included. Complete vector paths, image payloads, font binaries, codec/buffer settings, network/analytics constants and literal catalogue content are outside design-token scope.

### Extraction at a glance

| Area | Recorded coverage |
| --- | --- |
| Shared colour foundations | 59 raw colour/material declarations; 31 paired light/dark semantic roles |
| Typography | 84 named entries; 90 declared size/leading/tracking-tier records; font and accessibility resolution rules |
| Core geometry and behaviour | Canvas/safe area, grid/radii, focus/pressed states, shadows, button variants, menu and motion defaults |
| Named page/component declarations | 67 scoped objects, 975 leaf values/expressions, plus curated inline/conditional values and 29 legacy WebVTT renderer rows |
| Additional app component declarations | 130 groups, 2216 property rows; 21 long/computed/asset dependencies retain source locators |
| Player | Transport, thumbnails, track menus, overlays, timing, post-play, PIP, InSight, live and multiview presentation |
| Broad discovery pass | 139 JavaScript files parsed; 16196 heuristic property candidates used for discovery, not presented as confirmed design tokens |

Counts describe entries/rows in their stated scope. Aliases, repeated styles and overrides mean they must not be added up as a count of unique scalar tokens.

## A. Foundations and shared behaviour

Target: saved build **16.4.4.J34L139**, inspected 15 October 2026. This is a static token inventory from shipped JavaScript, not an official design specification. No application or native API was executed. Dimensions are unscaled reference units unless noted; durations are milliseconds. Source coordinates are zero-based UTF-8 bytes in `app.js`, SHA-256 `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4`.

### A.1 Raw colors and materials

Source: factory **44548**, `Color` static definitions. Values retain their declared normalized-channel constructor or 0–255 RGB/hex helper expression; named alpha variants retain their explicit opacity. The table inventories all static color/material declarations, excluding the non-color parser regex.

| Token | Exact static value / dependency | Source byte |
| --- | --- | --- |
| `Color.Black` | `new ConstantColor(0,0,0)` | 2125442 |
| `Color.Black0` | `Color.constantColorWithAlpha(Color.Black,0)` | 2125479 |
| `Color.Black5` | `Color.constantColorWithAlpha(Color.Black,.05)` | 2125536 |
| `Color.Black10` | `Color.constantColorWithAlpha(Color.Black,.1)` | 2125595 |
| `Color.Black18` | `Color.constantColorWithAlpha(Color.Black,.18)` | 2125654 |
| `Color.Black20` | `Color.constantColorWithAlpha(Color.Black,.2)` | 2125714 |
| `Color.Black25` | `Color.constantColorWithAlpha(Color.Black,.25)` | 2125773 |
| `Color.Black30` | `Color.constantColorWithAlpha(Color.Black,.3)` | 2125833 |
| `Color.Black36` | `Color.constantColorWithAlpha(Color.Black,.36)` | 2125892 |
| `Color.Black40` | `Color.constantColorWithAlpha(Color.Black,.4)` | 2125952 |
| `Color.Black50` | `Color.constantColorWithAlpha(Color.Black,.5)` | 2126011 |
| `Color.Black60` | `Color.constantColorWithAlpha(Color.Black,.6)` | 2126070 |
| `Color.Black70` | `Color.constantColorWithAlpha(Color.Black,.7)` | 2126129 |
| `Color.Black80` | `Color.constantColorWithAlpha(Color.Black,.8)` | 2126188 |
| `Color.Black90` | `Color.constantColorWithAlpha(Color.Black,.9)` | 2126247 |
| `Color.dimmedWhite` | `Color.constantColorFromRGB(240,240,240,1)` | 2126306 |
| `Color.White` | `new ConstantColor(1,1,1)` | 2126366 |
| `Color.White0` | `Color.constantColorWithAlpha(Color.White,0)` | 2126403 |
| `Color.White5` | `Color.constantColorWithAlpha(Color.White,.05)` | 2126460 |
| `Color.White10` | `Color.constantColorWithAlpha(Color.White,.1)` | 2126519 |
| `Color.White15` | `Color.constantColorWithAlpha(Color.White,.15)` | 2126578 |
| `Color.White20` | `Color.constantColorWithAlpha(Color.White,.2)` | 2126638 |
| `Color.White30` | `Color.constantColorWithAlpha(Color.White,.3)` | 2126697 |
| `Color.White40` | `Color.constantColorWithAlpha(Color.White,.4)` | 2126756 |
| `Color.White50` | `Color.constantColorWithAlpha(Color.White,.5)` | 2126815 |
| `Color.White55` | `Color.constantColorWithAlpha(Color.White,.55)` | 2126874 |
| `Color.White60` | `Color.constantColorWithAlpha(Color.White,.6)` | 2126934 |
| `Color.White70` | `Color.constantColorWithAlpha(Color.White,.7)` | 2126993 |
| `Color.White75` | `Color.constantColorWithAlpha(Color.White,.75)` | 2127052 |
| `Color.White80` | `Color.constantColorWithAlpha(Color.White,.8)` | 2127112 |
| `Color.White85` | `Color.constantColorWithAlpha(Color.White,.85)` | 2127171 |
| `Color.White90` | `Color.constantColorWithAlpha(Color.White,.9)` | 2127231 |
| `Color.White95` | `Color.constantColorWithAlpha(Color.White,.95)` | 2127290 |
| `Color.AlertDestructive` | `new ConstantColor(247/255,38/255,44/255)` | 2127350 |
| `Color.AlertDestructive40` | `new ConstantColor(247/255,38/255,44/255,.4)` | 2127414 |
| `Color.GrayLight` | `ConstantColor.fromHexColor("#B4B4B4")` | 2127483 |
| `Color.Gray` | `ConstantColor.fromHexColor("#787878")` | 2127537 |
| `Color.GrayDark` | `ConstantColor.fromHexColor("#3C3C3C")` | 2127586 |
| `Color.Gray5` | `new ConstantColor(12/255,12/255,12/255,1)` | 2127639 |
| `Color.Gray30` | `new ConstantColor(76/255,76/255,76/255,1)` | 2127693 |
| `Color.Gray49` | `new ConstantColor(124/255,124/255,124/255,1)` | 2127748 |
| `Color.Gray92` | `new ConstantColor(235/255,235/255,235/255,1)` | 2127806 |
| `Color.Gray85` | `new ConstantColor(216/255,216/255,216/255,1)` | 2127864 |
| `Color.Background` | `new ConstantColor(44/255,44/255,46/255)` | 2127922 |
| `Color.BlueLight` | `new ConstantColor(.2,.58,.9,1)` | 2127979 |
| `Color.Red` | `new ConstantColor(1,0,0)` | 2128026 |
| `Color.Green` | `new ConstantColor(0,1,0)` | 2128061 |
| `Color.Blue` | `new ConstantColor(0,0,1)` | 2128098 |
| `Color.Cyan` | `new ConstantColor(0,1,1)` | 2128134 |
| `Color.Yellow` | `new ConstantColor(1,1,0)` | 2128170 |
| `Color.Magenta` | `new ConstantColor(1,0,1)` | 2128208 |
| `Color.Orange` | `ConstantColor.fromHexColor("#FFA500")` | 2128247 |
| `Color.FillPrimary` | `new ConstantColor(1,1,1,.2)` | 2128298 |
| `Color.FillSecondary` | `new ConstantColor(1,1,1,.16)` | 2128344 |
| `Color.FillTertiary` | `new ConstantColor(1,1,1,.12)` | 2128393 |
| `Color.FillQuaternary` | `new ConstantColor(1,1,1,.08)` | 2128441 |
| `Color.Material` | `{"UltraLight":"new ConstantColor(1,1,1,.9)","MediumLight":"new ConstantColor(235/255,235/255,235/255,.56)","SemiLight":"new ConstantColor(235/255,235/255,235/255,.4)","MediumDark":"new ConstantColor(38/255,38/255,38/255,.95)","MediumDarkOpaque":"new ConstantColor(38/255,38/255,38/255,1)","SemiDark":"Color.Black40"}` | 2128527 |
| `Color.AccessibilityCursor` | `{"innerBorder":"new ConstantColor(0,0,0)","outerBorder":"new ConstantColor(1,1,1)"}` | 2128835 |
| `Color.AccessibilityReduceTransparencyBackground` | light: #BFC4C7; dark: Color.Background | 2128937 |

`ConstantColor` constructor arguments are normalized RGB(A); absent alpha is 1. `constantColorFromRGB` accepts 0–255 RGB. `constantColorWithAlpha` replaces alpha, preserving channels. No color-space conversion beyond these numeric operations is established here.

### A.2 Semantic palette: light, dark and contrast roles

| Semantic role | Light mapping | Dark mapping |
| --- | --- | --- |
| `OpacityZero` | `Color.Black0` | `Color.White0` |
| `LabelPrimary` | `Color.Black` | `Color.White` |
| `LabelSecondary` | `Color.Black60` | `Color.White50` |
| `LabelTertiary` | `Color.Black40` | `Color.White30` |
| `LabelQuaternary` | `Color.Black20` | `Color.White20` |
| `HighContrastPrimary` | `Color.Black` | `Color.White` |
| `HighContrastSecondary` | `Color.Black80` | `Color.White85` |
| `HighContrastTertiary` | `Color.Black60` | `Color.White75` |
| `HighContrastQuaternary` | `Color.Black50` | `Color.White55` |
| `Semi` | `Color.Black40` | `Color.White40` |
| `Semi50` | `Color.Black20` | `Color.White20` |
| `Accessory` | `Color.Black20` | `Color.White50` |
| `SettingsButtonPlatter` | `Color.White20` | `Color.White10` |
| `ButtonDisabledCopy` | `Color.Black20` | `Color.White20` |
| `text100` | `Color.Black` | `Color.White` |
| `text70` | `Color.Black70` | `Color.White70` |
| `text60` | `Color.Black60` | `Color.White60` |
| `text40` | `Color.Black40` | `Color.White40` |
| `hairline` | `Color.colorFromRGB(0,0,0,.1)` | `Color.colorFromRGB(255,255,255,.08)` |
| `hairline_HighContrast` | `Color.colorFromRGB(0,0,0,.4)` | `Color.colorFromRGB(255,255,255,.32)` |
| `SystemGray` | `Color.colorFromRGB(142,142,147,1)` | `Color.colorFromRGB(142,142,147,1)` |
| `SystemGray2` | `Color.colorFromRGB(174,174,178,1)` | `Color.colorFromRGB(99,99,102,1)` |
| `SystemGray3` | `Color.colorFromRGB(199,199,204,1)` | `Color.colorFromRGB(72,72,74,1)` |
| `SystemGray4` | `Color.colorFromRGB(209,209,214,1)` | `Color.colorFromRGB(58,58,60,1)` |
| `SystemGray5` | `Color.colorFromRGB(229,229,234,1)` | `Color.colorFromRGB(44,44,46,1)` |
| `SystemGray5_HighContrast` | `Color.colorFromRGB(219,219,224,1)` | `Color.colorFromRGB(54,54,56,1)` |
| `SystemGray6` | `Color.colorFromRGB(242,242,247,1)` | `Color.colorFromRGB(28,28,30,1)` |
| `SystemGreen` | `Color.colorFromRGB(52,199,89,1)` | `Color.colorFromRGB(50,215,75,1)` |
| `SystemRed` | `Color.colorFromRGB(255,59,48,1)` | `Color.colorFromRGB(255,69,58,1)` |
| `SystemBlue` | `Color.colorFromRGB(0,122,255,1)` | `Color.colorFromRGB(10,132,255,1)` |
| `SystemPink` | `Color.colorFromRGB(255,45,85,1)` | `Color.colorFromRGB(255,55,95,1)` |

Source: factory **44548**, semantic map `l`, byte 2129065. Theme selector `s()` returns dark by default; explicit theme overrides and reversed-theme accessors exist. High-contrast roles are separate named roles rather than a third complete palette. Reduce-transparency accessors promote secondary→primary, tertiary→secondary and quaternary→tertiary; reversed variants do the same in the opposite theme. Reduce-transparency background: light #BFC4C7, dark `Background`.

### A.3 Typography: every named table entry

| Role / family | Size | Leading | Weight | Font | OpenType |
| --- | --- | --- | --- | --- | --- |
| `Standard.Title1` | 76 | 96 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Title2` | 57 | 66 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Title3` | 48 | 56 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Headline` | 38 | 46 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Subtitle1` | 38 | 46 | `a.I.Regular` | `SFHello (loader default)` | `none declared` |
| `Standard.Callout` | 31 | 38 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Body` | 29 | 36 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Caption1` | 25 | 32 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Caption2` | 23 | 30 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Standard.Caption3` | 21 | 28 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Regular.Title1` | 76 | 96 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Title2` | 57 | 66 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Title3` | 48 | 56 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Headline` | 38 | 46 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Subtitle1` | 38 | 46 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Regular.Callout` | 31 | 38 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Body` | 29 | 36 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Caption1` | 25 | 32 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Caption2` | 23 | 30 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Regular.Caption3` | 21 | 28 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Short.Title1` | 76 | 94 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Title2` | 57 | 64 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Title3` | 48 | 54 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Headline` | 38 | 44 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Subtitle1` | 38 | 44 | `a.I.Regular` | `SFHello (loader default)` | `none declared` |
| `Short.Callout` | 31 | 36 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Body` | 29 | 34 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Caption1` | 25 | 30 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Caption2` | 23 | 28 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Short.Caption3` | 23 | 26 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Title1` | 76 | 96 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Title2` | 57 | 66 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Title3` | 48 | 56 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Headline` | 38 | 46 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Subtitle1` | 38 | 46 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Callout` | 31 | 38 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Body` | 29 | 36 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Caption1` | 25 | 32 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Caption2` | 23 | 30 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Emphasized.Caption3` | 21 | 28 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Tall.Title1` | 76 | 98 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Title2` | 57 | 68 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Title3` | 48 | 58 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Headline` | 38 | 48 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Subtitle1` | 38 | 48 | `a.I.Regular` | `SFHello (loader default)` | `none declared` |
| `Tall.Callout` | 31 | 40 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Body` | 29 | 38 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Caption1` | 25 | 34 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Caption2` | 23 | 32 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `Tall.Caption3` | 21 | 30 | `a.I.Medium` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Title1` | 76 | 98 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Title2` | 57 | 68 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Title3` | 48 | 58 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Headline` | 38 | 48 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Subtitle1` | 38 | 48 | `a.I.Semibold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Callout` | 31 | 40 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Body` | 29 | 38 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Caption1` | 25 | 34 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Caption2` | 23 | 32 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `EmphasizedTall.Caption3` | 21 | 30 | `a.I.Bold` | `SFHello (loader default)` | `none declared` |
| `Rounded.Headline` | 38 | 46 | `a.I.Semibold` | `SFCompactRounded` | `none declared` |
| `Rounded.Caption1` | 25 | 32 | `a.I.Semibold` | `SFCompactRounded` | `none declared` |
| `Rounded.Caption2` | 23 | 30 | `a.I.Semibold` | `SFCompactRounded` | `none declared` |
| `Tabular.Standard.Title1` | 76 | 96 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Title2` | 57 | 66 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Title3` | 48 | 56 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Headline` | 38 | 46 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Subtitle1` | 38 | 46 | `a.I.Regular` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Callout` | 31 | 38 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Body` | 29 | 36 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Caption1` | 25 | 32 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Caption2` | 23 | 30 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Standard.Caption3` | 21 | 28 | `a.I.Medium` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Title1` | 76 | 96 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Title2` | 57 | 66 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Title3` | 48 | 56 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Headline` | 38 | 46 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Subtitle1` | 38 | 46 | `a.I.Semibold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Callout` | 31 | 38 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Body` | 29 | 36 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Caption1` | 25 | 32 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Caption2` | 23 | 30 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.Emphasized.Caption3` | 21 | 28 | `a.I.Bold` | `SFHello (loader default)` | `n.Gfx.TextFormat.OpenTypeFeature.TabularFigures` |
| `Tabular.MonospacedDebug` | 22 | 0 | `a.I.Regular` | `<luna-debug-font>` | `none declared` |

Source: factory **3038**, typography table `r`, byte 2196008. No tracking is declared in these role entries. `Regular` is a family name: most of its roles use Semibold, so it must not be interpreted as Regular weight. `Short.Caption3` is 23/26, whereas Standard Caption3 is 21/28. Rounded declares only three roles. Tabular contains Standard, Emphasized and MonospacedDebug; it is not a separate complete rounded family.

### A.4 Dynamic text and tracking tiers

Source: factory **38413**, table `a`; values below are exact declared scale, size, leading, tracking and emphasized-weight fields. They are independent declarations: do not derive all sizes by multiplying the base size by scale. Caption3 base leading here is 30, unlike the ordinary Standard table’s 28.

| Role | Tier | Scale | Size | Leading | Tracking | Emphasized weight |
| --- | --- | --- | --- | --- | --- | --- |
| Title1 | L | 1 | 76 | 96 | 0.07 | bold |
| Title1 | XL | 1.05 | 78 | 97 | 0.04 | bold |
| Title1 | XXL | 1.07 | 79 | 98 | 0.01 | bold |
| Title1 | XXXL | 1.08 | 80 | 99 | 0 | bold |
| Title1 | AX1 | 1.15 | 87 | 102 | 0 | bold |
| Title1 | AX2 | 1.2 | 91 | 108 | 0 | bold |
| Title1 | AX3 | 1.25 | 95 | 113 | 0 | bold |
| Title1 | AX4 | 1.3 | 99 | 118 | 0 | bold |
| Title1 | AX5 | 1.35 | 103 | 120 | 0 | bold |
| Title2 | L | 1 | 57 | 66 | 0.29 | bold |
| Title2 | XL | 1.08 | 62 | 74 | 0.24 | bold |
| Title2 | XXL | 1.1 | 63 | 76 | 0.23 | bold |
| Title2 | XXXL | 1.12 | 64 | 78 | 0.22 | bold |
| Title2 | AX1 | 1.2 | 68 | 84 | 0.17 | bold |
| Title2 | AX2 | 1.26 | 72 | 88 | 0.14 | bold |
| Title2 | AX3 | 1.32 | 75 | 94 | 0.1 | bold |
| Title2 | AX4 | 1.38 | 79 | 98 | 0.01 | bold |
| Title2 | AX5 | 1.44 | 82 | 100 | 0 | bold |
| Title3 | L | 1 | 48 | 56 | 0.35 | bold |
| Title3 | XL | 1.1 | 53 | 62 | 0.33 | bold |
| Title3 | XXL | 1.15 | 55 | 64 | 0.31 | bold |
| Title3 | XXXL | 1.2 | 58 | 67 | 0.28 | bold |
| Title3 | AX1 | 1.28 | 61 | 72 | 0.25 | bold |
| Title3 | AX2 | 1.36 | 65 | 76 | 0.2 | bold |
| Title3 | AX3 | 1.44 | 69 | 81 | 0.15 | bold |
| Title3 | AX4 | 1.5 | 72 | 82 | 0.14 | bold |
| Title3 | AX5 | 1.56 | 75 | 87 | 0.1 | bold |
| Headline | L | 1 | 38 | 46 | 0.37 | bold |
| Headline | XL | 1.1 | 42 | 51 | 0.37 | bold |
| Headline | XXL | 1.15 | 44 | 53 | 0.37 | bold |
| Headline | XXXL | 1.2 | 46 | 55 | 0.36 | bold |
| Headline | AX1 | 1.28 | 49 | 59 | 0.33 | bold |
| Headline | AX2 | 1.36 | 52 | 63 | 0.31 | bold |
| Headline | AX3 | 1.44 | 55 | 67 | 0.31 | bold |
| Headline | AX4 | 1.5 | 57 | 69 | 0.29 | bold |
| Headline | AX5 | 1.56 | 59 | 72 | 0.27 | bold |
| Subtitle1 | L | 1 | 38 | 46 | 0.37 | medium |
| Subtitle1 | XL | 1.1 | 42 | 51 | 0.37 | medium |
| Subtitle1 | XXL | 1.15 | 44 | 53 | 0.37 | medium |
| Subtitle1 | XXXL | 1.2 | 46 | 55 | 0.36 | medium |
| Subtitle1 | AX1 | 1.28 | 49 | 59 | 0.33 | medium |
| Subtitle1 | AX2 | 1.36 | 52 | 63 | 0.31 | medium |
| Subtitle1 | AX3 | 1.44 | 55 | 67 | 0.31 | medium |
| Subtitle1 | AX4 | 1.5 | 57 | 69 | 0.29 | medium |
| Subtitle1 | AX5 | 1.56 | 59 | 72 | 0.27 | medium |
| Callout | L | 1 | 31 | 38 | 0.39 | bold |
| Callout | XL | 1.1 | 34 | 42 | 0.4 | bold |
| Callout | XXL | 1.15 | 37 | 46 | 0.36 | bold |
| Callout | XXXL | 1.2 | 39 | 48 | 0.38 | bold |
| Callout | AX1 | 1.28 | 41 | 51 | 0.36 | bold |
| Callout | AX2 | 1.36 | 44 | 55 | 0.37 | bold |
| Callout | AX3 | 1.44 | 47 | 57 | 0.37 | bold |
| Callout | AX4 | 1.5 | 50 | 60 | 0.34 | bold |
| Callout | AX5 | 1.56 | 53 | 64 | 0.33 | bold |
| Body | L | 1 | 29 | 36 | 0.4 | bold |
| Body | XL | 1.12 | 32 | 40 | 0.41 | bold |
| Body | XXL | 1.2 | 35 | 43 | 0.38 | bold |
| Body | XXXL | 1.28 | 37 | 46 | 0.36 | bold |
| Body | AX1 | 1.4 | 39 | 48 | 0.38 | bold |
| Body | AX2 | 1.5 | 42 | 52 | 0.37 | bold |
| Body | AX3 | 1.6 | 45 | 56 | 0.35 | bold |
| Body | AX4 | 1.7 | 48 | 58 | 0.35 | bold |
| Body | AX5 | 1.8 | 51 | 62 | 0.35 | bold |
| Caption1 | L | 1 | 25 | 32 | 0.15 | bold |
| Caption1 | XL | 1.12 | 28 | 36 | 0.38 | bold |
| Caption1 | XXL | 1.2 | 30 | 38 | 0.4 | bold |
| Caption1 | XXXL | 1.28 | 32 | 41 | 0.41 | bold |
| Caption1 | AX1 | 1.4 | 35 | 45 | 0.38 | bold |
| Caption1 | AX2 | 1.5 | 38 | 48 | 0.37 | bold |
| Caption1 | AX3 | 1.6 | 40 | 51 | 0.37 | bold |
| Caption1 | AX4 | 1.7 | 43 | 55 | 0.35 | bold |
| Caption1 | AX5 | 1.8 | 45 | 56 | 0.35 | bold |
| Caption2 | L | 1 | 23 | 30 | -0.1 | bold |
| Caption2 | XL | 1.12 | 26 | 34 | 0.22 | bold |
| Caption2 | XXL | 1.2 | 28 | 36 | 0.38 | bold |
| Caption2 | XXXL | 1.28 | 29 | 38 | 0.4 | bold |
| Caption2 | AX1 | 1.4 | 32 | 42 | 0.41 | bold |
| Caption2 | AX2 | 1.5 | 35 | 45 | 0.38 | bold |
| Caption2 | AX3 | 1.6 | 37 | 48 | 0.36 | bold |
| Caption2 | AX4 | 1.7 | 39 | 51 | 0.36 | bold |
| Caption2 | AX5 | 1.8 | 41 | 54 | 0.36 | bold |
| Caption3 | L | 1 | 21 | 30 | -0.36 | bold |
| Caption3 | XL | 1.12 | 24 | 31 | 0.07 | bold |
| Caption3 | XXL | 1.2 | 25 | 32 | 0.15 | bold |
| Caption3 | XXXL | 1.28 | 27 | 36 | 0.29 | bold |
| Caption3 | AX1 | 1.4 | 29 | 39 | 0.4 | bold |
| Caption3 | AX2 | 1.5 | 32 | 42 | 0.41 | bold |
| Caption3 | AX3 | 1.6 | 34 | 45 | 0.4 | bold |
| Caption3 | AX4 | 1.7 | 36 | 48 | 0.37 | bold |
| Caption3 | AX5 | 1.8 | 38 | 50 | 0.37 | bold |

Dynamic lookup functions `o(role,tier)` and `s(role,tier)` expose the full record and a derived glyph-scale table respectively. The derived glyph scale caps tiers above AX3 at AX3 scale; the full text-size table remains as listed. **Declared tokens do not prove active dynamic resizing:** factory 29942's text-scale and glyph-scale helpers both return 1 in this shipped build, and its default/max resolver returns the default. The dynamic tables are recoverable declarations, not evidence that this LG build actively applies every accessibility tier.

### A.5 Font resolution and accessibility transformations

| Token / rule | Value | Source |
| --- | --- | --- |
| Default text format | Standard.Body | factory 16393, TextFormat constructor |
| Default font family / extension | SFHello / woff2 | factory 16393, TextFormat.createFontList |
| Rounded font | SFCompactRounded | factory 3038 Rounded; factory 16393 createFontList |
| Available weight enum | Regular, RegularItalic, Medium, Semibold, Bold, Heavy, Black | factory 55242 |
| Bold Text mapping for multi-weight fonts | Regular→Semibold; Medium→Bold; Semibold→Heavy; Bold/Heavy→Black; Black unchanged | factory 16393 createFontList |
| Rounded Bold Text special case | SFCompactRounded Semibold→Heavy | factory 16393 createFontList |
| Fallback for non-SF font | SFHello of selected weight, then locale/script font paths | factory 16393 createFontList |
| Final symbol fallbacks | HelveticaNeue.symbols.subset.woff; AppleSymbols.symbols.subset.woff; ZapfDingbatsITC.symbols.subset.woff | factory 16393 createFontList |
| Fit-to-string default floor | 23; individual callers may override | factory 16393 scaleFormatToFitString / minimumFontSize |
| Automatic screen scaling | Font lists use canvas scale only when shouldAutoScaleFontsToFitScreenSize is enabled | factory 16393 createFontList |
| Vibrancy gate | Disabled with reduce transparency or Bold Text | factory 16393 helper c / createTextFormat |

### A.6 Canvas, safe area and grid primitives

| Token / formula | Reference value | Source |
| --- | --- | --- |
| Reference area | 1920×1080 = 2,073,600 | factory 46491 d |
| Ordinary canvas scale | sqrt(viewport width × viewport height / 2,073,600) | factory 46491 d |
| Scaled dimension | round(reference dimension × scale), unless fractional output requested | factory 46491 u |
| Explicit height scaling | reference dimension × supplied height / 1080, rounded unless fractional output requested | factory 46491 u |
| Safe-area horizontal / vertical | 80 / 60, scaled | factory 46491 screen initializer n |
| Safe width / height | viewport width − 2×horizontal / viewport height − 2×vertical | factory 46491 n |
| Safe padding | leading/left/trailing/right 80; top/bottom 60, scaled | factory 46491 n |
| Resolution classes | 480, 720, 1080, 1440, 2160, 4320 | factory 46491 enum / l |
| Grid width for column count c>1 | floor((safe width − (c−1)×horizontal gap)/c) | factory 301 calculateGridWidth |
| One-column width | safe width | factory 301 calculateGridWidth |
| Shelf item height | round(width/aspect ratio) | factory 301 calculateShelfItemHeight |

| Radius name | Reference size |
| --- | --- |
| None | 0 |
| XS | 16 |
| S | 18 |
| M | 20 |
| L | 24 |
| XL | 28 |
| XXL | 32 |

Factory 301 `cornerRadiusForColumnCount`: 1→XXL, 2→XL, 3→L, 4→M, 5→S, 6→XS, otherwise None. Grid presets below have their own explicit choices; do not overwrite them with that helper.

| Grid preset | Columns | Horizontal gap | Vertical gap | Aspect | Radius |
| --- | --- | --- | --- | --- | --- |
| A | 2 | 40 | 109 | 16:9 | XL 28 |
| B | 3 | 40 | 109 | 16:9 | L 24 |
| C | 4 | 40 | 109 | 16:9 | M 20 |
| D | 5 | 40 | 109 | 16:9 | M 20 |
| E | 6 | 40 | 109 | 16:9 | S 18 |
| F | 8 | 40 | 109 | 16:9 | S 18 |
| L | 5 | 40 | 104 | 1:1 | L 24 |

### A.7 Shared focus, pressed and drawing defaults

| Token | Value | Source |
| --- | --- | --- |
| Zoom horizontal expansion | 50 scaled; vertical and anchors unspecified | factory 31210 selectable style supplier |
| Focus arrival / departure | 85 / 230 ms | factory 31210 |
| Press / release | 80 / 80 ms | factory 31210 |
| Disappear | 200 ms | factory 31210 |
| Pressed style | Acknowledge; alternatives AcknowledgeAndHide and Deflate | factory 31210 |
| Alternate selection acknowledgement | true by default; ButtonWidget overrides false | factory 31210; 95055 constructor |
| Accessibility rectangular cursor radius | 12 scaled default | factory 1718 DrawTools.accessibilityRectangleCursor |
| Accessibility inner border | 2×round(max(scale(4),4)/2), black; fixed border width | factory 1718 accessibilityRectangleCursor |
| Accessibility outer border | white; inner thickness, or half thickness when focusStyle=false | factory 1718 accessibilityRectangleCursor |
| Outline radius / width | 12 / 1 scaled defaults; width rounded | factory 1718 outlineRect |
| Outline default color | semantic borderColor (hairline); skipped for light theme when ordinary-theme restriction=true | factory 1718 outlineRect |
| Rounded shadow opacity | 0.3 + 0.1×elevation factor | factory 1718 roundedRectShadow |
| Rounded shadow blur | scale(12) + scale(13)×elevation factor | factory 1718 roundedRectShadow |
| Rounded shadow offset | x=0; y=scale(7)+scale(13)×elevation factor | factory 1718 roundedRectShadow |
| Shadow overall opacity | 1 default; supplied corner radius | factory 1718 roundedRectShadow |
| Button accessibility cursor border | 4 scaled | factory 95055 ButtonWidget.AccessibilityCursorBorderWidth |

A zoom expansion is additive width, not a universal percentage. Components override these defaults. `SelectableWidget.animateFocusChange` uses ease-in arrival and ease-out departure; the press path distinguishes linear deflation and ease-out acknowledgement. Zoom anchors default to 0.5/0.5. Reduced motion or disabled focus-animation capability forces focus/press animations to immediate values; automatic selection/reselection and uncover paths can also skip the normal transition.

### A.8 Standard button token matrix

Source: factory **41457**, style constructor `y(size,weight,variant,labelMode,focusCandidate)`. Export groups represent label/icon-only and ordinary/focus-candidate variants.

| Size | Height | Internal spacing | Standard typography | Emphasized typography | Base radius |
| --- | --- | --- | --- | --- | --- |
| Small | 50 | 8 | Regular.Caption1 | Emphasized.Caption1 | ceil(scaled height/2) |
| Regular | 66 | 12 | Regular.Body | Emphasized.Body | ceil(scaled height/2) |
| Large | 84 | 12 | Regular.Callout | Emphasized.Callout | ceil(scaled height/2) |

| Shared token | Value |
| --- | --- |
| Focus arrival/departure | 75/150 ms |
| Horizontal expansion | 8 scaled |
| Labelled leading/trailing padding | 30/30 scaled |
| Icon-only padding | not declared by this constructor |
| Gamepad hint width/height | 36/24 scaled |
| Gamepad hint top/leading | 21/12 scaled |
| Idle title truncation | Fade |
| Focused/candidate title truncation | Rolling |
| Focused/candidate shadow | enabled |
| Button radius during focus | base radius + focusZoom×horizontal expansion/2 unless fixedCornerRadius |

| Variant | Idle background | Candidate background | Focused background | Idle text | Candidate text | Focused text | Idle outline |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Default | White10 | White60 | White90 | LabelPrimary | LabelPrimaryReversed | LabelPrimaryReversed | none |
| Dark | Black50 | White20 | White90 | LabelPrimary | LabelPrimary | LabelPrimaryReversed | White α0.16 |
| DarkPlayer | RGB(30,30,30) α0.9 | Gray49 α0.8 | White90 | LabelPrimary | LabelPrimary | LabelPrimaryReversed | White α0.16 |
| Platterless | Black0 | White60 | White90 | LabelPrimary | LabelPrimaryReversed | LabelPrimaryReversed | none |
| Privacy | Black0 | White60 | White90 | LabelSecondary | LabelPrimaryReversed | LabelPrimaryReversed | none |
| KeyColor | SystemBlue α0.2 | White60 | SystemBlue | SystemBlue | LabelPrimaryReversed | LabelPrimary | none |
| Destructive | SystemRed α0.2 | White60 | SystemRed | SystemRed | LabelPrimaryReversed | LabelPrimary | none |

Candidate styling is returned only when requested by the export group. Disabled idle/focused text uses LabelTertiary/LabelTertiaryReversed; background follows its matching idle/focused variant. Published combinations: ordinary labelled Small/Regular/Large plus emphasized equivalents with Default/Dark/Platterless/KeyColor/Destructive; Privacy only ordinary Regular. Candidate groups expose Default/Dark at all sizes/weights and DarkPlayer only Regular/RegularEmphasized. Icon-only groups mirror the ordinary and candidate sets except Privacy is absent.

### A.9 List/settings button tokens

Source: factory **31398**, constructor `f(layout,variant,truncationMode)` and GDPR supplier `y`.

| Token | Default | Multiline / variant |
| --- | --- | --- |
| Height | 66 | 120 multiline |
| Padding top/bottom | 0/0 | 25/23 multiline |
| Padding leading/trailing | 30/30 | 30/30 multiline |
| Radius / horizontal expansion | 33/8 | same |
| Focus arrival/departure | 100/200 ms | same |
| Title / subtitle type | Regular.Body / not present | Regular.Body / Regular.Caption1 |
| Title/subtitle/decorator spacing | 40 / absent / 20 | 40 / 5 / 20 |
| Idle / focused background | White10 / White90 | NoFocus variant Black0 / Black0 |
| Idle primary / secondary text | LabelPrimary / LabelSecondary | same |
| Focused primary / secondary text | LabelPrimaryReversed / LabelSecondaryReversed | NoFocus retains unreversed roles |
| Disabled idle / focused text | LabelTertiary / LabelTertiaryReversed | same |
| Focused shadow | enabled | same |

| Truncation mode | Idle title / decorator | Focused title / decorator |
| --- | --- | --- |
| TruncatedDecorator | None / Ellipsis | None / Ellipsis |
| RollingTitle | Fade / None | Rolling / None |
| RollingDecorator | None / Fade | None / Rolling |
| RollingDecoratorAlways | None / Rolling | None / Rolling |

Published list variants: TruncatedDecorator; TruncatedDecoratorAlwaysEnabled (disabled style aliases enabled); TruncatedTitle (RollingTitle policy); RollingDecorator; MultipleLines; InfoPlatter (NoFocus + RollingDecoratorAlways); GDPR. GDPR inherits Regular.Platterless but uses focus durations 100/200, leading icon 39×27 with spacing20, Regular.Caption2, tertiary idle/disabled text, reversed primary focused text and reversed tertiary disabled-focused text.

### A.10 Animation primitives and default parameters

| Token / primitive | Value or normalized-time rule | Source |
| --- | --- | --- |
| Animation types | Snap, Linear, EaseIn, EaseOut, EaseInOut, Spring, StaticRubberBand | factory 21456 enum |
| Linear | p | AnimationQueue.linear |
| EaseIn | p² | AnimationQueue.easeIn |
| EaseOut | p×(2−p) | AnimationQueue.easeOut |
| EaseInOut | 2p² for p<0.5; second half is complementary quadratic | AnimationQueue.easeInOut |
| Generic duration / minimum step | 400 ms / 0 ms for byParams timed types | AnimationQueue.byParams |
| Spring damping/stiffness/mass | 30 / 150 / 1 defaults in byParams | AnimationQueue.byParams |
| Spring minimum linear velocity | 0.1 | AnimationQueue.spring / byParams |
| Spring space | ScreenCoordinate; scales distances by canvas factor | AnimationQueue.spring |
| Rubber-band duration / size | 400 / 400 defaults in byParams | AnimationQueue.byParams |
| CA-style spring mass / initial velocity / step | 1 / 0 / 0 defaults; stiffness/damping caller-supplied | AnimationQueue.caSpring |
| Forced-instant mode | Replaces animations with target assignment | AnimationQueue timed/spring methods |

### A.11 Coverage and unresolved values

- Complete declaration coverage within factory 44548 static Color assignments and light/dark semantic maps, factory 3038 named typography tables, factory 38413 dynamic tiers, factory 301 grid/radius presets, factory 31210 selectable defaults, and constructor-level button branches in 41457/31398.
- This is not the complete app inventory: page-specific overrides, shared card/shelf variants elsewhere in app.js, icon paths and runtime/server-provided artwork palettes need their separate inventories.
- Native text shaping, actual loaded fallback font, display viewport, feature flags and active accessibility settings were not executed or measured.
- Generic easing names are resolved where formulas are directly implemented; component-supplied spring/Bezier curves must retain their own values.
- No global spacing grid, universal corner radius, fixed card scale percentage or frame-rate guarantee is inferred from these tokens.

### A.12 Factory evidence coordinates

| Factory | Meaning | UTF-8 byte start | UTF-8 byte end (exclusive) |
| --- | --- | --- | --- |
| 44548 | Color | 2119261 | 2131276 |
| 55242 | Weight enum | 2134041 | 2134242 |
| 3038 | Typography | 2195936 | 2202714 |
| 38413 | Dynamic typography | 1961633 | 1968554 |
| 16393 | TextFormat | 2188142 | 2195935 |
| 46491 | Canvas scaling | 1865595 | 1867540 |
| 301 | Grid/radius | 2202715 | 2204864 |
| 31210 | Selectable defaults | 2393797 | 2394320 |
| 81539 | SelectableWidget | 2383618 | 2393796 |
| 1718 | DrawTools | 2131277 | 2134040 |
| 41457 | Standard button styles | 2262401 | 2272315 |
| 31398 | List button styles | 2272316 | 2277489 |
| 95055 | ButtonWidget | 2277490 | 2294482 |
| 21456 | Animation primitives | 2534476 | 2549590 |

### A.13 Floating menus: complete shared presentation defaults

Source: `app.js`, factory **46261**, bytes **2326272–2331533**; supplier `f` and button suppliers `p`/`m`. All geometry below is scaled except the explicitly unscaled shadow-offset array and animation factors. Material tokens remain native enum dependencies.

| Menu token | Value |
| --- | --- |
| Header type/color/alignment | Standard.Caption2 / White50 / vertical alignment 0.5 |
| Header height | 59 |
| Header padding leading/trailing/top/bottom | 25 / 25 / 22 / 8 |
| Platter radius | 54 |
| Show opacity, scale, duration | 0→1; 0.35→1; 380 ms |
| Hide duration | 210 ms |
| Background | RGB(30,30,30), alpha 0.9 |
| Solid/reduced-transparency background | Material.MediumDarkOpaque |
| Material / theme | native Regular / Dark |
| Width / maximum height | 454 / 600 |
| Top/bottom padding | 21 / 21 |
| Top fade inner extent / scroll fade | 62 / 22; ellipsis enabled |
| Bottom fade inner extent / scroll fade / clip expansion | 62 / 20 / 40; ellipsis enabled |
| Scroll animation | Spring; damping 30, stiffness 250; mass inherited from generic default 1 |
| Separator thickness | 1 |
| Separator solid/material color | White10 / White10 |
| Separator horizontal/vertical padding | 12 / 16 |
| Anchor padding | 20 |
| Submenu horizontal overlap | −26 |
| Minimum resized height | 250 |
| Loading row height | 74 |
| Loading label | Standard.Caption1 / LabelSecondary |
| Loading spinner | ExtraSmall native/shared enum dependency |
| Loading padding leading/trailing | 24 / 22 |
| Button title width | 323 |
| Two-line button height | 86 |
| Shadow color / radius / offset | Black30 / 30 / [0,10]; offset array is not passed through the scaling helper here |
| Border width/color | 1; borderColorHighContrast when focusStyle enabled, ordinary borderColor otherwise |
| Horizontal padding / item spacing | 21 / 4 |
| Submenu animation x displacement | 50 |

| Menu-row token | Value |
| --- | --- |
| Height / radius / horizontal expansion | 66 / 33 / 8 |
| Focus arrival / departure | 100 / 200 ms |
| Leading/trailing row padding | 0 / 20 |
| Leading icon box | 66×27; spacing 0; padding unspecified |
| Selectable-row icon override | height 20 in every available state |
| Title | Standard.Caption1; spacing 20; line limit 2 |
| Subtitle | Standard.Caption2; spacing 5 |
| Idle/candidate title/subtitle | LabelPrimary / LabelSecondary, Fade truncation |
| Candidate background | White15 |
| Focused title/subtitle | LabelPrimaryReversed / LabelSecondaryReversed, Rolling truncation |
| Focused background / shadow | Gray92 / enabled |
| Disabled idle title/subtitle | LabelTertiary; Fade |
| Disabled focused title/subtitle | LabelTertiaryReversed; Rolling; Gray92 background and shadow |

### A.14 Post-play/promotional shared composition

Source: factory **46827**, bytes **1625196–1626844**, base style `d`, dark-button exports `c`/`u`, transparent-forward variant `h`.

| Token | Value |
| --- | --- |
| Content width / button height | 454 / 66 |
| Padding leading/trailing/top/bottom | 80 / 80 / 60 / 80 |
| Logo→content gap | 24 |
| Metadata→content / body→description gap | 18 / 18 |
| Content→buttons gap | 32 |
| Between horizontal/vertical buttons | 24 / 14 |
| Button→legal line gap | 12 |
| Primary button minimum/maximum width | 220 / 440 |
| Forward button offset | −12 |
| Title font | Emphasized.Title2 / LabelPrimary |
| Small body font | Standard.Caption1 / LabelPrimary |
| Regular body font | Standard.Body / LabelPrimary |
| Emphasized body font | Emphasized.Callout / LabelPrimary |
| Description font | Standard.Body / LabelSecondary |
| Button footer font | Standard.Caption2 / LabelSecondary, text alignment 0.5 |
| Content logo box | 380×114 |
| Channel logo box | 260×114 |
| Fade-in | 240 ms |
| Standard button / icon button | Regular.Dark labelled / Regular.Dark icon-only |
| Transparent forward idle | Black0 background; Emphasized.Body icon format, White, page alignment 0.5; icon opacity 1; zero icon padding |
| Transparent forward focused | Inherits dark focused button; leading padding override 5 |

### A.15 Advisory/ratings overlay tokens

Source: factory **23408**, bytes **2359243–2360504**, style `s`. Durations are milliseconds; geometry is scaled.

| Token | Value |
| --- | --- |
| Rating/advisory image | 100×100, png |
| Divider margin / width / height | 28 / 1 / 140 |
| Legend box padding / radius | 5 / 5 |
| Legend block line height | 30 |
| Legend letter format | Emphasized.Body / Black; text/page alignment 0.5; scaling disabled |
| Legend→description gap | 12 |
| Description format | Standard.Caption2 / White; page alignment 0.5 |
| Maximum description width | warning overlay 300; ratings overlay 1000 |
| Description line vertical spacing | 5 |
| Image in delay/duration | 300 / 800 |
| Divider in delay/duration | 0 / 500 |
| Lines in delay | 600 when image exists, otherwise 0 |
| Lines in duration/stagger | 500 / 200 |
| End-of-in boundary | maximum of image end (if image), divider end (if image), and last line end |
| Out boundary | supplied in-end plus supplied display interval |
| Image out delay/duration | 0 / 500 after out boundary |
| Divider out delay/duration | 400 / 500 after out boundary |
| Lines out delay/duration | 0 / 500 after out boundary |

### A.16 Caption rendering and user-style tokens

Sources: factory **50836**, `VTTSubtitlesWidget`, bytes **2416621–2425477**; factory **60908**, caption-font resolver, bytes **1935056–1936064**; factory **80816**, caption enums, bytes **1931391–1935055**. These are app-rendered WebVTT styling rules; they do not establish that every media backend uses this renderer.

| Caption token | Value |
| --- | --- |
| Base typography | Standard.Subtitle1 (Regular, 38/46) unless supplied by caller |
| Initial font scale | 1 |
| Size multipliers | Small 0.5; Medium 1; Large 1.5; ExtraLarge 2 |
| Effective size | rounded resolved font size × caller fontScale × size multiplier |
| Minimum active-cue content height | maximum measured cue height, rounded up, plus 32 (raw units in this method) |
| Opacity ladder | Zero 0; TwentyFive 0.25; Fifty 0.5; SeventyFive 0.75; Hundred 1 |
| Text opacity aliases | Opaque 1; SemiTransparent 0.5; **Transparent 0.25**, not zero |
| Colors | Black #000000; Blue #0000FF; Cyan #00FFFF; Green #00FF00; Magenta #FF00FF; Red #FF0000; White #FFFFFF; Yellow #FFFF00 |
| Edge style options | None, Raised, Depressed, Uniform, DropShadow, passed to native character-edge style names |
| Regular/bold font weights | SanFranciscoMedium style uses Medium/Heavy; other styles use Regular/Bold |
| Italic weight | RegularItalic |
| Font extension | woff2 |
| Rounded background scaling | enabled initially; rounded(style radius × caller fontScale), or unscaled style radius when disabled |
| Caption font list screen scaling | disabled at this construction point; language-specific fallback list requested |

#### A. Caption font resolution

| Setting | Resolved family/path stem |
| --- | --- |
| Helvetica | HelveticaNeue |
| Avenir | Avenir-Roman |
| Copperplate | Copperplate |
| Iowan | **Courier** in this resolver; do not assume the name maps literally |
| Courier | Courier |
| Menlo | Menlo-Regular |
| SanFranciscoMono | SFMono-Light |
| SanFrancisco / Regular / Medium | SFHello-Regular, RegularItalic, Medium, Semibold, Bold, Heavy or Black according to resolved weight |
| Savoye / unrecognised setting fallback | SFHello-Medium |

#### A. Caption preset records

Source: factory **61202**, records at bytes **913625**, **914625**, **915582**, **916533**, **917502**. All five records set white opaque text, black background/highlight colors, userCreated false, and video-override flags true except background-opacity override false. These are preset declarations; the active user-selected preset is runtime state.

| Preset record | Main font | Size | Background alpha | Highlight alpha | Edge | Radius | ID |
| --- | --- | --- | --- | --- | --- | --- | --- |
| TransparentBackground | SanFranciscoMedium | Medium | 0.5 | 0 | None | scaled 12 | 1 |
| LargeText | Helvetica | Large | 0.5 | 0 | None | scaled 12 | 2 |
| Classic | Courier | Medium | 0 | 1 | None | 0 | 3 |
| OutlineText large record | SanFranciscoMedium | Large | 0 | 0 | Uniform | 0 | 4 |
| OutlineText medium record | SanFranciscoMedium | Medium | 0 | 0 | Uniform | 0 | 5 |

All preset records declare monospaced-serif/serif Courier, monospaced-sans Menlo, casual Avenir, script SanFranciscoMedium and small-caps Copperplate. The sans-serif slot is SanFranciscoMedium except LargeText uses Helvetica. Preset titles alone do not describe every literal field: TransparentBackground still declares 0.5 background alpha while permitting video to override background opacity.

### A.17 Additional primitive evidence

- Factory **21456**, `Animation.SpringAnimationIntegrationInterval`: **18 ms**. This is integration granularity, not an observed display frame rate.
- Factory **29942**, bytes **1936065–1936394**: text/glyph scale helpers are constant 1.
- Factory **84184**, bytes **2719133–2720930**, `CrossfadeTransition`: default duration **400 ms**, separation **0**, curve **easeOut**, fadeBottom false; default top/bottom fractional positions [0,0]. Animation capability may force duration to zero unless explicitly requested.

### A.18 Shared hero, canonical-lockup, footer and shelf tokens

The following tables exhaust the named data declarations in the selected style objects. A dependency expression is retained when screen state, content type, accessibility, another token or an imported factory determines its value. `scale(x)` denotes the shared dimension helper. Typography/color import prefixes are local aliases, so follow factory imports when resolving them. No widget constructors or application factories were executed.

#### A. Factory 394: bytes 1597470–1598959

| Token path | Static value / dependency | UTF-8 byte |
| --- | --- | --- |
| `l.cornerRadius` | scale(30) | 1597597 |
| `l.padding` | scale(20) | 1597618 |
| `l.qualityBadgeHeight` | scale(22) | 1597650 |
| `l.artwork.width` | scale(213) | 1597678 |
| `l.artwork.height` | scale(120) | 1597699 |
| `l.artwork.cornerRadius` | scale(10) | 1597726 |
| `l.artwork.fadeInDuration` | 240 | 1597754 |
| `l.artwork.placeholder` | ()=>s.yP.rgba(29,29,29) | 1597770 |
| `l.platter.focusedColor` | a.Q1.White20 | 1597817 |
| `l.platter.unFocusedColor` | a.Q1.White10 | 1597845 |
| `l.fonts.title` | typography: o.o.Emphasized.Body; color: a.Q1.LabelPrimary | 1597872 |
| `l.fonts.subtitle` | typography: o.o.Standard.Caption2; color: a.Q1.LabelSecondary | 1597947 |
| `l.fonts.subtitleMinSize` | o.o.Standard.Caption2.size-2 | 1598033 |
| `l.spacings.qualityBadgeToText` | scale(8) | 1598092 |
| `l.spacings.firstLine` | scale(2) | 1598114 |
| `l.spacings.secondLine` | scale(12) | 1598137 |
| `l.spacings.secondLineNarrow` | scale(4) | 1598167 |
| `l.spacings.secondLineSlim` | scale(2) | 1598194 |
| `d.width` | scale(560) | 1598217 |
| `d.height` | scale(397) | 1598238 |
| `d.twoLineHeight` | scale(435) | 1598266 |
| `d.threeLineHeight` | scale(489) | 1598296 |
| `d.cornerRadius` | scale(12) | 1598323 |
| `d.padding.leading` | scale(20) | 1598353 |
| `d.padding.trailing` | scale(20) | 1598375 |
| `d.padding.bottom` | scale(30) | 1598395 |
| `d.artwork.width` | scale(560) | 1598424 |
| `d.artwork.height` | scale(315) | 1598445 |
| `d.fonts.title` | typography: o.o.Emphasized.Callout; color: a.Q1.LabelPrimary | 1598473 |
| `d.fonts.labelOne` | typography: o.o.Standard.Body; color: a.Q1.LabelSecondary | 1598551 |
| `d.fonts.labelTwo` | typography: o.o.Standard.Callout; color: a.Q1.LabelSecondary | 1598626 |
| `d.fonts.titleBaseline` | scale(52) | 1598709 |
| `d.fonts.labelOneBaseline` | scale(38) | 1598739 |
| `d.fonts.labelTwoBaseline` | scale(54) | 1598769 |
| `d.fonts.footer` | typography: o.o.Standard.Caption2; color: a.Q1.LabelSecondary; textAlignment: .5 | 1598789 |
| `d.platter.unfocusedColor` | s.yP.white.alpha(.1) | 1598901 |
| `d.platter.focusedColor` | s.yP.white.alpha(.25) | 1598935 |

#### A. Factory 58119: bytes 1529088–1537517

| Token path | Static value / dependency | UTF-8 byte |
| --- | --- | --- |
| `g` | r.JM.Regular.Dark() | 1529321 |
| `p` | r.jM.Regular.Dark() | 1529343 |
| `m` | r.Rw.Regular.Dark() | 1529365 |
| `f` | r.pz.Regular.Dark() | 1529387 |
| `y.[inherited]` | r.jM.Regular.Platterless() | 1529410 |
| `y.enabled.[inherited]` | r.jM.Regular.Platterless().enabled | 1529449 |
| `y.enabled.Focused.[inherited]` | r.jM.Regular.Platterless().enabled.Focused | 1529496 |
| `v.[inherited]` | y | 1529548 |
| `v.enabled.Idle.[inherited]` | p.enabled.Focused | 1529568 |
| `S.width` | (0,u.F8)()===u.J3.HD_720p?scale(680):scale(510) | 1529601 |
| `S.anchorHeight` | u.nl.height-scale(272) | 1529668 |
| `S.anchorHeightOffsetEpicStage` | scale(30) | 1529722 |
| `S.anchorHeightOffsetCanonical` | scale(64) | 1529763 |
| `S.titleImage.small.maxWidth` | scale(283) | 1529804 |
| `S.titleImage.small.maxHeight` | scale(85) | 1529828 |
| `S.titleImage.large.maxWidth` | scale(420) | 1529858 |
| `S.titleImage.large.maxHeight` | scale(126) | 1529882 |
| `S.titleImage.fadeDuration` | 200*!!c.w.Config.Performance.otherAnimationsEnabled | 1529910 |
| `S.titleImage.padding.image.top` | scale(28) | 1529982 |
| `S.titleImage.padding.text.epic.top` | scale(28) | 1530012 |
| `S.titleImage.padding.text.canonical.top` | scale(6) | 1530041 |
| `S.metadata.width` | scale(680) | 1530073 |
| `S.metadata.providerLogo.imageMaxWidth` | scale(40) | 1530115 |
| `S.metadata.providerLogo.imageMaxHeight` | scale(40) | 1530143 |
| `S.metadata.providerLogo.storeIconMaxWidth` | scale(18) | 1530174 |
| `S.metadata.providerLogo.storeIconMaxHeight` | scale(18) | 1530206 |
| `S.metadata.providerLogo.storeBackgroundColor` | o.Q1.SystemGray5() | 1530240 |
| `S.metadata.providerLogo.backgroundColor` | h.yP.black | 1530275 |
| `S.metadata.providerLogo.hairline.thickness` | scale(1) | 1530306 |
| `S.metadata.providerLogo.hairline.color` | o.Q1.colorWithAlpha(o.Q1.White,.16) | 1530324 |
| `S.ratingHeight` | scale(22) | 1530376 |
| `S.spacings.badgeToClock` | scale(12) | 1530412 |
| `S.spacings.smallTitle` | scale(18) | 1530436 |
| `S.spacings.title` | scale(13) | 1530455 |
| `S.spacings.metadataEpic` | scale(20) | 1530481 |
| `S.spacings.metadataCanonical` | scale(18) | 1530512 |
| `S.spacings.body` | scale(18) | 1530530 |
| `S.spacings.button` | scale(32) | 1530550 |
| `S.spacings.explainabilityLine` | scale(18) | 1530582 |
| `S.spacings.buttonToFooter` | scale(15) | 1530610 |
| `S.detailsSpacing` | scale(12) | 1530639 |
| `S.body.maxLines.epic` | 3 | 1530673 |
| `S.body.maxLines.canonical` | 4 | 1530685 |
| `S.body.maxLines.episode` | 2 | 1530695 |
| `S.body.width.epic` | scale(570) | 1530710 |
| `S.body.width.canonical` | scale(667) | 1530734 |
| `S.body.width.episode` | scale(860) | 1530756 |
| `S.title.maxWidth.canonical` | scale(667) | 1530799 |
| `S.title.maxWidth.episode` | scale(600) | 1530821 |
| `S.formats.title` | typography: l.o.Emphasized.Title3; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 0; paragraphSpacingAfter: 0 | 1530852 |
| `S.formats.titleMaxWidth` | scale(600) | 1530990 |
| `S.formats.titleMaxLines` | 4 | 1531018 |
| `S.formats.upsellTitle` | typography: l.o.Emphasized.Title2; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 0; paragraphSpacingAfter: 0 | 1531032 |
| `S.formats.upsellTitleMaxLines` | 3 | 1531176 |
| `S.formats.upsellTitleMinFontSize` | l.o.Emphasized.Title3.size | 1531201 |
| `S.formats.smallTitle` | typography: l.o.Emphasized.Callout; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 0; paragraphSpacingAfter: 0 | 1531239 |
| `S.formats.smallTitleMaxLines` | 2 | 1531383 |
| `S.formats.smallTitleMinFontSize` | l.o.Emphasized.Body.size | 1531407 |
| `S.formats.headline` | typography: l.o.Emphasized.Headline; color: o.Q1.LabelSecondary; textAlignment: 0; pageAlignment: 0; paragraphSpacingAfter: 0 | 1531441 |
| `S.formats.headlineMaxLines` | 2 | 1531586 |
| `S.formats.headlineMaxWidth` | scale(600) | 1531605 |
| `S.formats.headlineMinFontSize` | l.o.Emphasized.Body.size | 1531639 |
| `S.formats.details` | typography: l.o.Standard.Caption1; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: .5; paragraphSpacingAfter: 0 | 1531672 |
| `S.formats.body.title.default` | typography: l.o.Emphasized.Caption1; color: o.Q1.LabelSecondary_RespectTransparencySetting; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1531818 |
| `S.formats.body.title.small` | typography: l.o.Emphasized.Caption2; color: o.Q1.LabelSecondary_RespectTransparencySetting; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1531979 |
| `S.formats.body.text.default` | typography: l.o.Standard.Caption1; color: o.Q1.LabelSecondary_RespectTransparencySetting; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1532149 |
| `S.formats.body.text.small` | typography: l.o.Standard.Caption2; color: o.Q1.LabelSecondary_RespectTransparencySetting; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1532308 |
| `S.formats.body.maxLines.epic` | 3 | 1532477 |
| `S.formats.body.maxLines.canonical` | 4 | 1532489 |
| `S.formats.body.maxLines.episode` | 2 | 1532499 |
| `S.formats.body.width.epic` | scale(570) | 1532514 |
| `S.formats.body.width.canonical` | scale(667) | 1532538 |
| `S.formats.body.width.episode` | scale(860) | 1532560 |
| `S.formats.footer` | typography: l.o.Standard.Caption2; color: o.Q1.LabelSecondary; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1532583 |
| `S.formats.footerReducedTransparency` | typography: l.o.Standard.Caption2; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1532735 |
| `S.formats.footerMaxLines` | 2 | 1532874 |
| `S.formats.footerMinFontSize` | l.o.Standard.Caption3.size | 1532894 |
| `S.formats.footerMarqueeStyle.speed` | scale(30) | 1532947 |
| `S.formats.footerMarqueeStyle.insetFadeTime` | 300 | 1532974 |
| `S.formats.footerMarqueeStyle.wait` | 2700 | 1532983 |
| `S.formats.footerMarqueeStyle.spacing` | scale(150) | 1532996 |
| `S.formats.footerMarqueeStyle.restingMode` | h.bJ.RestingMode.Masked | 1533022 |
| `S.formats.explainabilityLine` | typography: l.o.Standard.Caption1; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 1; paragraphSpacingAfter: 0 | 1533066 |
| `S.button.minWidth` | scale(220) | 1533208 |
| `S.button.maxWidthEpicStage` | scale(390) | 1533240 |
| `S.button.maxWidthCanonical` | scale(420) | 1533272 |
| `S.button.style` | g | 1533292 |
| `S.iconSize.play` | scale(26) | 1533310 |
| `S.iconSize.default` | scale(30) | 1533331 |
| `S.iconSize.info` | scale(32) | 1533349 |
| `S.forwardButton.offset` | -scale(12) | 1533385 |
| `S.progressBar.spacing` | scale(12) | 1533421 |
| `w.[inherited]` | S | 1533439 |
| `w.teams.width` | scale(500) | 1533457 |
| `w.teams.height` | scale(158) | 1533478 |
| `w.teamLogoSize` | scale(76) | 1533506 |
| `w.teamLogoFadeInDuration` | 200*!!c.w.Config.Performance.otherAnimationsEnabled | 1533542 |
| `w.flexibleLogo.width` | scale(260) | 1533614 |
| `w.flexibleLogo.height` | scale(50) | 1533635 |
| `w.flexibleLogo.fadeDuration` | 200*!!c.w.Config.Performance.otherAnimationsEnabled | 1533661 |
| `w.flexibleLogo.padding.bottom` | scale(20) | 1533729 |
| `w.spacings.[inherited]` | S.spacings | 1533754 |
| `w.spacings.details` | scale(14) | 1533776 |
| `w.spacings.symbols` | scale(8) | 1533797 |
| `w.spacings.teams` | scale(20) | 1533815 |
| `w.spacings.title` | scale(14) | 1533834 |
| `w.spacings.logo` | scale(28) | 1533852 |
| `w.spacings.teamRows` | scale(6) | 1533874 |
| `w.spacings.logoToTeamName` | scale(12) | 1533901 |
| `w.spacings.teamNameToScore` | scale(40) | 1533930 |
| `w.spacings.body` | scale(18) | 1533948 |
| `w.formats.[inherited]` | S.formats | 1533971 |
| `w.formats.symbolDetails` | typography: {weight:d.I.Medium,size:18,leading:18}; color: o.Q1.LabelSecondary; textAlignment: 0; pageAlignment: .5; paragraphSpacingAfter: 0 | 1533998 |
| `w.formats.teamName` | typography: {fontName:"SFCompressed-Semibold",weight:d.I.Semibold,size:l.o.Standard.Title1.size,leading:20}; color: o.Q1.LabelPrimary; textAlignment: 0; pageAlignment: 0; paragraphSpacingAfter: 0 | 1534151 |
| `w.formats.teamNameMinFontSize` | Math.floor(.7*l.o.Emphasized.Title3.size) | 1534369 |
| `w.formats.teamScore` | typography: {fontName:"SFCompressed-Semibold",weight:d.I.Semibold,size:l.o.Standard.Title1.size,leading:20}; color: o.Q1.LabelPrimary; textAlignment: 1; pageAlignment: 0; paragraphSpacingAfter: 0 | 1534421 |
| `w.formats.losingTeamScore` | typography: {fontName:"SFCompressed-Semibold",weight:d.I.Semibold,size:l.o.Standard.Title1.size,leading:20}; color: o.Q1.LabelSecondary; textAlignment: 1; pageAlignment: 0; paragraphSpacingAfter: 0 | 1534635 |
| `w.formats.teamScoreReducedTransparency` | typography: {fontName:"SFCompressed-Semibold",weight:d.I.Semibold,size:l.o.Standard.Title1.size,leading:20}; color: o.Q1.LabelPrimary; textAlignment: 1; pageAlignment: 0; paragraphSpacingAfter: 0 | 1534864 |

#### A. Factory 65581: bytes 745323–747720

| Token path | Static value / dependency | UTF-8 byte |
| --- | --- | --- |
| `p.spacing.productCards` | scale(40) | 745594 |
| `p.rects.fullscreen` | h | 745626 |
| `p.rects.cardView` | g | 745637 |
| `p.cornerRadius` | scale(36) | 745653 |
| `p.trailerRect` | h | 745678 |
| `p.subtitleArea.fullscreen` | new d.r(scale(830),scale(160),scale(1010),scale(700)) | 745705 |
| `p.subtitleArea.cardView` | new d.r(scale(910),scale(200),scale(810),scale(660)) | 745780 |
| `p.subtitleArea.fullscreenTight` | new d.r(scale(830),scale(160),scale(1010),scale(430)) | 745861 |
| `p.subtitleArea.cardViewTight` | new d.r(scale(910),scale(200),scale(810),scale(390)) | 745941 |
| `p.timing.cardArtworkDelay` | 2500 | 746032 |
| `p.timing.autoPlayTrailer` | 2e3 | 746053 |
| `p.timing.fadeInTrailer` | 500 | 746071 |
| `p.timing.fadeOutTrailer` | 400 | 746090 |
| `p.timing.fadeInChevron` | 300 | 746108 |
| `p.timing.zoomCard` | 300 | 746121 |
| `p.timing.fadeInDelay` | 500 | 746137 |
| `p.getScrollParams` | ()=>a.O.accessibility.getReduceMotion()?{type:c.Y5.Snap}:{type:c.Y5.Spring,mass:1,damping:20,stiffness:100} | 746158 |
| `p.delays.minimumLoadingPageDisplay` | 300 | 746300 |
| `p.protectionGradient.height` | scale(416) | 746332 |
| `p.protectionGradient.solidHeight` | 0 | 746358 |
| `p.protectionGradient.midPointOpacity` | .5 | 746376 |
| `m.titleSpacing` | scale(30) | 746397 |
| `m.groupSpacing` | scale(57) | 746423 |
| `m.groupWidth` | scale(542) | 746447 |
| `m.title.font` | typography: o.o.Emphasized.Headline; color: s.Q1.LabelSecondary; vibrancyEffectStyle: "secondary" | 746473 |
| `m.tuneIn.font` | typography: {...o.o.Emphasized.Caption1,leading:30}; color: s.Q1.LabelPrimary; paragraphSpacingAfter: 0; paragraphSpacingBefore: 0 | 746591 |
| `m.tuneIn.marginTop` | scale(60) | 746736 |
| `m.multilineText.titleSpacing` | scale(30) | 746778 |
| `m.multilineText.horizontalBoxPadding` | scale(15) | 746812 |
| `m.multilineText.verticalBoxPadding` | scale(15) | 746844 |
| `m.multilineText.backgroundFocusColor.dark` | new n.Gfx.Color(1,1,1,.4) | 746884 |
| `m.multilineText.backgroundFocusColor.light` | new n.Gfx.Color(1,1,1,.4) | 746916 |
| `m.multilineText.backgroundUnfocusColor` | new n.Gfx.Color(1,1,1,0) | 746966 |
| `m.multilineText.cornerRadius` | scale(20) | 747004 |
| `m.multilineText.pressedStyle` | l.e.Deflate | 747030 |
| `m.multilineText.moreTextStyle.subtitle` | typography: o.o.Standard.Caption2; color: s.Q1.LabelTertiary; vibrancyEffectStyle: "tertiary"; paragraphSpacingAfter: 0 | 747066 |
| `m.multilineText.moreTextStyle.body` | typography: o.o.Standard.Caption2; color: s.Q1.LabelSecondary; vibrancyEffectStyle: "secondary"; paragraphSpacingAfter: 0 | 747195 |
| `m.multilineText.moreTextStyle.ellipsis` | typography: o.o.Emphasized.Caption2; color: s.Q1.LabelSecondary | 747330 |
| `m.multilineText.moreTextStyle.alignment` | 1 | 747412 |
| `m.multilineText.moreTextStyle.hideOrphanSubheadings` | !0 | 747436 |
| `m.multilineText.moreTextExpandedStyle.subtitle` | typography: o.o.Standard.Body; color: s.Q1.LabelSecondary; vibrancyEffectStyle: "secondary"; paragraphSpacingAfter: 0 | 747472 |
| `m.multilineText.moreTextExpandedStyle.body` | typography: o.o.Standard.Body; color: s.Q1.LabelPrimary; vibrancyEffectStyle: "primary"; paragraphSpacingAfter: 0 | 747599 |

#### A. Factory 81935: bytes 744467–745322

| Token path | Static value / dependency | UTF-8 byte |
| --- | --- | --- |
| `c.width` | scale(1920) | 744602 |
| `c.artworkHeight` | scale(716) | 744631 |
| `c.logoAlignment` | scale(100) | 744659 |
| `c.shelfOffset` | e=>d.w.isSportingEventNodeEx(e)?scale(722):scale(742) | 744685 |
| `c.solidBackgroundColor` | e=>{let t,i=e.getTheme();(t=r.O.accessibility.reduceTransparency()?o.Q1.AccessibilityReduceTransparencyBackground[i]:e.getColor(l.Z.ColorType.Background))===o.Q1.Black0&&(t=o.Q1.Background);let n=o.Q1.getLuminosityFor(t);return o.Q1.blendColors(t,o.Q1.Black,n>.5?.55:.2)} | 744766 |
| `c.overlay` | e=>{let t=new(a()).Gfx.Color(e.getColor(l.Z.ColorType.Background));return o.Q1.getLuminosityFor(t)>=.9?o.Q1.colorWithAlpha(o.Q1.Black,.55):o.Q1.colorWithAlpha(o.Q1.Black,.2)} | 745046 |
| `c.belowTheFoldBlurDownScaleFactor` | 4 | 745253 |
| `c.belowTheFoldBlurRadius` | scale(100) | 745278 |
| `c.artworkCrossFadeDuration` | 300 | 745317 |

#### A. Shelf headers and spacing

Sources: factory **219** bytes **394118–395151**; factory **47706** bytes **391406–392300**.

| Token | Reference value |
| --- | --- |
| `Header.TopSpacing` | 20 |
| `Header.BottomSpacingNoSubtitle` | 14 |
| `Header.BottomSpacingWithSubtitle` | 22 |
| `Header.SingleLineTitleHeight` | 72 |
| `Header.FocusableSingleLineTitleHeight` | 86 |
| `Header.TwoLinesTitleHeight` | 110 |
| `Header.TitleToSubtitleSpacing` | 0 |
| `Header.TitleToShelfItemsSpacing` | 14 |
| `Header.IdleRichHeaderToShelfItemsSpacing` | 22 |
| `Header.FocusedRichHeaderToShelfItemsSpacing` | 21 |
| `Header.RaisedRichHeaderToShelfItemsSpacing` | 21 |
| `Header.RichHeaderHeight` | 112 |
| `Header.FocusableRichHeaderHeight` | 116 |
| `Header.RaisedTitleToShelfItemsSpacing` | 21 |
| `Header.MaximumWidthAfterEpicStage` | scale(776) − safe horizontal inset |
| `Header.TopSpacingPromotionalRoom` | 8 |
| `Header.TitleFormat` | Emphasized.Callout / LabelSecondary |
| `Header.UpNextTitleFormat` | Emphasized.Callout / LabelPrimary |
| `Header.SubTitleFormat` | Standard.Body / LabelTertiary |
| `TopPadding.Default` | 0 |
| `TopPadding.DefaultNoTitle` | 48 |
| `TopPadding.Picker` | 20 |
| `TopPadding.MainTabPadding` | 137 |
| `TopPadding.RoomPadding` | 12 |
| `TopPadding.EpicStage` | 888 |
| `TopPadding.SegmentedSearchResultShelves` | 38 |
| `BottomPadding.Default` | 55 |
| `BottomPadding.DefaultTwoLines` | 114 |
| `BottomPadding.LeagueStandingsWithoutLegend` | 40 |
| `BottomPadding.LeagueStandingsWithLegend` | 100 |
| `BottomPadding.SearchCards` | 26 |
| `BottomPadding.SearchCardsMultipleRows` | 59 |
| `BottomPadding.PersonLockupOneLine` | 91 |
| `BottomPadding.PersonLockupTwoLines` | scale(91)+scale(33) |
| `ScrollDistance.EpicStage` | 900 |
| `EpicStageShelfPeeking` | 120 |
| `SpringAnimParams` | Spring damping30/stiffness150/mass1 |
| `SpringAnimParamsSlow` | Spring damping20/stiffness100/mass1 |

Factory 65581's rectangle dependencies resolve as follows: `h` is the current full viewport rectangle; `g` is x=120, y=40, width=1680, height=1040, each scaled. Its subtitle rectangles are x/y/width/height: fullscreen 830/160/1010/700; card 910/200/810/660; fullscreen-tight 830/160/1010/430; card-tight 910/200/810/390. Factory 58119 style aliases are `g` Regular.Dark labelled, `p` Regular.Dark icon-only, `m` focus-candidate Regular.Dark labelled, `f` focus-candidate Regular.Dark icon-only; `y` starts with Regular.Platterless icon-only, and `v` substitutes dark-button focused styling as its idle state.

Numeric header/shelf geometry is scaled. The hero solid-background and overlay expressions in factory 81935 depend on artwork palette and contrast, not a global fixed background. Factory 58119 contains additional SVG gradient assets: bottom vertical alpha stops 0%→0, 64%→0, 71%→0.03, 78%→0.14, 84%→0.27, 91%→0.41, 95%→0.54, 100%→0.68; side linear 0%→0.14,100%→0; principal radial alpha 0%→0,19%→0.04,36%→0.15,51%→0.30,53%→0.33,65%→0.49,77%→0.67,89%→0.85,100%→1. The gradient geometry remains artwork-specific, not a reusable universal scrim.

## B. Pages and component overrides

Build: `16.4.4.J34L139`. Extraction date: 15 October 2026. Static inspection only; no TV/app execution.

This companion records actual assigned page/component style data from the saved effective JavaScript. It supplements the principal token catalogue. It is not an official token specification and does not establish which remote feature variants were live.

### B. Reading and coverage rules

- Source offsets are zero-based UTF-8 byte offsets in the original named bundle; factory numbers identify webpack factories.
- `scaled(n)` below is a presentation shorthand for a source call of the form `(0, alias.yA)(n)`; the original alias imports sizing factory `46491`. It is not an assertion of physical pixels.
- Constructor arguments, theme roles, arithmetic, inherited objects and conditional callbacks are retained as expressions when not resolved here. A timing constant is not observed latency.
- Each appendix table enumerates the leaves of one explicit top-level component/style object in its factory. `(spread)` means inherited fields, not replacement of the inherited object. Undefined values are deliberate source values.
- The tables omit SVG path geometry and long executable function bodies. Inline drawing/animation calls, local function-scoped objects, scalar variables and runtime-derived styles are outside this declaration appendix; important extra inline values are listed separately. The principal whole-bundle inventory covers a broader candidate set.
- There is no inferred universal spacing/radius scale. A component value remains scoped to its owner.

### B. High-value component groups

| Component | Token set | Evidence |
| --- | --- | --- |
| Season pills | Height 66; radius 33; leading/trailing padding 30; horizontal focus expansion 8; focus gain 100 ms/loss 200 ms; Emphasized.Callout | `ProductPage.js`, factory `86794`, `eI`, byte 17299 |
| Season/episode rail | Vertical distance 11; spacing multi/single group 35/18; row heights multi/single 54/71; group spacing 20 | `ProductPage.js`, factory `86794`, `eF`, byte 17083 |
| Description card | Width 560; height 292; radius 30; content padding 22 leading/trailing, 18 top, 20 bottom; body height 150/180 for 5/6 lines | `ProductPage.js`, factory `86794`, `ej`/`e$`, bytes 37454/37606 |
| About score cards | Width 260, max width 560; height 292; radius 30; content padding 22; header baseline 78, superscript 46, source 48, bottom padding 28 | `ProductPage.js`, factory `86794`, `eq`/`eJ`, bytes 40204/40380 |
| Canonical title actions | Anchor height viewport height minus scaled 162; button-row height 66; gaps metadata 18, button row 32, buttons 24, features 10 | `ProductPage.js`, factory `86794`, `tE`, byte 61087 |
| Browse filter bar | Top/bottom padding 30/26; button gap 16 | `SuperBrowsePage.js`, factory `17380`, `O`, byte 760 |
| Expandable browse menu | Horizontal/vertical panel padding 21/19; max height 600; row 408 by 66, row padding 4; focused growth 4; pill height 66 | `SuperBrowsePage.js`, factory `17380`, `J`, byte 10893 |
| Menu springs | Expand 25 stiffness/185 damping/1 mass; press shrink 300/50/1; press bounce 150/20/1 | Same `J` object |
| Related-shelf crossfade | Down 170 ms at .98 scale; up 150 ms at 1.03; settle 430 ms; stagger 100 ms; glimmer exit 230 ms at .95 | `SuperBrowsePage.js`, factory `38673`, `a`, byte 122065 |
| Search composition | Keyboard width 432; input y 139 and height 85; content y 251; first-row shift 105; input padding 20 leading/trailing | `SearchPageFactory.js`, factory `58454`, `Q`/`X`/`N`, bytes 4546/4775/5704 |
| Search segment platter | Pill height scaled(66) + 2*scaled(9); radius ceil(pillHeight/2); max width 1260; button gap 8; divider gap 32; shadow blur 48, y 4 | `SearchPageFactory.js`, factory `58454`, `ex`, byte 8800 |
| Library layout | Menu width 320; horizontal card gaps 40; family vertical gap 56; library vertical gap 60; initial-spinner hold 4500 ms and fade 400 ms | `LibraryPage.js`, factory `90601`, `ey`, byte 19435 |

### B. Additional scalar, inline and conditional evidence

| Scope | Exact data / condition | Source locator |
| --- | --- | --- |
| Episode data work | Episode fetch delay 500 ms | `ProductPage.js`, factory `11739`, `EpisodePaging.loadEpisodes`, byte 3172 (`delay`) |
| Product canvas scrolling | Spring damping 30, stiffness 150, mass 1 | `ProductPage.js`, factory `86794`, `ProductCanvasFactory` closure, byte 54823 |
| Product fold narrow variant | Above-fold horizontal padding scaled(121)*unfoldedAmount; top scaled(42)*unfoldedAmount; canonical extra horizontal padding scaled(80)*(1-scaleValue) | `ProductPage.js`, `ProductPageInner.animateFold`, byte 80490 (`animateFold`) |
| Product fold distance | Artwork height + first shelf height + scaled(180), multiplied by fold amount | `ProductPage.js`, `ProductPageInner.animateFold`, byte 80490 (`animateFold`) |
| Product image preload | Timeout passed to image load 7000 ms | `ProductPage.js`, `ProductPageInner.preloadContentImages`, byte 80323 (`preloadContentImages`) |
| About-card subtitle | One line; minimum size inherited subtitle typography size minus 2 | `ProductPage.js`, `AboutCardLockup` content, byte 40118 vicinity |
| Common Sense card body | Maximum 4 lines | `ProductPage.js`, `CommonSenseLockup`, byte 41320 vicinity |
| Footer text budget | 7 lines for tune-in branch; 9 narrow; 16 normal | `ProductPage.js`, `FooterWidget_FooterWidget.updateContent`, byte 12339 (`updateContent`) |
| High-contrast search platter | Background SystemGray5_HighContrast vs SystemGray5; shadow opacity .2 vs .4; borderColorHighContrast vs borderColor | `SearchPageFactory.js`, factory `58454`, `ex`, byte 8800 |
| Search query delivery | Incremental delay 200 ms | `SearchPageFactory.js`, search input controller, byte 38189 (`delay`) |
| Search suggestion announcement | 1500 ms | `SearchPageFactory.js`, factory `58454`, `N.accessibility.suggestionTime` |
| Library category debounce | 200 ms except first/default-category immediate branches | `LibraryPage.js`, `LibraryPage` constructor, byte 22310 (constructor) |
| Library page replacement | 150 ms, linear, separation .1, fadeBottom true | `LibraryPage.js`, `LibraryPage.browse`, bytes 30102/30458 |
| Library menu | Fade-out 170 ms; fade-in delay 167 ms, fade-in 170 ms; specific custom focus-origin skips delay | `LibraryPage.js`, `LibraryPage.fadeOut/fadeIn`, bytes 35166–35553 |
| Library badge | Padding scaled(14) | `LibraryPage.js`, factory `90601`, scalar `eR`, byte 21583 |
| Library icon | Symbol size 21; Focused/Highlighted LabelPrimaryReversed; FocusCandidate LabelPrimary; otherwise White80 | `LibraryPage.js`, helper `e_`, byte 21744 |
| Multi-choice divider | Opacity .2 * expansion progress | `SuperBrowsePage.js`, `MultiChoiceMenu._updateItemWidgets`, byte 62822 |
| Glimmer exit | Cubic Bézier [.33,0,.33,1] | `SuperBrowsePage.js`, `SuperBrowseShelf.animateSlotExit`, byte 125029 |
| Browse customization backdrop | Other shelves opacity .1 | `SuperBrowsePage.js`, customization state, byte 111176 |

### B. Declaration appendix

The appendix contains **67 declaration groups / 975 leaf rows**. Some rows are inherited references or conditional expressions rather than independent literal tokens.

#### B. LibraryPage.js — LibraryPage

Factory `90601`; source declaration `ey`; object begins at byte **19435**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ey.titleFormat.typography` | `em.o.Emphasized.Title1` | 19469 |
| `ey.titleFormat.color` | `x.Q1.LabelSecondary` | 19498 |
| `ey.titleFormat.textAlignment` | `0` | 19532 |
| `ey.bodyFormat.typography` | `em.o.Standard.Body` | 19568 |
| `ey.bodyFormat.textAlignment` | `.5` | 19601 |
| `ey.expirationColor` | `new(eu()).Gfx.Color("#FF453A")` | 19622 |
| `ey.familyMemberHorizontalSpacing` | `scaled(40)` | 19683 |
| `ey.familyMemberVerticalSpacing` | `scaled(56)` | 19725 |
| `ey.libraryItemHorizontalSpacing` | `scaled(40)` | 19768 |
| `ey.libraryItemVerticalSpacing` | `scaled(60)` | 19809 |
| `ey.menuWidth` | `scaled(320)` | 19833 |
| `ey.initialSpinner.size` | `_.Q.Size.Large` | 19869 |
| `ey.initialSpinner.holdTime` | `4500` | 19893 |
| `ey.initialSpinner.fadeInDuration` | `400` | 19913 |
| `ey.brandDividerTextFormat.typography` | `em.o.Emphasized.Caption2` | 19962 |
| `ey.brandDividerTextFormat.color` | `()=>X.O.accessibility.focusStyle?x.Q1.LabelSecondary():x.Q1.LabelTertiary()` | 19993 |
| `ey.brandDividerTextFormat.textAlignment` | `0` | 20083 |

#### B. LibraryPage.js — LibraryShowPage

Factory `90601`; source declaration `eN`; object begins at byte **37133**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eN.titleFormat.typography` | `em.o.Emphasized.Title1` | 37167 |
| `eN.titleFormat.color` | `x.Q1.LabelSecondary` | 37196 |
| `eN.titleFormat.textAlignment` | `.5` | 37230 |
| `eN.titleFormat.pageAlignment` | `.55` | 37247 |
| `eN.verticalSpacing` | `scaled(68)` | 37269 |

#### B. ManageSportsPage.js — component style `Z`

Factory `86210`; source declaration `Z`; object begins at byte **556**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `Z.mySportsIcon.size.width` | `104.882` | 1594 |
| `Z.mySportsIcon.size.height` | `71.191` | 1609 |
| `Z.padding.top` | `scaled(46)` | 1631 |
| `Z.padding.bottom` | `scaled(168)` | 1651 |
| `Z.header.iconHeight` | `scaled(30)` | 1685 |
| `Z.header.format.typography` | `W.o.Emphasized.Callout` | 1725 |
| `Z.header.format.color` | `O.Q1.LabelPrimary` | 1754 |
| `Z.header.iconToTextSpacing` | `scaled(24)` | 1792 |
| `Z.header.padding.top` | `scaled(64)` | 1818 |
| `Z.header.padding.leading` | `scaled(18)` | 1839 |
| `Z.menu.width` | `scaled(320)` | 1866 |
| `Z.menu.leagueIcon.width` | `scaled(50)` | 1898 |
| `Z.menu.leagueIcon.height` | `scaled(50)` | 1918 |
| `Z.menu.buttonStyle` | `U.Z.Default()` | 1944 |
| `Z.content.width` | `5*scaled(240)+4*H.x.L.horizontalSpacing` | 1974 |
| `Z.content.itemSize` | `scaled(240)` | 2025 |
| `Z.content.horizontalSpacing` | `H.x.L.horizontalSpacing` | 2057 |
| `Z.content.verticalSpacing` | `scaled(60)` | 2097 |
| `Z.spinner.size` | `P.Q.Size.Large` | 2125 |
| `Z.spinner.holdTime` | `1500` | 2149 |
| `Z.spinner.fadeInDuration` | `400` | 2169 |
| `Z.teamLogoFadeInDuration` | `240` | 2197 |

#### B. MovieBundlePage.js — component style `b`

Factory `77822`; source declaration `b`; object begins at byte **312**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `b.titleFormat.typography` | `m.o.Emphasized.Title2` | 345 |
| `b.titleFormat.color` | `y.Q1.White` | 373 |
| `b.titleFormat.textAlignment` | `.5` | 398 |
| `b.purchasedFormat.typography` | `m.o.Standard.Body` | 439 |
| `b.purchasedFormat.color` | `y.Q1.White40` | 463 |
| `b.purchasedFormat.textAlignment` | `.5` | 490 |
| `b.purchasedFormat.pageAlignment` | `.5` | 507 |

#### B. MultiViewPage.js — component style `J`

Factory `66912`; source declaration `J`; object begins at byte **1477**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `J.videoWidgets.rects.fullscreen` | `new w.r(G.nl.rect)` | 1510 |
| `J.videoWidgets.rects.initialEmptyLeft` | `new w.r(scaled(508),scaled(372),0,0)` | 1546 |
| `J.videoWidgets.rects.initialEmptyRight` | `new w.r(scaled(1412),scaled(372),0,0)` | 1605 |
| `J.videoWidgets.rects.singleCentered` | `new w.r(scaled(530),scaled(130),scaled(864),scaled(486))` | 1662 |
| `J.videoWidgets.rects.singleCenteredCornerRadius` | `scaled(24)` | 1754 |
| `J.videoWidgets.rects.editMode.twoUpLeftLarge.left` | `new w.r(scaled(308),scaled(130),scaled(864),scaled(486))` | 1798 |
| `J.videoWidgets.rects.editMode.twoUpLeftLarge.leftCornerRadius` | `scaled(24)` | 1880 |
| `J.videoWidgets.rects.editMode.twoUpLeftLarge.right` | `new w.r(scaled(1212),scaled(260),scaled(400),scaled(225))` | 1899 |
| `J.videoWidgets.rects.editMode.twoUpLeftLarge.rightCornerRadius` | `scaled(20)` | 1983 |
| `J.videoWidgets.rects.editMode.twoUpEqual.left` | `new w.r(scaled(80),scaled(130),scaled(864),scaled(486))` | 2014 |
| `J.videoWidgets.rects.editMode.twoUpEqual.leftCornerRadius` | `scaled(24)` | 2095 |
| `J.videoWidgets.rects.editMode.twoUpEqual.right` | `new w.r(scaled(976),scaled(130),scaled(864),scaled(486))` | 2114 |
| `J.videoWidgets.rects.editMode.twoUpEqual.rightCornerRadius` | `scaled(24)` | 2197 |
| `J.videoWidgets.rects.editMode.twoUpRightLarge.left` | `new w.r(scaled(308),scaled(260),scaled(400),scaled(225))` | 2233 |
| `J.videoWidgets.rects.editMode.twoUpRightLarge.leftCornerRadius` | `scaled(20)` | 2315 |
| `J.videoWidgets.rects.editMode.twoUpRightLarge.right` | `new w.r(scaled(748),scaled(130),scaled(864),scaled(486))` | 2334 |
| `J.videoWidgets.rects.editMode.twoUpRightLarge.rightCornerRadius` | `scaled(24)` | 2417 |
| `J.videoWidgets.rects.watchMode.twoUpLeftLarge.left` | `new w.r(scaled(60),scaled(193),scaled(1232),scaled(693))` | 2464 |
| `J.videoWidgets.rects.watchMode.twoUpLeftLarge.leftCornerRadius` | `scaled(34)` | 2546 |
| `J.videoWidgets.rects.watchMode.twoUpLeftLarge.right` | `new w.r(scaled(1332),scaled(391),scaled(528),scaled(297))` | 2565 |
| `J.videoWidgets.rects.watchMode.twoUpLeftLarge.rightCornerRadius` | `scaled(24)` | 2649 |
| `J.videoWidgets.rects.watchMode.twoUpEqual.left` | `new w.r(scaled(60),scaled(292),scaled(880),scaled(495))` | 2680 |
| `J.videoWidgets.rects.watchMode.twoUpEqual.leftCornerRadius` | `scaled(24)` | 2761 |
| `J.videoWidgets.rects.watchMode.twoUpEqual.right` | `new w.r(scaled(980),scaled(292),scaled(880),scaled(495))` | 2780 |
| `J.videoWidgets.rects.watchMode.twoUpEqual.rightCornerRadius` | `scaled(24)` | 2863 |
| `J.videoWidgets.rects.watchMode.twoUpRightLarge.left` | `new w.r(scaled(60),scaled(391),scaled(528),scaled(297))` | 2899 |
| `J.videoWidgets.rects.watchMode.twoUpRightLarge.leftCornerRadius` | `scaled(24)` | 2980 |
| `J.videoWidgets.rects.watchMode.twoUpRightLarge.right` | `new w.r(scaled(628),scaled(193),scaled(1232),scaled(693))` | 2999 |
| `J.videoWidgets.rects.watchMode.twoUpRightLarge.rightCornerRadius` | `scaled(34)` | 3083 |
| `J.videoWidgets.shadow.color` | `e=>P.Q1.colorWithAlpha(P.Q1.Black,.1+.3*e)` | 3113 |
| `J.videoWidgets.shadow.blurRadius` | `e=>scaled(12)+scaled(13)*e` | 3167 |
| `J.videoWidgets.shadow.offset` | `e=>[0,scaled(7)+scaled(13)*e]` | 3205 |
| `J.videoWidgets.outline.default.width` | `scaled(1)` | 3264 |
| `J.videoWidgets.outline.default.color` | `P.Q1.borderColor()` | 3282 |
| `J.videoWidgets.outline.highlight.width` | `scaled(2)` | 3319 |
| `J.videoWidgets.outline.highlight.color` | `P.Q1.White` | 3337 |
| `J.videoWidgets.audioIndicator.icon` | `x._V.symbol(_.A,21,P.Q1.White).getImage()` | 3371 |
| `J.videoWidgets.audioIndicator.size` | `scaled(36)` | 3418 |
| `J.videoWidgets.audioIndicator.padding.trailing` | `scaled(13)` | 3449 |
| `J.videoWidgets.audioIndicator.padding.bottom` | `scaled(7)` | 3469 |
| `J.addMoreShelf.offset` | `scaled(710)` | 3505 |
| `J.addMoreShelf.exitTranslate` | `scaled(200)` | 3533 |
| `J.transitions.fullscreenDuration` | `300` | 3580 |
| `J.transitions.layoutChangeDuration` | `200*!F.O.accessibility.getReduceMotion()` | 3605 |
| `J.transitions.shelfFadeInDuration` | `240` | 3666 |
| `J.transitions.shelfExitDuration` | `240` | 3688 |
| `J.transitions.buttonFadeDuration` | `100` | 3711 |
| `J.transitions.audioIconFade` | `150` | 3729 |
| `J.layoutSwitcher.largeLeftSplitIcon` | `e=>x._V.symbol(N,32,e).getImage()` | 3769 |
| `J.layoutSwitcher.largeRightSplitIcon` | `e=>x._V.symbol(Z,32,e).getImage()` | 3823 |
| `J.layoutSwitcher.evenSplitIcon` | `e=>x._V.symbol(Q.A,32,e).getImage()` | 3871 |
| `J.layoutSwitcher.spacing` | `scaled(20)` | 3915 |
| `J.layoutSwitcher.rect` | `new w.r(scaled(886),scaled(654),scaled(144),scaled(64))` | 3933 |
| `J.idle.opacity` | `.75` | 4012 |
| `J.idle.timeout` | `1e4` | 4024 |
| `J.idle.duration` | `500` | 4037 |

#### B. MultiViewPage.js — component style `j`

Factory `66912`; source declaration `j`; object begins at byte **4098**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `j.(spread)` | `O.x.D` | 4102 |
| `j.shelfHeight` | `$+X.H.SingleLineTitleHeight+scaled(17)` | 4120 |
| `j.shelfWidth` | `G.nl.width` | 4172 |
| `j.shelfItemHeight` | `$` | 4199 |
| `j.cornerRadius` | `O.x.CornerRadius.M` | 4214 |
| `j.topPadding` | `K.A.TopPadding.Default` | 4244 |
| `j.bottomPadding` | `K.A.BottomPadding.Default` | 4281 |

#### B. MultiViewPage.js — component style `Y`

Factory `66912`; source declaration `Y`; object begins at byte **4310**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `Y.(spread)` | `H.jM.Regular.Platterless()` | 4314 |
| `Y.enabled.(spread)` | `H.jM.Regular.Platterless().enabled` | 4353 |
| `Y.enabled.Idle.(spread)` | `H.jM.Regular.Platterless().enabled.Idle` | 4397 |
| `Y.enabled.Idle.title.format.typography` | `z.o.Emphasized.Body` | 4471 |
| `Y.enabled.Idle.title.format.color` | `P.Q1.LabelSecondary` | 4497 |

#### B. MultiViewPage.js — component style `ee`

Factory `66912`; source declaration `ee`; object begins at byte **4526**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ee.(spread)` | `Y` | 4530 |
| `ee.enabled.(spread)` | `Y.enabled` | 4544 |
| `ee.enabled.Idle.(spread)` | `Y.enabled.Idle` | 4563 |
| `ee.enabled.Idle.title.format.typography` | `z.o.Emphasized.Body` | 4612 |
| `ee.enabled.Idle.title.format.color` | `P.Q1.LabelPrimary` | 4638 |

#### B. ProductPage.js — FooterAlertLayout

Factory `86794`; source declaration `ew`; object begins at byte **9052**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ew.titleFormat.typography` | `et.o.Emphasized.Callout` | 9086 |
| `ew.titleFormat.color` | `D.Q1.LabelPrimary` | 9116 |
| `ew.titleFormat.textAlignment` | `0` | 9148 |
| `ew.itemTitleFormat.typography` | `et.o.Emphasized.Caption1` | 9189 |
| `ew.itemTitleFormat.color` | `D.Q1.LabelPrimary` | 9220 |
| `ew.itemTitleFormat.textAlignment` | `0` | 9252 |
| `ew.itemValueFormat.typography` | `et.o.Standard.Caption1` | 9293 |
| `ew.itemValueFormat.color` | `D.Q1.LabelSecondary` | 9322 |
| `ew.itemValueFormat.textAlignment` | `0` | 9356 |
| `ew.layout.maxHeight` | `scaled(920)` | 9378 |
| `ew.layout.spacing` | `scaled(32)` | 9401 |
| `ew.title.spacing.bottom` | `scaled(32)` | 9439 |
| `ew.item.spacing.bottom` | `scaled(2)` | 9477 |
| `ew.section.spacing.bottom` | `scaled(24)` | 9517 |
| `ew.accessibilityImage.height` | `scaled(20)` | 9560 |
| `ew.accessibilityImage.padding.trailing` | `scaled(8)` | 9592 |
| `ew.columns.spacing` | `scaled(40)` | 9624 |
| `ew.columns.width` | `scaled(380)` | 9644 |

#### B. ProductPage.js — GroupShelfWidget rail geometry

Factory `86794`; source declaration `eF`; object begins at byte **17083**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eF.verticalDistance` | `scaled(11)` | 17101 |
| `eF.spacingWithMultipleGroups` | `scaled(35)` | 17141 |
| `eF.spacingWithSingleGroup` | `scaled(18)` | 17178 |
| `eF.multipleGroupsRowHeight` | `scaled(54)` | 17216 |
| `eF.singleGroupRowHeight` | `scaled(71)` | 17251 |
| `eF.groupRowSpacing` | `scaled(20)` | 17281 |

#### B. ProductPage.js — GroupShelfWidget season/group pill

Factory `86794`; source declaration `eI`; object begins at byte **17299**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eI.height` | `scaled(66)` | 17307 |
| `eI.cornerRadius` | `scaled(33)` | 17334 |
| `eI.gotFocusAnimationDuration` | `100` | 17374 |
| `eI.lostFocusAnimationDuration` | `200` | 17405 |
| `eI.horizontalExpand` | `scaled(8)` | 17426 |
| `eI.enabled.Idle.padding.leading` | `scaled(30)` | 17471 |
| `eI.enabled.Idle.padding.trailing` | `scaled(30)` | 17494 |
| `eI.enabled.Idle.title.format.typography` | `et.o.Emphasized.Callout` | 17544 |
| `eI.enabled.Idle.title.format.color` | `D.Q1.LabelSecondary` | 17574 |
| `eI.enabled.Idle.title.hoverTextColor` | `D.Q1.LabelPrimary` | 17611 |
| `eI.enabled.Focused.background.color` | `new D.Q1(D.Q1.White)` | 17658 |
| `eI.enabled.Focused.padding.leading` | `scaled(30)` | 17697 |
| `eI.enabled.Focused.padding.trailing` | `scaled(30)` | 17720 |
| `eI.enabled.Focused.title.format.typography` | `et.o.Emphasized.Callout` | 17770 |
| `eI.enabled.Focused.title.format.color` | `D.Q1.Black` | 17800 |
| `eI.enabled.FocusCandidate.padding.leading` | `scaled(30)` | 17848 |
| `eI.enabled.FocusCandidate.padding.trailing` | `scaled(30)` | 17871 |
| `eI.enabled.FocusCandidate.background.color` | `new D.Q1(D.Q1.White10)` | 17904 |
| `eI.enabled.FocusCandidate.title.format.typography` | `et.o.Emphasized.Callout` | 17963 |
| `eI.enabled.FocusCandidate.title.format.color` | `D.Q1.LabelPrimary` | 17993 |
| `eI.disabled.Idle.padding.leading` | `scaled(30)` | 18049 |
| `eI.disabled.Idle.padding.trailing` | `scaled(30)` | 18072 |
| `eI.disabled.Idle.title.format.typography` | `et.o.Emphasized.Callout` | 18122 |
| `eI.disabled.Idle.title.format.color` | `D.Q1.LabelSecondary` | 18152 |
| `eI.disabled.Focused.padding.leading` | `scaled(30)` | 18202 |
| `eI.disabled.Focused.padding.trailing` | `scaled(30)` | 18225 |
| `eI.disabled.Focused.title.format.typography` | `et.o.Emphasized.Callout` | 18275 |
| `eI.disabled.Focused.title.format.color` | `D.Q1.LabelSecondary` | 18305 |
| `eI.disabled.FocusCandidate.padding.leading` | `scaled(30)` | 18362 |
| `eI.disabled.FocusCandidate.padding.trailing` | `scaled(30)` | 18385 |
| `eI.disabled.FocusCandidate.title.format.typography` | `et.o.Emphasized.Callout` | 18435 |
| `eI.disabled.FocusCandidate.title.format.color` | `D.Q1.LabelSecondary` | 18465 |

#### B. ProductPage.js — AboutAlertLayout

Factory `86794`; source declaration `eM`; object begins at byte **25259**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eM.titleFormat.typography` | `et.o.Emphasized.Callout` | 25293 |
| `eM.titleFormat.color` | `D.Q1.LabelPrimary` | 25323 |
| `eM.subtitleFormat.typography` | `et.o.Standard.Caption1` | 25379 |
| `eM.subtitleFormat.color` | `D.Q1.LabelSecondary` | 25408 |
| `eM.bodyFormat.typography` | `et.o.Standard.Callout` | 25462 |
| `eM.bodyFormat.color` | `D.Q1.LabelSecondary` | 25490 |
| `eM.layout.spacing` | `0` | 25528 |
| `eM.title.padding.top` | `scaled(72)` | 25551 |
| `eM.title.padding.leading` | `scaled(80)` | 25573 |
| `eM.title.padding.trailing` | `scaled(80)` | 25596 |
| `eM.subtitle.padding.top` | `scaled(0)` | 25635 |
| `eM.subtitle.padding.leading` | `scaled(80)` | 25656 |
| `eM.subtitle.padding.trailing` | `scaled(80)` | 25679 |
| `eM.body.padding.top` | `scaled(32)` | 25714 |
| `eM.body.padding.leading` | `scaled(80)` | 25736 |
| `eM.body.padding.trailing` | `scaled(80)` | 25759 |

#### B. ProductPage.js — RottenTomatoesAlertLayout

Factory `86794`; source declaration `eN`; object begins at byte **27445**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eN.titleFormat.typography` | `et.o.Emphasized.Callout` | 27479 |
| `eN.titleFormat.color` | `D.Q1.LabelPrimary` | 27509 |
| `eN.titleFormat.textAlignment` | `0` | 27541 |
| `eN.bodyFormat.typography` | `et.o.Standard.Callout` | 27577 |
| `eN.bodyFormat.color` | `D.Q1.LabelSecondary` | 27605 |
| `eN.bodyFormat.textAlignment` | `0` | 27639 |
| `eN.infoFormat.typography` | `et.o.Standard.Caption1` | 27675 |
| `eN.infoFormat.color` | `D.Q1.LabelSecondary` | 27704 |
| `eN.infoFormat.textAlignment` | `0` | 27738 |
| `eN.layout.padding.trailing` | `scaled(80)` | 27768 |
| `eN.layout.padding.leading` | `scaled(80)` | 27790 |
| `eN.layout.padding.top` | `scaled(72)` | 27808 |
| `eN.layout.padding.bottom` | `scaled(68)` | 27829 |
| `eN.title.padding.bottom` | `scaled(32)` | 27868 |
| `eN.title.padding.top` | `scaled(72)` | 27886 |
| `eN.title.padding.trailing` | `scaled(80)` | 27909 |
| `eN.oneLiner.padding.bottom` | `scaled(25)` | 27951 |
| `eN.oneLiner.padding.trailing` | `scaled(80)` | 27974 |
| `eN.oneLiner.padding.leading` | `scaled(80)` | 27996 |
| `eN.logo.size` | `37` | 28023 |
| `eN.logo.padding.trailing` | `scaled(9)` | 28044 |
| `eN.logo.padding.leading` | `scaled(80)` | 28065 |
| `eN.entry.labelWidth` | `scaled(150)` | 28099 |
| `eN.entry.labelValueSpacing` | `scaled(40)` | 28132 |
| `eN.entry.padding.bottom` | `scaled(13)` | 28162 |
| `eN.entry.padding.trailing` | `scaled(80)` | 28185 |
| `eN.entry.padding.leading` | `scaled(80)` | 28207 |

#### B. ProductPage.js — CommonSenseMediaAlertLayout

Factory `86794`; source declaration `eU`; object begins at byte **32002**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eU.titleFormat.typography` | `et.o.Emphasized.Callout` | 32036 |
| `eU.titleFormat.color` | `D.Q1.LabelPrimary` | 32066 |
| `eU.titleFormat.textAlignment` | `0` | 32098 |
| `eU.bodyFormat.typography` | `et.o.Standard.Callout` | 32134 |
| `eU.bodyFormat.color` | `D.Q1.LabelSecondary` | 32162 |
| `eU.bodyFormat.textAlignment` | `0` | 32196 |
| `eU.infoFormat.typography` | `et.o.Standard.Caption1` | 32232 |
| `eU.infoFormat.color` | `D.Q1.LabelSecondary` | 32261 |
| `eU.infoFormat.textAlignment` | `0` | 32295 |
| `eU.layout.padding.trailing` | `scaled(80)` | 32325 |
| `eU.layout.padding.leading` | `scaled(80)` | 32347 |
| `eU.layout.padding.top` | `scaled(72)` | 32365 |
| `eU.layout.padding.bottom` | `scaled(68)` | 32386 |
| `eU.title.padding.bottom` | `scaled(32)` | 32425 |
| `eU.title.padding.top` | `scaled(72)` | 32443 |
| `eU.title.padding.trailing` | `scaled(80)` | 32466 |
| `eU.title.padding.leading` | `scaled(80)` | 32488 |
| `eU.oneLiner.padding.bottom` | `scaled(25)` | 32530 |
| `eU.oneLiner.padding.leading` | `scaled(80)` | 32552 |
| `eU.oneLiner.padding.trailing` | `scaled(80)` | 32575 |
| `eU.badge.image` | `(e,t=new D.Q1(D.Q1.LabelPrimary))=>ev.R.commonSense(e,t.getColor())` | 32604 |
| `eU.badge.size` | `37` | 32677 |
| `eU.badge.width` | `scaled(37)` | 32686 |
| `eU.badge.height` | `scaled(32)` | 32707 |
| `eU.badge.padding.trailing` | `scaled(9)` | 32739 |
| `eU.rating.maxDots` | `5` | 32770 |
| `eU.rating.dotSize` | `scaled(16)` | 32780 |
| `eU.rating.dotSpacing` | `scaled(8)` | 32805 |
| `eU.rating.dotActiveAlpha` | `1` | 32833 |
| `eU.rating.dotInactiveAlpha` | `.2` | 32852 |
| `eU.rating.dotPadding.bottom` | `scaled(8)` | 32874 |
| `eU.rating.linePadding.bottom` | `scaled(13)` | 32908 |
| `eU.rating.linePadding.leading` | `scaled(80)` | 32930 |
| `eU.rating.linePadding.trailing` | `scaled(80)` | 32953 |
| `eU.rating.textPadding.trailing` | `scaled(40)` | 32990 |

#### B. ProductPage.js — DescriptionLockup container

Factory `86794`; source declaration `ej`; object begins at byte **37454**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ej.width` | `scaled(560)` | 37461 |
| `ej.height` | `scaled(292)` | 37483 |
| `ej.isOpaque` | `!1` | 37507 |
| `ej.outlineColor` | `void 0` | 37523 |
| `ej.shadow` | `eY.j.shadow` | 37537 |
| `ej.cornerRadius` | `scaled(30)` | 37562 |
| `ej.pressedStyle` | `eQ.e.Deflate` | 37589 |

#### B. ProductPage.js — DescriptionLockup content

Factory `86794`; source declaration `e$`; object begins at byte **37606**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `e$.padding.leading` | `scaled(22)` | 37624 |
| `e$.padding.trailing` | `scaled(22)` | 37647 |
| `e$.padding.top` | `scaled(18)` | 37665 |
| `e$.padding.bottom` | `scaled(20)` | 37686 |
| `e$.backgroundColor.focused` | `new(w()).Gfx.Color(1,1,1,.4)` | 37726 |
| `e$.backgroundColor.unfocused` | `new(w()).Gfx.Color(1,1,1,.2)` | 37765 |
| `e$.fonts.title.typography` | `et.o.Emphasized.Callout` | 37829 |
| `e$.fonts.title.color` | `D.Q1.LabelPrimary` | 37859 |
| `e$.fonts.genre.typography` | `et.o.Standard.Caption2` | 37906 |
| `e$.fonts.genre.color` | `D.Q1.LabelSecondary` | 37935 |
| `e$.fonts.body.typography` | `et.o.Standard.Caption2` | 37983 |
| `e$.fonts.body.color` | `D.Q1.LabelPrimary` | 38012 |
| `e$.fonts.ellipsis.typography` | `et.o.Emphasized.Caption2` | 38062 |
| `e$.fonts.ellipsis.color` | `D.Q1.LabelPrimary` | 38093 |
| `e$.titleSubtitleSpacing` | `scaled(28)` | 38135 |
| `e$.heightsForLines.5` | `scaled(150)` | 38168 |
| `e$.heightsForLines.6` | `scaled(180)` | 38185 |

#### B. ProductPage.js — AboutCardLockup container

Factory `86794`; source declaration `eq`; object begins at byte **40204**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eq.width` | `scaled(260)` | 40211 |
| `eq.height` | `scaled(292)` | 40233 |
| `eq.maxWidth` | `scaled(560)` | 40257 |
| `eq.isOpaque` | `!1` | 40281 |
| `eq.outlineColor` | `void 0` | 40297 |
| `eq.shadow` | `eY.j.shadow` | 40311 |
| `eq.cornerRadius` | `scaled(30)` | 40336 |
| `eq.pressedStyle` | `eQ.e.Deflate` | 40363 |

#### B. ProductPage.js — AboutCardLockup content

Factory `86794`; source declaration `eJ`; object begins at byte **40380**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eJ.padding` | `scaled(22)` | 40389 |
| `eJ.backgroundColor.focused` | `new ef.yP(1,1,1,.4)` | 40428 |
| `eJ.backgroundColor.unfocused` | `new ef.yP(1,1,1,.2)` | 40458 |
| `eJ.fonts.title.typography` | `et.o.Emphasized.Title1` | 40513 |
| `eJ.fonts.title.color` | `D.Q1.LabelPrimary` | 40542 |
| `eJ.fonts.titleSuper.typography` | `et.o.Standard.Headline` | 40594 |
| `eJ.fonts.titleSuper.color` | `D.Q1.LabelPrimary` | 40623 |
| `eJ.fonts.subtitle.typography` | `et.o.Emphasized.Caption1` | 40673 |
| `eJ.fonts.subtitle.color` | `D.Q1.LabelPrimary` | 40704 |
| `eJ.fonts.body.typography` | `et.o.Standard.Caption2` | 40750 |
| `eJ.fonts.body.color` | `D.Q1.LabelPrimary` | 40779 |
| `eJ.baseline.title` | `scaled(78)` | 40816 |
| `eJ.baseline.titleSuper` | `scaled(46)` | 40841 |
| `eJ.baseline.source` | `scaled(48)` | 40862 |
| `eJ.bottomPadding` | `scaled(28)` | 40891 |

#### B. ProductPage.js — CommonSenseLockup body

Factory `86794`; source declaration `eZ`; object begins at byte **41350**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eZ.descriptionFormat.typography` | `et.o.Standard.Caption2` | 41390 |
| `eZ.descriptionFormat.color` | `D.Q1.LabelPrimary` | 41419 |
| `eZ.imageSize` | `21` | 41449 |

#### B. ProductPage.js — RottenTomatoesLockup override

Factory `86794`; source declaration `e1`; object begins at byte **43405**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `e1.(spread)` | `eJ` | 43409 |
| `e1.fonts.(spread)` | `eJ.fonts` | 43422 |
| `e1.fonts.titleSuper.typography` | `et.o.Emphasized.Caption1` | 43463 |
| `e1.fonts.titleSuper.color` | `D.Q1.LabelPrimary` | 43494 |
| `e1.scoreLabelFormat.typography` | `et.o.Standard.Caption2` | 43553 |
| `e1.scoreLabelFormat.color` | `D.Q1.LabelPrimary` | 43582 |
| `e1.scoreValueFormat.typography` | `et.o.Standard.Caption2` | 43640 |
| `e1.scoreValueFormat.color` | `D.Q1.LabelPrimary` | 43669 |
| `e1.scoreValueFormat.textAlignment` | `1` | 43701 |

#### B. ProductPage.js — CastLockup

Factory `86794`; source declaration `tD`; object begins at byte **59361**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `tD.width` | `scaled(410)` | 59368 |
| `tD.title.typography` | `et.o.Standard.Caption1` | 59410 |
| `tD.title.color` | `D.Q1.LabelTertiary_RespectTransparencySetting` | 59439 |
| `tD.names.typography` | `et.o.Standard.Caption1` | 59514 |
| `tD.names.color` | `D.Q1.LabelSecondary_RespectTransparencySetting` | 59543 |
| `tD.axCursor.padding` | `-scaled(12)` | 59610 |
| `tD.axCursor.cornerRadius` | `scaled(12)` | 59638 |

#### B. ProductPage.js — CanonicalLockup

Factory `86794`; source declaration `tE`; object begins at byte **61087**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `tE.anchorHeight` | `eg.nl.height-scaled(162)` | 61101 |
| `tE.buttonRowHeight` | `scaled(66)` | 61145 |
| `tE.spacings.metadata` | `scaled(18)` | 61178 |
| `tE.spacings.buttonRow` | `scaled(32)` | 61202 |
| `tE.spacings.buttons` | `scaled(24)` | 61224 |
| `tE.spacings.features` | `scaled(10)` | 61247 |

#### B. ProductPage.js — CanonicalSportsLockup

Factory `86794`; source declaration `t_`; object begins at byte **76478**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `t_.anchorHeight` | `eg.nl.height-scaled(162)` | 76492 |
| `t_.spacings.metadata` | `scaled(18)` | 76539 |
| `t_.spacings.button` | `scaled(32)` | 76560 |

#### B. SearchPageFactory.js — SearchHintBarWidget

Factory `58454`; source declaration `f`; object begins at byte **259**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `f.button.maxWidth` | `scaled(500)` | 277 |
| `f.spacing` | `scaled(20)` | 300 |
| `f.padding.bottom` | `scaled(44)` | 329 |
| `f.animation.damping` | `33` | 362 |
| `f.animation.stiffness` | `280` | 375 |
| `f.animation.mass` | `1` | 384 |
| `f.animation.minimumLinearVelocity` | `.001` | 408 |

#### B. SearchPageFactory.js — Search input style

Factory `58454`; source declaration `X`; object begins at byte **4775**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `X.height` | `scaled(85)` | 4783 |
| `X.highlightColor` | `b.Q1.White10` | 4811 |
| `X.bgColor` | `b.Q1.White0` | 4832 |
| `X.padding.leading` | `scaled(20)` | 4861 |
| `X.padding.trailing` | `scaled(20)` | 4883 |
| `X.padding.bottom` | `0` | 4903 |
| `X.padding.top` | `0` | 4909 |
| `X.mask.gradientWidth` | `scaled(30)` | 4932 |
| `X.search.indentation` | `scaled(20)` | 4966 |
| `X.format` | `K` | 5616 |
| `X.infoFormat` | `_` | 5629 |
| `X.passwordFormat` | `K` | 5646 |

#### B. SearchPageFactory.js — Search keyboard spring

Factory `58454`; source declaration `j`; object begins at byte **5651**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `j.type` | `y.Y5.Spring` | 5657 |
| `j.mass` | `1` | 5674 |
| `j.damping` | `22` | 5684 |
| `j.stiffness` | `150` | 5697 |

#### B. SearchPageFactory.js — SearchPage geometry and transitions

Factory `58454`; source declaration `N`; object begins at byte **5704**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `N.input.style` | `X` | 5718 |
| `N.input.y` | `scaled(139)` | 5722 |
| `N.input.nonHighlightedSearchTermFormat.typography.(spread)` | `T.o.Emphasized.Title1` | 5791 |
| `N.input.nonHighlightedSearchTermFormat.typography.leading` | `40` | 5821 |
| `N.input.nonHighlightedSearchTermFormat.color` | `b.Q1.LabelTertiary` | 5831 |
| `N.input.nonHighlightedSearchTermFormat.pageAlignment` | `1` | 5864 |
| `N.content.y` | `scaled(251)` | 5880 |
| `N.content.searchHintsBar.clipRect.x` | `-scaled(80)` | 5922 |
| `N.content.searchHintsBar.clipRect.y` | `-scaled(20)` | 5938 |
| `N.content.searchHintsBar.clipRect.w` | `scaled(1e4)` | 5954 |
| `N.content.searchHintsBar.clipRect.h` | `scaled(1e4)` | 5970 |
| `N.content.searchHintsBar.maskRect.x` | `scaled(0)` | 5997 |
| `N.content.searchHintsBar.maskRect.y` | `-scaled(10)` | 6011 |
| `N.content.searchHintsBar.maskRect.w` | `scaled(50)` | 6027 |
| `N.content.searchHintsBar.maskRect.h` | `scaled(80)` | 6042 |
| `N.keyboard.width` | `Q` | 6074 |
| `N.searchTermDescriptionWidget.spacingToHairline` | `scaled(50)` | 6124 |
| `N.hairline.height` | `scaled(1)` | 6155 |
| `N.hairline.spacingToContent` | `scaled(10)` | 6184 |
| `N.hairline.color` | `b.Q1.LabelTertiary()` | 6203 |
| `N.keyboardAnimation.offset` | `S.nl.SafeArea.horizontal+Q` | 6251 |
| `N.keyboardAnimation.animParams.(spread)` | `j` | 6293 |
| `N.deckTransition.separation` | `1.25` | 6324 |
| `N.deckTransition.duration` | `1e3` | 6338 |
| `N.deckTransition.fadeBottom` | `!0` | 6353 |
| `N.accessibility.suggestionTime` | `1500` | 6387 |
| `N.firstRowShift` | `scaled(105)` | 6407 |

#### B. SearchPageFactory.js — AISafetyUILockup

Factory `58454`; source declaration `ev`; object begins at byte **7941**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ev.message.format.typography` | `T.o.Tall.Body` | 7978 |
| `ev.message.format.color` | `b.Q1.LabelPrimary` | 7998 |
| `ev.message.format.paragraphSpacingAfter` | `0` | 8038 |
| `ev.message.format.textAlignment` | `.5` | 8054 |
| `ev.message.format.pageAlignment` | `.5` | 8071 |
| `ev.message.maxLines` | `3` | 8085 |
| `ev.padding` | `scaled(50)` | 8096 |
| `ev.background.idleColor` | `new eb.yP(1,1,1,.1)` | 8131 |
| `ev.background.focusedColor` | `new eb.yP(1,1,1,.2)` | 8164 |

#### B. SearchPageFactory.js — SearchContentWidget segment platter and empty state

Factory `58454`; source declaration `ex`; object begins at byte **8800**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ex.searchHintsBar.bottomBuffer` | `eD` | 8830 |
| `ex.searchResultsSegmentPlatter.pillHeight` | `eW` | 8874 |
| `ex.searchResultsSegmentPlatter.pillAutoHeight` | `eW+eD` | 8892 |
| `ex.searchResultsSegmentPlatter.cornerRadius` | `Math.ceil(eW/2)` | 8911 |
| `ex.searchResultsSegmentPlatter.padding.vertical` | `eT` | 8945 |
| `ex.searchResultsSegmentPlatter.padding.horizontal` | `scaled(9)` | 8959 |
| `ex.searchResultsSegmentPlatter.dividerGap` | `scaled(32)` | 8983 |
| `ex.searchResultsSegmentPlatter.buttonSpacing` | `scaled(8)` | 9010 |
| `ex.searchResultsSegmentPlatter.maxWidth` | `scaled(1260)` | 9031 |
| `ex.searchResultsSegmentPlatter.bottomBuffer` | `eR` | 9059 |
| `ex.searchResultsSegmentPlatter.fade.transparentWidth` | `scaled(30)` | 9085 |
| `ex.searchResultsSegmentPlatter.fade.gradientWidth` | `scaled(60)` | 9112 |
| `ex.searchResultsSegmentPlatter.hairlineAlignment` | `eW/2/(eW+eR)` | 9144 |
| `ex.searchResultsSegmentPlatter.backgroundColor` | `()=>z.O.accessibility.focusStyle?b.Q1.SystemGray5_HighContrast():b.Q1.SystemGray5()` | 9173 |
| `ex.searchResultsSegmentPlatter.shadow.color` | `b.Q1.Black` | 9271 |
| `ex.searchResultsSegmentPlatter.shadow.opacity` | `()=>z.O.accessibility.focusStyle?.2:.4` | 9290 |
| `ex.searchResultsSegmentPlatter.shadow.offset` | `new h.Math.Point2(0,scaled(4))` | 9336 |
| `ex.searchResultsSegmentPlatter.shadow.blurRadius` | `scaled(48)` | 9380 |
| `ex.searchResultsSegmentPlatter.border.color` | `()=>z.O.accessibility.focusStyle?b.Q1.borderColorHighContrast():b.Q1.borderColor()` | 9408 |
| `ex.missingContentWidget.UpperSpacing` | `scaled(-7)` | 9528 |
| `ex.missingContentWidget.LowerSpacing` | `scaled(15)` | 9554 |
| `ex.missingContentWidget.HeadlineAlignment` | `.5` | 9585 |
| `ex.missingContentWidget.Headline.typography` | `T.o.Emphasized.Title3` | 9617 |
| `ex.missingContentWidget.Headline.color` | `b.Q1.LabelPrimary` | 9645 |
| `ex.missingContentWidget.body.typography` | `T.o.Standard.Body` | 9690 |
| `ex.missingContentWidget.body.color` | `b.Q1.LabelSecondary` | 9714 |
| `ex.aISafetyUI.bottomPadding` | `scaled(-5)` | 9763 |
| `ex.aISafetyUI.spacing` | `scaled(64)` | 9784 |

#### B. Settings.js — component style `td`

Factory `10986`; source declaration `td`; object begins at byte **50157**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `td.measure.leadingContentWidth.default` | `scaled(800)` | 50196 |
| `td.measure.leadingContentWidth.max` | `scaled(800)` | 50214 |
| `td.title.format.typography` | `V.o.Emphasized.Headline` | 50264 |
| `td.title.format.color` | `ta.Q1.LabelPrimary` | 50294 |
| `td.title.format.textAlignment` | `.5` | 50327 |
| `td.subtitle.format.typography` | `V.o.Standard.Callout` | 50370 |
| `td.subtitle.format.color` | `ta.Q1.LabelSecondary` | 50397 |
| `td.subtitle.format.textAlignment` | `.5` | 50432 |
| `td.avatar.size` | `scaled(150)` | 50451 |
| `td.spacing.leadingMargin` | `scaled(190)` | 50489 |
| `td.spacing.trailingMargin.default` | `scaled(190)` | 50527 |
| `td.spacing.trailingMargin.max` | `scaled(80)` | 50545 |
| `td.spacing.avatarsToTitleBaseline` | `scaled(92)` | 50582 |
| `td.spacing.titleToSubtitleBaseline` | `scaled(64)` | 50619 |

#### B. SettingsPage.js — component style `D`

Factory `18203`; source declaration `D`; object begins at byte **385**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `D.titleRect` | `new S.r(C.nl.SafeArea.horizontal,C.nl.SafeArea.vertical,C.nl.SafeArea.width,scaled(97))` | 396 |
| `D.titlePadding.top` | `scaled(10)` | 504 |
| `D.listRect` | `new S.r(scaled(1080),scaled(190),scaled(760),scaled(890))` | 527 |
| `D.helpSectionRect` | `new S.r(scaled(80),scaled(191),scaled(900),scaled(837))` | 609 |
| `D.listWideRect` | `new S.r(scaled(160),scaled(190),scaled(1600),scaled(890))` | 686 |
| `D.helpRect` | `new S.r(0,0,scaled(900),scaled(180))` | 761 |
| `D.helpRectSpacing` | `scaled(5)` | 818 |
| `D.titleFormat.typography` | `x.o.Emphasized.Title2` | 862 |
| `D.titleFormat.color` | `I.Q1.LabelSecondary` | 890 |
| `D.titleFormat.textAlignment` | `.5` | 924 |
| `D.headlineFormat.typography` | `x.o.Standard.Body` | 964 |
| `D.headlineFormat.color` | `I.Q1.White40` | 988 |
| `D.headlineFormat.leftMargin` | `scaled(20)` | 1012 |
| `D.inlineTextFormat.typography` | `x.o.Standard.Body` | 1064 |
| `D.inlineTextFormat.textAlignment` | `.5` | 1096 |
| `D.helpFormat.typography` | `x.o.Standard.Body` | 1132 |
| `D.helpFormat.color` | `I.Q1.White40` | 1156 |
| `D.helpFormat.textAlignment` | `.5` | 1183 |
| `D.titleFadeTime` | `250` | 1202 |
| `D.titleTransition` | `()=>new b.M({duration:400,fadeBottom:!0,curve:"easeOut"})` | 1222 |
| `D.helpTextAttributes.autoshrink` | `!1` | 1585 |
| `D.helpTextAttributes.truncation` | `k.q.TruncationStyle.Ellipsis` | 1599 |
| `D.headlineTopPadding` | `scaled(30)` | 1648 |
| `D.headlineBottomPadding` | `scaled(1)` | 1683 |
| `D.inlineTextTopSpacing` | `scaled(30)` | 1716 |
| `D.inlineTextBottomSpacing` | `scaled(1)` | 1753 |
| `D.buttonSpacing` | `scaled(14)` | 1779 |
| `D.listBottomPadding` | `scaled(90)` | 1810 |
| `D.imagePadding` | `scaled(56)` | 1836 |
| `D.listScrollArgs.pointerConfig.enabled` | `!0` | 1888 |
| `D.listScrollArgs.pointerConfig.scrollSpeed` | `scaled(132)` | 1903 |
| `D.listScrollArgs.top` | `scaled(200)` | 1922 |
| `D.listScrollArgs.bottom` | `scaled(150)` | 1943 |
| `D.listFadeArgs.top.inner` | `scaled(90)` | 1983 |
| `D.listFadeArgs.top.ellipsis` | `!0` | 2005 |
| `D.listFadeArgs.top.fadeWithScroll` | `scaled(20)` | 2023 |
| `D.listFadeArgs.top.clipExpand` | `scaled(90)` | 2047 |
| `D.listFadeArgs.top.orthogonalMargin` | `scaled(50)` | 2077 |
| `D.listFadeArgs.bottom.inner` | `scaled(90)` | 2105 |
| `D.listFadeArgs.bottom.ellipsis` | `!0` | 2127 |
| `D.listFadeArgs.bottom.orthogonalMargin` | `scaled(50)` | 2147 |
| `D.buttonSpinnerStyle.color` | `I.Q1.Black50` | 2188 |
| `D.buttonSpinnerStyle.size` | `F.Q.Size.Small` | 2206 |

#### B. SportsPostPlayPage.js — component style `m`

Factory `71412`; source declaration `m`; object begins at byte **306**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `m.subtitlesBottomPadding` | `scaled(300)` | 330 |
| `m.gradient.height` | `scaled(556)` | 361 |
| `m.gradient.bottomMargin` | `scaled(60)` | 388 |
| `m.animation.duration` | `400` | 422 |

#### B. SuperBrowsePage.js — SuperBrowseFilterBar

Factory `17380`; source declaration `O`; object begins at byte **760**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `O.paddingTop` | `scaled(30)` | 772 |
| `O.paddingBottom` | `scaled(26)` | 799 |
| `O.buttonSpacing` | `scaled(16)` | 826 |

#### B. SuperBrowsePage.js — LiquidList shared menu

Factory `17380`; source declaration `G`; object begins at byte **905**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `G.menu.spacing` | `scaled(24)` | 920 |
| `G.menu.backgroundColor` | `o.Q1.colorFromRGB(255,255,255,.1)` | 949 |
| `G.menu.panelGradientColor` | `o.Q1.colorFromRGB(67,67,67,1)` | 1002 |
| `G.menu.defaultIconColor` | `o.Q1.colorFromRGB(255,127,0,1)` | 1049 |
| `G.animation.expandedYOffset` | `scaled(-21)` | 1108 |
| `G.animation.springStiffness` | `25` | 1138 |
| `G.animation.springDamping` | `185` | 1155 |
| `G.animation.springMass` | `1` | 1170 |

#### B. SuperBrowsePage.js — ExpandableMenuBase

Factory `17380`; source declaration `J`; object begins at byte **10893**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `J.menu.paddingH` | `scaled(21)` | 10909 |
| `J.menu.paddingV` | `scaled(19)` | 10931 |
| `J.menu.maxExpandedHeight` | `scaled(600)` | 10962 |
| `J.item.height` | `scaled(66)` | 10990 |
| `J.item.padding` | `scaled(4)` | 11011 |
| `J.item.width` | `scaled(408)` | 11029 |
| `J.item.trailingPadding` | `scaled(15)` | 11059 |
| `J.focus.grow` | `scaled(4)` | 11085 |
| `J.pillFocus.shadow.blurRadius` | `scaled(25)` | 11128 |
| `J.pillFocus.shadow.offsetY` | `scaled(20)` | 11149 |
| `J.pillFocus.shadow.opacity` | `.4` | 11170 |
| `J.pill.height` | `scaled(66)` | 11188 |
| `J.pill.horizontalPadding` | `scaled(30)` | 11219 |
| `J.pill.verticalPadding` | `scaled(18)` | 11248 |
| `J.animation.checkmarkFadeThreshold` | `.7` | 11296 |
| `J.animation.checkmarkFadeRange` | `.3` | 11318 |
| `J.animation.spring.stiffness` | `25` | 11339 |
| `J.animation.spring.damping` | `185` | 11350 |
| `J.animation.spring.mass` | `1` | 11359 |
| `J.animation.pressShrink.stiffness` | `300` | 11385 |
| `J.animation.pressShrink.damping` | `50` | 11397 |
| `J.animation.pressShrink.mass` | `1` | 11405 |
| `J.animation.pressBounce.stiffness` | `150` | 11431 |
| `J.animation.pressBounce.damping` | `20` | 11443 |
| `J.animation.pressBounce.mass` | `1` | 11451 |
| `J.gradient.height` | `scaled(62)` | 11472 |
| `J.gradient.expandedCornerRadius` | `scaled(54)` | 11506 |

#### B. SuperBrowsePage.js — LiquidMenu initial geometry

Factory `17380`; source declaration `et`; object begins at byte **11544**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `et.width` | `scaled(100)` | 11551 |
| `et.height` | `scaled(44)` | 11572 |
| `et.paddingHorizontal` | `scaled(16)` | 11603 |
| `et.paddingVertical` | `scaled(8)` | 11632 |

#### B. SuperBrowsePage.js — ChoiceMenuItem

Factory `17380`; source declaration `ea`; object begins at byte **27162**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ea.item.(spread)` | `J.item` | 27172 |
| `ea.item.iconWidth` | `scaled(66)` | 27189 |
| `ea.item.glyphSize` | `22` | 27212 |
| `ea.item.iconOffset` | `scaled(44)` | 27226 |

#### B. SuperBrowsePage.js — SingleChoiceMenu

Factory `17380`; source declaration `er`; object begins at byte **28568**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `er.(spread)` | `J` | 28572 |
| `er.item.(spread)` | `J.item` | 28583 |
| `er.item.iconWidth` | `scaled(66)` | 28600 |
| `er.compact.size` | `scaled(66)` | 28628 |
| `er.compact.padding` | `scaled(12)` | 28649 |
| `er.compact.glyphSize` | `22` | 28672 |
| `er.pill.(spread)` | `J.pill` | 28685 |
| `er.pill.textPadding` | `scaled(60)` | 28704 |

#### B. SuperBrowsePage.js — MultiChoiceMenu

Factory `17380`; source declaration `ec`; object begins at byte **60291**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ec.(spread)` | `J` | 60295 |
| `ec.item.(spread)` | `J.item` | 60306 |
| `ec.item.iconWidth` | `scaled(66)` | 60323 |
| `ec.item.glyphSize` | `22` | 60346 |
| `ec.item.iconOffset` | `scaled(44)` | 60360 |
| `ec.compact.width` | `scaled(66)` | 60389 |
| `ec.compact.height` | `scaled(66)` | 60409 |
| `ec.compact.padding` | `scaled(12)` | 60430 |
| `ec.compact.glyphSize` | `28` | 60453 |
| `ec.pill.(spread)` | `J.pill` | 60466 |
| `ec.pill.textPadding` | `scaled(60)` | 60485 |
| `ec.badge.size` | `scaled(28)` | 60511 |
| `ec.badge.gap` | `scaled(8)` | 60528 |
| `ec.divider.height` | `scaled(29)` | 60557 |
| `ec.divider.lineHeight` | `scaled(1)` | 60581 |
| `ec.divider.paddingH` | `scaled(12)` | 60602 |

#### B. SuperBrowsePage.js — component style `eQ`

Factory `17380`; source declaration `eQ`; object begins at byte **73670**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eQ.item.(spread)` | `J.item` | 73680 |
| `eQ.item.avatarSize` | `scaled(50)` | 73698 |
| `eQ.item.avatarX` | `scaled(52)` | 73719 |
| `eQ.item.checkmarkX` | `scaled(4)` | 73743 |
| `eQ.item.textGap` | `scaled(12)` | 73763 |

#### B. SuperBrowsePage.js — ProfileMenu

Factory `17380`; source declaration `ew`; object begins at byte **75769**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ew.(spread)` | `J` | 75773 |
| `ew.item.(spread)` | `J.item` | 75784 |
| `ew.item.avatarSize` | `scaled(50)` | 75802 |
| `ew.item.avatarX` | `scaled(52)` | 75823 |
| `ew.item.checkmarkX` | `scaled(4)` | 75847 |
| `ew.item.textGap` | `scaled(12)` | 75867 |
| `ew.compact.size` | `scaled(56)` | 75895 |
| `ew.compact.padding` | `scaled(12)` | 75916 |
| `ew.pill.(spread)` | `J.pill` | 75939 |
| `ew.pill.sidePadding` | `scaled(30)` | 75958 |
| `ew.pill.avatarSize` | `scaled(40)` | 75982 |
| `ew.pill.avatarStackOffset` | `scaled(28)` | 76013 |
| `ew.pill.textGap` | `scaled(8)` | 76034 |

#### B. SuperBrowsePage.js — CustomizeIndicatorWidget

Factory `17380`; source declaration `eU`; object begins at byte **97444**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eU.pillHeight` | `scaled(50)` | 97456 |
| `eU.leadingPadding` | `scaled(34)` | 97484 |
| `eU.trailingPadding` | `scaled(32)` | 97513 |
| `eU.glyphTextGap` | `scaled(11)` | 97539 |

#### B. SuperBrowsePage.js — SuperBrowseShelf / SuperBrowseRelatedShelf animation and layout

Factory `38673`; source declaration `a`; object begins at byte **122065**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `a.canvasBodyBottomPadding` | `scaled(45)` | 122090 |
| `a.fadeDelay` | `150` | 122113 |
| `a.fadeDuration` | `200` | 122130 |
| `a.shelfFadeDuration` | `500` | 122152 |
| `a.fadeHeight` | `100` | 122167 |
| `a.headerTopPadding` | `scaled(140)` | 122188 |
| `a.overlayTopPadding` | `scaled(45)` | 122220 |
| `a.focusDebounceMs` | `300` | 122249 |
| `a.overlayButtonAlignment` | `.5` | 122276 |
| `a.shelfTopOffset` | `scaled(70)` | 122294 |
| `a.filterBarDepth` | `1` | 122322 |
| `a.primaryShelfBottomPadding` | `scaled(35)` | 122350 |
| `a.relatedShelfBottomPadding` | `scaled(30)` | 122389 |
| `a.emptyStateBodyBottomOffsetFromCenter` | `scaled(10)` | 122439 |
| `a.titleFadeOutDuration` | `150` | 122473 |
| `a.titleFadeInDuration` | `200` | 122497 |
| `a.titleFadeBezier` | `[.33,0,.67,1]` | 122517 |
| `a.pressActionHintFadeInDuration` | `350` | 122561 |
| `a.pressActionHintFadeInDelay` | `300` | 122592 |
| `a.pressActionHintFadeOutDuration` | `100` | 122627 |
| `a.crossfadeSnap` | `n.X5.cubicBezierEasing(.5135,0,.6683,1.0006)` | 122645 |
| `a.crossfadeSettle` | `n.X5.cubicBezierEasing(.3333,0,.4019,1)` | 122706 |
| `a.crossfadeScaleDownDuration` | `170` | 122773 |
| `a.crossfadeScaleUpDuration` | `150` | 122802 |
| `a.crossfadeSettleDuration` | `430` | 122830 |
| `a.crossfadeScaleDown` | `.98` | 122853 |
| `a.crossfadeScaleUp` | `1.03` | 122874 |
| `a.crossfadeStaggerDelay` | `100` | 122901 |
| `a.glimmerFadeScaleDownDuration` | `230` | 122934 |
| `a.glimmerFadeScaleDown` | `.95` | 122959 |
| `a.bounceScale` | `1.02` | 122975 |
| `a.bounceDuration` | `120` | 122995 |
| `a.bounceBezier` | `[.3718,0,.6321,1.006]` | 123012 |
| `a.bounceSpringStiffness` | `200` | 123056 |
| `a.bounceSpringDamping` | `20` | 123080 |

#### B. TextInput.js — component style `l`

Factory `10197`; source declaration `l`; object begins at byte **13733**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `l.type` | `s.grid` | 13739 |
| `l.RowOffsets.keys` | `scaled(76)` | 13763 |
| `l.RowOffsets.controlKeys` | `scaled(76)` | 13788 |
| `l.Origins.keys` | `[scaled(31),scaled(105),scaled(179),scaled(253),scaled(327),scaled(401)]` | 13816 |
| `l.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 13913 |

#### B. TextInput.js — component style `d`

Factory `10197`; source declaration `d`; object begins at byte **13960**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `d.type` | `s.grid` | 13966 |
| `d.RowOffsets.keys` | `scaled(76)` | 13990 |
| `d.RowOffsets.controlKeys` | `scaled(76)` | 14015 |
| `d.Origins.keys` | `[scaled(30),scaled(92),scaled(154),scaled(216),scaled(278),scaled(340),scaled(402)]` | 14043 |
| `d.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14153 |

#### B. TextInput.js — component style `h`

Factory `10197`; source declaration `h`; object begins at byte **14200**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `h.type` | `s.grid` | 14206 |
| `h.RowOffsets.keys` | `scaled(93)` | 14230 |
| `h.RowOffsets.controlKeys` | `scaled(84)` | 14255 |
| `h.Origins.keys` | `[scaled(31),scaled(124),scaled(216),scaled(308),scaled(401)]` | 14283 |
| `h.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14366 |

#### B. TextInput.js — component style `u`

Factory `10197`; source declaration `u`; object begins at byte **14413**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `u.type` | `s.grid` | 14419 |
| `u.RowOffsets.keys` | `scaled(64)` | 14443 |
| `u.RowOffsets.controlKeys` | `scaled(70)` | 14468 |
| `u.Origins.keys` | `[scaled(31),scaled(124),scaled(216),scaled(308),scaled(401)]` | 14496 |
| `u.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14579 |

#### B. TextInput.js — component style `c`

Factory `10197`; source declaration `c`; object begins at byte **14626**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `c.type` | `s.grid` | 14632 |
| `c.RowOffsets.keys` | `scaled(93)` | 14656 |
| `c.RowOffsets.controlKeys` | `scaled(93)` | 14681 |
| `c.Origins.keys` | `[scaled(71),scaled(216),scaled(361)]` | 14709 |
| `c.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14764 |

#### B. TextInput.js — component style `g`

Factory `10197`; source declaration `g`; object begins at byte **14811**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `g.type` | `s.linear` | 14817 |
| `g.rowHeight` | `scaled(76)` | 14836 |
| `g.keySpacing` | `scaled(0)` | 14860 |

#### B. TextInput.js — component style `u`

Factory `36048`; source declaration `u`; object begins at byte **17406**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `u.height` | `scaled(70)` | 17414 |
| `u.bgColor` | `o.Q1.White10` | 17435 |
| `u.highlightColor` | `o.Q1.White10` | 17463 |
| `u.padding.leading` | `scaled(20)` | 17493 |
| `u.padding.trailing` | `scaled(20)` | 17515 |
| `u.padding.bottom` | `scaled(0)` | 17535 |
| `u.padding.top` | `scaled(0)` | 17551 |
| `u.highlightInsets.horizontal` | `scaled(0)` | 17592 |
| `u.highlightInsets.vertical` | `scaled(4)` | 17613 |
| `u.mask.gradientWidth` | `scaled(30)` | 17646 |
| `u.search.indentation` | `scaled(8)` | 17680 |
| `u.format.typography` | `Object.assign(Object.assign({},h.o.Standard.Body),{size:40,leading:40})` | 18309 |
| `u.format.color` | `o.Q1.LabelPrimary` | 18387 |
| `u.format.pageAlignment` | `.5` | 18419 |
| `u.infoFormat.typography` | `Object.assign(Object.assign({},h.o.Standard.Body),{size:40,leading:40})` | 18455 |
| `u.infoFormat.color` | `o.Q1.LabelTertiary` | 18533 |
| `u.infoFormat.pageAlignment` | `.5` | 18566 |
| `u.passwordFormat.typography` | `Object.assign(Object.assign({},h.o.Standard.Body),{size:40,leading:40})` | 18606 |
| `u.passwordFormat.color` | `o.Q1.LabelPrimary` | 18684 |
| `u.passwordFormat.pageAlignment` | `.5` | 18716 |

#### B. TextInput.js — component style `b`

Factory `23828`; source declaration `b`; object begins at byte **36802**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `b.width` | `scaled(54)` | 36809 |
| `b.height` | `scaled(64)` | 36829 |

#### B. TextInput.js — component style `D`

Factory `23828`; source declaration `D`; object begins at byte **37778**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `D.TabKey.Default.typography` | `Object.assign(Object.assign({},d.o.Standard.Body),{leading:56})` | 37807 |
| `D.TabKey.Default.textAlignment` | `.5` | 37885 |
| `D.TabKey.Default.pageAlignment` | `.5` | 37902 |
| `D.TabKey.Small.typography` | `Object.assign(Object.assign({},d.o.Standard.Body),{size:25,leading:56})` | 37924 |
| `D.TabKey.Small.textAlignment` | `.5` | 38010 |
| `D.TabKey.Small.pageAlignment` | `.5` | 38027 |
| `D.StandardKey.Default.typography` | `Object.assign(Object.assign({},d.o.Standard.Title3),{leading:56})` | 38065 |
| `D.StandardKey.Default.textAlignment` | `.5` | 38145 |
| `D.StandardKey.Default.pageAlignment` | `.5` | 38162 |
| `D.StandardKey.Small.typography.weight` | `h.I.Medium` | 38192 |
| `D.StandardKey.Small.typography.size` | `44` | 38208 |
| `D.StandardKey.Small.typography.leading` | `56` | 38219 |
| `D.StandardKey.Small.textAlignment` | `.5` | 38237 |
| `D.StandardKey.Small.pageAlignment` | `.5` | 38254 |
| `D.StandardKey.Smallest.typography.weight` | `h.I.Medium` | 38287 |
| `D.StandardKey.Smallest.typography.size` | `38` | 38303 |
| `D.StandardKey.Smallest.typography.leading` | `56` | 38314 |
| `D.StandardKey.Smallest.textAlignment` | `.5` | 38332 |
| `D.StandardKey.Smallest.pageAlignment` | `.5` | 38349 |
| `D.StandardKey.Placeholder.typography.weight` | `h.I.Medium` | 38385 |
| `D.StandardKey.Placeholder.typography.size` | `38` | 38401 |
| `D.StandardKey.Placeholder.typography.leading` | `56` | 38412 |
| `D.StandardKey.Placeholder.textAlignment` | `.5` | 38430 |
| `D.StandardKey.Placeholder.pageAlignment` | `.5` | 38447 |
| `D.ControlKey.Default.typography` | `Object.assign(Object.assign({},d.o.Standard.Body),{leading:56})` | 38484 |
| `D.ControlKey.Default.textAlignment` | `.5` | 38562 |
| `D.ControlKey.Default.pageAlignment` | `.5` | 38579 |
| `D.CandidateKey.Default.typography` | `Object.assign(Object.assign({},d.o.Standard.Title3),{size:36,leading:56})` | 38618 |
| `D.CandidateKey.Default.textAlignment` | `.5` | 38706 |
| `D.CandidateKey.Default.pageAlignment` | `.5` | 38723 |
| `D.DomainKey.Default.typography` | `Object.assign({},d.o.Standard.Caption2)` | 38759 |
| `D.DomainKey.Default.textAlignment` | `.5` | 38813 |
| `D.DomainKey.Default.pageAlignment` | `.5` | 38830 |

#### B. TextInput.js — component style `k`

Factory `23828`; source declaration `k`; object begins at byte **38838**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `k.States.Idle.Color.text` | `o.Q1.LabelSecondary()` | 38865 |
| `k.States.Idle.Color.iconOpacity` | `.5` | 38899 |
| `k.States.Idle.Padding.horizontal` | `scaled(0)` | 38923 |
| `k.States.Idle.Padding.vertical` | `scaled(0)` | 38944 |
| `k.States.Focused.Color.text` | `o.Q1.Black` | 38979 |
| `k.States.Focused.Color.iconOpacity` | `1` | 39002 |
| `k.States.Focused.Padding.horizontal` | `scaled(2)` | 39025 |
| `k.States.Focused.Padding.vertical` | `scaled(1)` | 39046 |
| `k.States.Pressed.Color.text` | `o.Q1.Black` | 39081 |
| `k.States.Pressed.Color.iconOpacity` | `1` | 39104 |
| `k.States.Pressed.Padding.horizontal` | `scaled(-2)` | 39127 |
| `k.States.Pressed.Padding.vertical` | `scaled(-3)` | 39149 |
| `k.States.Active.Color.text` | `o.Q1.White` | 39184 |
| `k.States.Active.Color.iconOpacity` | `1` | 39207 |
| `k.States.Active.Padding.horizontal` | `scaled(0)` | 39230 |
| `k.States.Active.Padding.vertical` | `scaled(0)` | 39251 |
| `k.textFormat` | `new l.l(Object.assign({},D.TabKey.Default))` | 39277 |

#### B. TextInput.js — component style `I`

Factory `23828`; source declaration `I`; object begins at byte **39397**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `I.States.Idle.Color.text` | `o.Q1.LabelSecondary()` | 39424 |
| `I.States.Idle.Color.iconOpacity` | `.6` | 39458 |
| `I.States.Idle.Padding.horizontal` | `scaled(0)` | 39482 |
| `I.States.Idle.Padding.vertical` | `scaled(0)` | 39503 |
| `I.States.Focused.Color.text` | `o.Q1.Black` | 39538 |
| `I.States.Focused.Color.iconOpacity` | `1` | 39561 |
| `I.States.Focused.Padding.horizontal` | `scaled(6)` | 39584 |
| `I.States.Focused.Padding.vertical` | `scaled(6)` | 39605 |
| `I.States.Pressed.Color.text` | `o.Q1.Black` | 39640 |
| `I.States.Pressed.Color.iconOpacity` | `1` | 39663 |
| `I.States.Pressed.Padding.horizontal` | `scaled(2)` | 39686 |
| `I.States.Pressed.Padding.vertical` | `scaled(2)` | 39707 |
| `I.textFormat` | `new l.l(Object.assign(Object.assign({},D.StandardKey.Default),{disableScaling:!0}))` | 39733 |

#### B. TextInput.js — component style `E`

Factory `23828`; source declaration `E`; object begins at byte **40049**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `E.States.Idle` | `I.States.Idle` | 40063 |
| `E.States.Focused.Color` | `I.States.Focused.Color` | 40092 |
| `E.States.Focused.Padding.horizontal` | `scaled(11)` | 40135 |
| `E.States.Focused.Padding.vertical` | `scaled(6)` | 40157 |
| `E.States.Pressed.Color` | `I.States.Pressed.Color` | 40186 |
| `E.States.Pressed.Padding.horizontal` | `scaled(7)` | 40229 |
| `E.States.Pressed.Padding.vertical` | `scaled(2)` | 40250 |
| `E.textFormat` | `new l.l(Object.assign(Object.assign({},D.StandardKey.Small),{disableScaling:!0}))` | 40276 |

#### B. TextInput.js — component style `F`

Factory `23828`; source declaration `F`; object begins at byte **40493**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `F.States.Idle.Color.text` | `o.Q1.LabelSecondary()` | 40520 |
| `F.States.Idle.Color.iconOpacity` | `.5` | 40554 |
| `F.States.Idle.Padding.horizontal` | `scaled(0)` | 40578 |
| `F.States.Idle.Padding.vertical` | `scaled(0)` | 40599 |
| `F.States.Focused.Color.text` | `o.Q1.Black` | 40634 |
| `F.States.Focused.Color.iconOpacity` | `1` | 40657 |
| `F.States.Focused.Padding.horizontal` | `scaled(6)` | 40680 |
| `F.States.Focused.Padding.vertical` | `scaled(6)` | 40701 |
| `F.States.Pressed.Color.text` | `o.Q1.Black` | 40736 |
| `F.States.Pressed.Color.iconOpacity` | `1` | 40759 |
| `F.States.Pressed.Padding.horizontal` | `scaled(2)` | 40782 |
| `F.States.Pressed.Padding.vertical` | `scaled(2)` | 40803 |
| `F.textFormat` | `new l.l(Object.assign(Object.assign({},D.ControlKey.Default),{disableScaling:!0}))` | 40829 |
| `F.insets` | `scaled(10)` | 40919 |

#### B. TextInput.js — component style `L`

Factory `23828`; source declaration `L`; object begins at byte **40935**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `L.States.Idle.Color.text` | `o.Q1.Black` | 40962 |
| `L.States.Idle.Color.iconOpacity` | `.6` | 40985 |
| `L.States.Idle.Padding.horizontal` | `scaled(1)` | 41009 |
| `L.States.Idle.Padding.vertical` | `scaled(1)` | 41030 |
| `L.States.Focused.Color.text` | `o.Q1.Black` | 41065 |
| `L.States.Focused.Color.iconOpacity` | `1` | 41088 |
| `L.States.Focused.Padding.horizontal` | `scaled(1)` | 41111 |
| `L.States.Focused.Padding.vertical` | `scaled(1)` | 41132 |
| `L.States.Pressed.Color.text` | `o.Q1.Black` | 41167 |
| `L.States.Pressed.Color.iconOpacity` | `1` | 41190 |
| `L.States.Pressed.Padding.horizontal` | `scaled(1)` | 41213 |
| `L.States.Pressed.Padding.vertical` | `scaled(1)` | 41234 |
| `L.textFormat` | `new l.l(Object.assign(Object.assign({},D.StandardKey.Default),{disableScaling:!0}))` | 41260 |
| `L.insets` | `scaled(8)` | 41351 |

#### B. TextInput.js — component style `_`

Factory `23828`; source declaration `_`; object begins at byte **41765**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `_.TabKey.Default` | `k` | 41782 |
| `_.TabKey.Small` | `R` | 41790 |
| `_.StandardKey.Default` | `I` | 41814 |
| `_.StandardKey.Small` | `T` | 41822 |
| `_.StandardKey.Smallest` | `O` | 41833 |
| `_.StandardKey.Square` | `E` | 41842 |
| `_.DomainKey.Default` | `M` | 41864 |
| `_.ControlKey.Default` | `F` | 41887 |
| `_.ContextKey.Default` | `L` | 41910 |
| `_.ContextKey.Small` | `P` | 41918 |
| `_.ContextKey.Smallest` | `W` | 41929 |
| `_.ContextKey.Domain` | `z` | 41938 |
| `_.CandidateKey.Default.States.Idle.Color.text` | `o.Q1.White` | 41990 |
| `_.CandidateKey.Default.States.Idle.Color.iconOpacity` | `1` | 42013 |
| `_.CandidateKey.Default.States.Idle.Padding.horizontal` | `scaled(0)` | 42036 |
| `_.CandidateKey.Default.States.Idle.Padding.vertical` | `scaled(0)` | 42057 |
| `_.CandidateKey.Default.States.Focused.Color.text` | `o.Q1.Black` | 42092 |
| `_.CandidateKey.Default.States.Focused.Color.iconOpacity` | `1` | 42115 |
| `_.CandidateKey.Default.States.Focused.Padding.horizontal` | `scaled(6)` | 42138 |
| `_.CandidateKey.Default.States.Focused.Padding.vertical` | `scaled(6)` | 42159 |
| `_.CandidateKey.Default.States.Pressed.Color.text` | `o.Q1.Black` | 42194 |
| `_.CandidateKey.Default.States.Pressed.Color.iconOpacity` | `1` | 42217 |
| `_.CandidateKey.Default.States.Pressed.Padding.horizontal` | `scaled(2)` | 42240 |
| `_.CandidateKey.Default.States.Pressed.Padding.vertical` | `scaled(2)` | 42261 |
| `_.CandidateKey.Default.States.Hinted.Color.text` | `o.Q1.White` | 42295 |
| `_.CandidateKey.Default.States.Hinted.Color.iconOpacity` | `1` | 42318 |
| `_.CandidateKey.Default.States.Hinted.Padding.horizontal` | `scaled(1)` | 42341 |
| `_.CandidateKey.Default.States.Hinted.Padding.vertical` | `scaled(1)` | 42362 |
| `_.CandidateKey.Default.textFormat` | `new l.l(Object.assign({},D.CandidateKey.Default))` | 42388 |
| `_.CandidateKey.Default.insets` | `scaled(9)` | 42445 |
| `_.CandidateKey.Default.minFontSize` | `29` | 42469 |
| `_.Sizes.width` | `b.width` | 42487 |
| `_.Sizes.height` | `b.height` | 42502 |
| `_.Sizes.Scaling` | `(b.width+2*I.States.Pressed.Padding.horizontal)/(b.width+2*I.States.Focused.Padding.horizontal)` | 42519 |
| `_.Platters.Cursor.backgroundColor` | `o.Q1.White` | 42650 |
| `_.Platters.Cursor.cornerRadius` | `scaled(20)` | 42674 |
| `_.Platters.Hint.backgroundColor` | `o.Q1.White10` | 42710 |
| `_.Platters.Hint.cornerRadius` | `scaled(6)` | 42736 |
| `_.Shadows.Focused.color` | `o.Q1.Black30` | 42774 |
| `_.Shadows.Focused.offset` | `scaled(20)` | 42794 |
| `_.Shadows.Focused.blurRadius` | `scaled(25)` | 42818 |
| `_.Shadows.Pressed.color` | `o.Q1.Black40` | 42847 |
| `_.Shadows.Pressed.offset` | `scaled(7)` | 42867 |
| `_.Shadows.Pressed.blurRadius` | `scaled(12)` | 42890 |
| `_.Icons.Globe` | `A` | 42918 |
| `_.Icons.DeleteGrid` | `x` | 42931 |
| `_.Icons.DeleteLinear` | `S` | 42946 |
| `_.Icons.DeleteContext` | `v` | 42962 |
| `_.Icons.ExpandArrow` | `K` | 42976 |
| `_.GamepadHints.Space.image` | `null==(n=p.Rd.Config.supportedHints())?void 0:n.space` | 43006 |
| `_.GamepadHints.Space.rect` | `new w.r(scaled(-1),scaled(-1),scaled(24),scaled(24))` | 43065 |
| `_.GamepadHints.Delete.image` | `null==(s=p.Rd.Config.supportedHints())?void 0:s.delete` | 43141 |
| `_.GamepadHints.Delete.rect` | `new w.r(scaled(-1),scaled(-1),scaled(24),scaled(24))` | 43201 |
| `_.GamepadHints.Toggle.image` | `null==(r=p.Rd.Config.supportedHints())?void 0:r.toggleKeyPlane` | 43277 |
| `_.GamepadHints.Toggle.rect` | `new w.r(scaled(3),scaled(4),scaled(32),scaled(32))` | 43345 |
| `_.GamepadHints.getImage` | `(e,t)=>f.R.gamepadHintWitHeight(p.Rd.Config.systemBrandName,e,t)` | 43414 |
| `_.Truncation.fadeWidth` | `scaled(40)` | 43502 |
| `_.Durations.defaultLongpress` | `1e3` | 43544 |
| `_.Durations.kanaLongpress` | `500` | 43562 |

#### B. TextInput.js — component style `l`

Factory `40083`; source declaration `l`; object begins at byte **117999**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `l.gridWidth` | `scaled(432)` | 118010 |
| `l.Depths.tabRow` | `3` | 118039 |
| `l.Depths.keyPlane` | `1` | 118050 |
| `l.Depths.candidateRow` | `2` | 118065 |
| `l.Transitions.Durations.crossFade` | `250` | 118102 |
| `l.Transitions.Durations.fadeIn` | `100` | 118113 |
| `l.Transitions.Durations.fadeOut` | `100` | 118125 |
| `l.Transitions.Durations.context` | `200` | 118137 |
| `l.Transitions.Delays.crossFade` | `50` | 118160 |
| `l.opacityWithContextMenu` | `.6` | 118188 |

#### B. TextInput.js — component style `d`

Factory `40083`; source declaration `d`; object begins at byte **118194**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `d.width` | `l.gridWidth` | 118201 |
| `d.height` | `scaled(50)` | 118220 |
| `d.topMargin` | `scaled(6)` | 118243 |
| `d.bottomMargin` | `scaled(72)` | 118268 |
| `d.bottomBorder` | `()=>(r.fD.Sizes.height+d.height)/2` | 118294 |
| `d.backgroundColor` | `s.Q1.White10` | 118345 |
| `d.cornerRadius` | `scaled(16)` | 118371 |
| `d.baselineOffset` | `scaled(49)` | 118399 |

#### B. TextInput.js — component style `h`

Factory `40083`; source declaration `h`; object begins at byte **118415**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `h.Grid.size.width` | `scaled(128)` | 118434 |
| `h.Grid.size.height` | `r.fD.Sizes.height` | 118455 |
| `h.Grid.gamepadOffset` | `scaled(4)` | 118488 |
| `h.Linear.defaultWidth` | `scaled(64)` | 118522 |
| `h.Linear.gamepadHintWidth` | `scaled(94)` | 118552 |
| `h.Linear.gamepadFocusOffset` | `scaled(4)` | 118584 |
| `h.Linear.height` | `r.fD.Sizes.height` | 118603 |
| `h.Linear.deleteKeySpacing` | `scaled(15)` | 118638 |

#### B. TextInput.js — component style `u`

Factory `40083`; source declaration `u`; object begins at byte **118655**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `u.Background.color` | `s.Q1.colorFromRGB(128,128,128,1)` | 118674 |
| `u.Background.cornerRadius` | `scaled(20)` | 118720 |
| `u.Cursor.color` | `r.fD.Platters.Cursor.backgroundColor` | 118748 |
| `u.Cursor.cornerRadius` | `scaled(20)` | 118798 |
| `u.Shadow` | `r.fD.Shadows.Focused` | 118819 |
| `u.Animation.cursorMovement` | `75` | 118866 |
| `u.Animation.dismissalTimeout` | `100` | 118886 |

#### B. TextInput.js — component style `c`

Factory `40083`; source declaration `c`; object begins at byte **118894**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `c.Default.firstKeyWidth` | `scaled(55)` | 118918 |
| `c.Default.middleKeyWidths` | `scaled(54)` | 118947 |
| `c.Default.lastKeyWidth` | `scaled(55)` | 118973 |
| `c.Default.cursorWidth` | `scaled(56)` | 118998 |
| `c.Square.firstKeyWidth` | `scaled(55)` | 119034 |
| `c.Square.middleKeyWidths` | `scaled(64)` | 119063 |
| `c.Square.lastKeyWidth` | `scaled(65)` | 119089 |
| `c.Square.cursorWidth` | `scaled(66)` | 119114 |
| `c.keyOffset.x` | `scaled(1)` | 119141 |
| `c.keyOffset.y` | `scaled(1)` | 119155 |
| `c.rowHeight` | `r.fD.Sizes.height+r.fD.StandardKey.Default.States.Pressed.Padding.horizontal` | 119178 |
| `c.cursorHeight` | `scaled(66)` | 119268 |

#### B. TextInput.js — component style `g`

Factory `40083`; source declaration `g`; object begins at byte **119284**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `g.character.width` | `scaled(64)` | 119302 |
| `g.character.height` | `scaled(64)` | 119322 |
| `g.backgroundExpand` | `scaled(1)` | 119353 |
| `g.cursorWidth` | `scaled(76)` | 119377 |
| `g.cursorHeight` | `scaled(76)` | 119403 |

#### B. TextInput.js — component style `y`

Factory `40083`; source declaration `y`; object begins at byte **119419**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `y.height` | `scaled(672)` | 119427 |
| `y.bottomMargin` | `scaled(79)` | 119454 |
| `y.maskExpand` | `scaled(20)` | 119478 |
| `y.ExpandKey.width` | `r.fD.Sizes.width` | 119508 |
| `y.ExpandKey.height` | `r.fD.Sizes.height` | 119532 |
| `y.ExpandKey.rightOrigin` | `scaled(31)` | 119562 |
| `y.ExpandKey.leftMargin` | `scaled(20)` | 119586 |
| `y.Rows.keySpacing` | `scaled(20)` | 119617 |
| `y.Rows.height` | `r.fD.Sizes.height` | 119637 |
| `y.Rows.Offsets.first` | `scaled(72)` | 119670 |
| `y.Rows.Offsets.following` | `scaled(76)` | 119693 |
| `y.Rows.Margin.left` | `scaled(4)` | 119720 |
| `y.Rows.Margin.top` | `scaled(4)` | 119736 |
| `y.Rows.Margin.right` | `scaled(0)` | 119754 |
| `y.Rows.Margin.bottom` | `scaled(0)` | 119773 |
| `y.ScrollParams.fadeHeight` | `scaled(90)` | 119812 |
| `y.ScrollParams.maskOverflow` | `scaled(50)` | 119838 |
| `y.ScrollParams.peeking` | `scaled(228)` | 119859 |
| `y.ScrollParams.triggerOffset` | `scaled(35)` | 119887 |
| `y.ScrollParams.hiddenOffsets.inner` | `scaled(-10)` | 119921 |
| `y.ScrollParams.hiddenOffsets.outer` | `scaled(30)` | 119941 |
| `y.ScrollParams.visibleOffsets.inner` | `scaled(105)` | 119977 |
| `y.ScrollParams.visibleOffsets.outer` | `scaled(-15)` | 119997 |

#### B. WelcomeScreenGDPRPage.js — component style `z`

Factory `76756`; source declaration `z`; object begins at byte **4736**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `z.contentWidth` | `scaled(960)` | 4750 |
| `z.legalTextWidth.default` | `scaled(960)` | 4788 |
| `z.legalTextWidth.max` | `scaled(1100)` | 4806 |
| `z.avatarSize` | `scaled(138)` | 4833 |
| `z.checkmarkSize` | `scaled(120)` | 4861 |
| `z.horizontalAlignment` | `.5` | 4895 |
| `z.verticalAlignment` | `.5` | 4916 |
| `z.buttonMinWidth` | `scaled(500)` | 4934 |
| `z.checkmark.size` | `scaled(120)` | 4964 |
| `z.checkmark.color` | `A.Q1.LabelPrimary` | 4984 |
| `z.text.title.typography` | `W.o.Emphasized.Title1` | 5035 |
| `z.text.title.color` | `A.Q1.LabelPrimary` | 5063 |
| `z.text.title.textAlignment` | `.5` | 5095 |
| `z.text.legal` | `k` | 5106 |
| `z.multilineText.maxHeight` | `scaled(360)` | 5134 |
| `z.multilineText.horizontalBoxPadding` | `scaled(15)` | 5169 |
| `z.multilineText.verticalBoxPadding` | `scaled(15)` | 5201 |
| `z.multilineText.textFocusColor.light` | `A.Q1.LabelPrimary()` | 5236 |
| `z.multilineText.textFocusColor.dark` | `A.Q1.LabelPrimary()` | 5261 |
| `z.multilineText.ellipsisFocusColor.dark` | `A.Q1.LabelPrimary()` | 5307 |
| `z.multilineText.ellipsisFocusColor.light` | `A.Q1.LabelPrimary()` | 5333 |
| `z.multilineText.backgroundFocusColor.dark` | `A.Q1.White40` | 5381 |
| `z.multilineText.backgroundFocusColor.light` | `A.Q1.White40` | 5400 |
| `z.multilineText.backgroundUnfocusColor` | `A.Q1.White0` | 5437 |
| `z.multilineText.cornerRadius` | `scaled(20)` | 5462 |
| `z.multilineText.pressedStyle` | `T.e.Deflate` | 5488 |
| `z.multilineText.moreTextStyle.body` | `k` | 5520 |
| `z.multilineText.moreTextStyle.ellipsis` | `k` | 5531 |
| `z.multilineText.moreTextStyle.centerTextWithoutEllipsis` | `!0` | 5559 |
| `z.multilineText.moreTextStyle.alignment` | `1` | 5572 |
| `z.spacing.avatarToTitle` | `scaled(20)` | 5599 |
| `z.spacing.titleToLegal` | `scaled(23)` | 5625 |
| `z.spacing.legalToButton` | `scaled(23)` | 5652 |
| `z.spacing.buttonToButton` | `scaled(48)` | 5680 |

#### B. WelcomeScreenPage.js — component style `N`

Factory `30861`; source declaration `N`; object begins at byte **556**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `N.contentWidth.standard.default` | `scaled(700)` | 589 |
| `N.contentWidth.standard.max` | `scaled(930)` | 607 |
| `N.contentWidth.qrWithRemote.default` | `scaled(750)` | 644 |
| `N.contentWidth.qrWithRemote.max` | `scaled(930)` | 662 |
| `N.logoHeight` | `e=>e?scaled(70):scaled(65)` | 689 |
| `N.logoColor` | `M.Q1.White` | 730 |
| `N.spacing.logoToTitle` | `scaled(20)` | 762 |
| `N.spacing.leadingToContent` | `scaled(90)` | 792 |
| `N.spacing.accountLinkingButtonsToTitle` | `scaled(30)` | 834 |
| `N.spacing.contentToBottom` | `scaled(80)` | 863 |
| `N.spacing.contentToLegal` | `scaled(25)` | 891 |
| `N.spacing.titleToQr` | `scaled(30)` | 914 |
| `N.spacing.buttonToButton` | `scaled(14)` | 942 |
| `N.title.text.typography.weight` | `V.I.Bold` | 996 |
| `N.title.text.typography.size` | `64` | 1010 |
| `N.title.text.typography.leading` | `80` | 1021 |
| `N.title.text.color` | `M.Q1.LabelPrimary` | 1031 |
| `N.title.text.paragraphSpacingAfter` | `0` | 1071 |
| `N.title.text.paragraphSpacingBefore` | `0` | 1096 |
| `N.title.minSize` | `52` | 1108 |
| `N.title.maxLines` | `3` | 1120 |
| `N.legal.text.typography` | `G.o.Tall.Caption1` | 1155 |
| `N.legal.text.color` | `M.Q1.LabelSecondary` | 1179 |
| `N.legal.text.paragraphSpacingAfter` | `0` | 1221 |
| `N.legal.text.paragraphSpacingBefore` | `0` | 1246 |
| `N.legal.maxLines` | `5` | 1259 |
| `N.button.qrcode.width.default` | `scaled(400)` | 1293 |
| `N.button.qrcode.width.max` | `scaled(650)` | 1311 |
| `N.button.accountLinking.maxWidth` | `scaled(412)` | 1352 |
| `N.button.accountLinking.minWidth` | `scaled(380)` | 1375 |
| `N.button.style.withoutPlatter` | `Q.JM.Regular.Platterless()` | 1412 |
| `N.button.style.withPlatter` | `Q.JM.Regular.Dark()` | 1451 |
| `N.button.text.typography` | `G.o.Emphasized.Body` | 1497 |
| `N.button.text.color` | `M.Q1.LabelPrimary` | 1523 |
| `N.button.text.textAlignment` | `.5` | 1555 |
| `N.backgroundGradient.steps` | `[{offset:0,color:new O.Gfx.Color(0,0,0,0)},{offset:1,color:new O.Gfx.Color(0,0,0,1)}]` | 1587 |
| `N.backgroundGradient.vector.start.x.default` | `scaled(775)` | 1699 |
| `N.backgroundGradient.vector.start.x.max` | `scaled(1015)` | 1717 |
| `N.backgroundGradient.vector.start.y` | `scaled(177)` | 1735 |
| `N.backgroundGradient.vector.end.x.default` | `scaled(390)` | 1766 |
| `N.backgroundGradient.vector.end.x.max` | `scaled(630)` | 1784 |
| `N.backgroundGradient.vector.end.y` | `scaled(674)` | 1801 |
| `N.backgroundGradient.rect.x` | `0` | 1825 |
| `N.backgroundGradient.rect.y` | `0` | 1829 |
| `N.backgroundGradient.rect.w` | `B.nl.width` | 1833 |
| `N.backgroundGradient.rect.h` | `B.nl.height` | 1846 |
| `N.backgroundGradient.opacity.video` | `.7` | 1874 |
| `N.backgroundGradient.opacity.image` | `.95` | 1883 |
| `N.videoStyle.fadeInDuration` | `500` | 1916 |
| `N.videoStyle.fadeOutDuration` | `500` | 1936 |
| `N.videoStyle.videoStartDelay` | `3e3` | 1956 |
| `N.videoStyle.verticalAlignment` | `.5` | 1978 |

### B. File coverage and identities

A zero count means this top-level-style selector did not find a qualifying declaration; it does not mean that bundle contains no UI values.

| Bundle | Declaration leaf rows | SHA-256 |
| --- | ---: | --- |
| `Dwell.js` | 0 | `227d67800de19d0b75c0bf7599d4ba5b21f2d14c68930ead3936fe3e8c7d7abd` |
| `InAppPurchasing.js` | 0 | `9befbbdb2981a0bdde58b475b053331c232cd8d62dd14df5ca5a09b80c648c34` |
| `InAppRating.js` | 0 | `0a57c176189989744f51d1a197954debeca2896d54691846837f1e5943cc898f` |
| `LibraryPage.js` | 22 | `9c5f064924fa4562cd69490b8f160319a21c041e89021c4270f948e72644fbf7` |
| `ManageSportsPage.js` | 22 | `c817883164f774aa6ba5d6d673adccb3c27d5747b299b28046d8811a2933ca80` |
| `MovieBundlePage.js` | 7 | `e7bb9e35954ad2477b3555ab1b57c0cbb2d0ad13776b9878e639134856cf5c51` |
| `MultiViewPage.js` | 73 | `81cfbc138e399866a7feddda57c7d49938611782ec89a68edec48cf93ea5894d` |
| `ProductPage.js` | 209 | `3cf224603413db5d9be5324efa7f9c11102086e800f6f8972155747e0f6d1870` |
| `ProfileSwitcherWidget.js` | 0 | `12695b8514207dd0eed3ea91ba5687588054cddbf20ff1cfca0a198fd03ee438` |
| `QRDialogPage.js` | 0 | `47dcd556b0a5875a9f4e72440d1613f7a25c350c8fde3d82b09b446b5b8b8eaa` |
| `SearchPageFactory.js` | 87 | `6248f84fa94bbb3e4ec50d047860485c82d9eee9cb460eabaac7369ae7264f04` |
| `SelectAccountPage.js` | 0 | `915684cd3eb25aa4c1d426470c5d67bb94e0139c88ca30af9137450d46e07a34` |
| `Settings.js` | 14 | `49d86950b2d0fc71d1d3d36ae7c94aa36108510810c692bf4e7e65aff94621cd` |
| `SettingsPage.js` | 43 | `f93662bc564dcbcb18fcceeb86ce686b8195d27ad3444ff2ca2ca6463659de9c` |
| `SportsPostPlayPage.js` | 4 | `5971143942077f68d15e47d429598a5c77c2fb0f32e644886d7053e1f13b1062` |
| `Subscriptions.js` | 0 | `725e532eae47316126e0982d426a0412f07ca1ce4a68cbe714ae0da4aab40566` |
| `SubscriptionsV3.js` | 0 | `cbbac001e6954639a0e4fa76ad09a27896f796557a118d30e46637ff1211cb42` |
| `SuperBrowsePage.js` | 127 | `77c182bc42387e5c4c4344e9253f60f4f8ff51ce8ca1586f34fb5ba64508cc25` |
| `TextInput.js` | 281 | `94f971f710372e3a28369333cba3cf69eab2b1ea5c62c80b2b2d6a22a317d8bf` |
| `Trampoline.js` | 0 | `8e8abfb6667233cc90316ee080a58800aa283deae44c45aafa2f33dec208988c` |
| `WelcomeScreenFlow.js` | 0 | `ba43e390edaf7d4645b476e19c6a9e0f4ee15f2e843bc946314acdbb4defc5c2` |
| `WelcomeScreenGDPRPage.js` | 34 | `ee0ddf30d96ba577da001c70f516991ddc03583748b2d008c94dce7ae93bdd3b` |
| `WelcomeScreenPage.js` | 52 | `5ed846e64b160b08b77f884d5a36d72a52b22ae3c2d6e91359c06acdfb6b1774` |

### B. Remaining boundary

Shared app typography, colours and common component tokens are analysed separately. PlayerPage is analysed separately. Native runtime rasterisation, dynamic service-provided artwork palettes, live remote flags, GPU effects and actual screen-scale output cannot be recovered as universal fixed tokens from these component declaration tables.

### B. Follow-up: WebVTT caption drawing tokens in `6759.js`

Static source: factory **70004**, exported class **`WebVTTLunaRenderer`**. SHA-256: `743703b3d53e1b6d0356d4e32729831104808b82d575ba15e3f158576fd4728a`. The base object `s` begins at byte **550248** (the identifier assignment starts at 550246). These are renderer defaults, not proof of a user's actual caption settings or of every active playback backend. Public setters and caption CSS can override them.

The following rows supplement the 975 declaration-appendix rows; they are not included in that count.

| Token / rule | Exact stored value or expression | Source owner; UTF-8 byte |
| --- | --- | --- |
| Default screen width / height | 1920 / 1080 | `s.screenWidth/screenHeight`; 550248 |
| Baseline resolution | 1080 | `s.resolution`; 550248 |
| Medium/default caption size | 46 | `s.fontSize.medium`; `_initStyles`; 550248 / 557299 |
| Large-caption threshold | 80; padding chooses the large set when `styles.size >= 80` | `s.fontSize.large`; `_calculateCueBoxes`; 550248 / 559933 |
| Standard background padding | Horizontal 18; vertical 12 | `s.backgroundPadding.standard`; 550248 |
| Large background padding | Horizontal 22; vertical 7 | `s.backgroundPadding.large`; 550248 |
| Per-line foreground/highlight padding | Horizontal 6; vertical 3 | `s.cuePadding`; 550248 |
| Caption line spacing | `2 * cuePadding.vertical` = 6 | `_calculateCueBoxes`; 559933 |
| Default font collection | New empty native `Gfx.FontList`; no family name established here | `_initStyles`; 557299 |
| Default bold / italic font override | `null` / `null` | `_initStyles`; 557299 |
| Default text colour | Native `Gfx.Color(1,1,1,1)` | `_initStyles`; 557299 |
| Default background colour | Native `Gfx.Color(0,0,0,.5)` | `_initStyles`; 557299 |
| Default foreground highlight | Native `Gfx.Color(0,0,0,0)` | `_initStyles`; 557299 |
| Default text edge style | `CMCharacterEdgeStyle_None` | `_initStyles`; 557299 |
| Default background corner radius | 0; subsequently settable | Constructor / `setRoundedCorners`; 550727 / 555403 |
| Edge width basis | `round(fontSize * (3/34) * (resolution/1080))` | `_setEdgeStyle` / `_pixels`; 557934 / 565304 |
| Depressed edge | Black shadow at `(-edge,+edge)`, blur edge | `_setEdgeStyle`; 557934 |
| Drop shadow edge | Black shadow at `(0,0)`, blur edge | `_setEdgeStyle`; 557934 |
| Raised edge | Black shadow at `(-edge,-edge)`, blur edge | `_setEdgeStyle`; 557934 |
| Uniform edge blur | Black shadow at `(0,0)`, blur `0.4 * edge` | `_setEdgeStyle`; 557934 |
| Uniform edge drawing spread | `(2/34) * fontSize * (resolution/1080)`; eight offset text draws around the foreground | `_handleUniformTextShadow`; 559250 |
| Cue paragraph spacing / initial page alignment | 0 / 0 | `_createCueBox`; 558370 |
| Snap-to-lines grid | 15 positions; calculated line index clamped to 0–14, then divided by 15; negative indexes offset from 15 | `_calculateCueBoxes`; 559933 |
| Default cue extent | 100 (incoming WebVTT cue size); default alignment centre; line/position auto; snap-to-lines true | `initDefaultValues`; 556927 |
| Default override flags enabled | font, size, colour, textOpacity, backgroundColor, textEdgeStyle | `_initOverrideProperties`; 557694 |
| Default override flags disabled | backgroundColorOpacity, textHighlight | `_initOverrideProperties`; 557694 |
| Render-area fallback dimensions | Parent width or 1920; parent height or 1080 | `getWidth/getHeight`; 565545 / 565718 |
| Safe drawing area | Subtract supplied leading/trailing or top/bottom padding from parent extent, clamp result at zero | `getWidth/getHeight`; 565545 / 565718 |
| Background box rounding | Origin floored; extent ceiled after padding | `_calculateCueBoxes`; 559933 |

Important boundaries: `setStyles`, `setVideoOverride`, `setBackgroundPadding`, `setSubtitlesAreaPadding` and `setRoundedCorners` alter these settings. Cue CSS may supply a background colour. The renderer separately lays out horizontal, vertical-left-to-right and vertical-right-to-left cues. Its caption padding is not the shared application safe-area token. No font family, accessibility text-size choice or actual rendered subtitle size should be inferred from the default empty font collection alone.

## C. Player and playback presentation

### C. Scope and evidence

Static extraction from the retained decoded `PlayerPage.js` in the effective OTA package, build `16.4.4.J34L139`. SHA-256: `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955`. Values below are observed declarations and call-site constants, not measurements of a live TV and not recommendations for Bloob.

Locations are **zero-based UTF-8 byte offsets in the retained decoded file**, not line numbers in a beautified copy. The minified variable name is a source identifier; names such as `player.transport` below are descriptive aliases, not claims that the reference exports a public token API.

#### C. Units and unresolved references

- **S(n)** means the source's `yA(n)` screen-scaling helper. Preserve it as a scaled design value; do not treat it as an invariant physical pixel value. Plain numeric values and `S(...)` are kept distinct.
- Duration values are milliseconds unless explicitly stated otherwise.
- Semantic colour and typography references are preserved by their source names. Resolve them against the shared foundation inventory, rather than inventing RGB/font values here.
- Runtime-dependent dimensions, platform constraints and inherited style factories remain expressions.
- Vector icon path control points are artwork, not design tokens; icon frame dimensions are included. No complete extracted component source is reproduced.
- Playback buffer/codec settings and analytics constants are outside this design-token scope.

### C.1 Transport bar foundation

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

#### C. Derived transport geometry

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

### C.2 Unified player layout and button references

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

#### C. Player control icon frames

Source: module `54629`, icon factory `h` (local `T`), immediately after the style factory.

| Token | Value |
|---|---|
| Subtitle/audio icon frame | 29 × 29 through `symbolWithSize` |
| Generated transport-control SVG view box | 64 × 64 |
| Generated transport-control SVG display size | S(64) × S(64) |
| Plus icon | Shared symbol using `Emphasized.Body` |
| Play, pause, forward, rewind, skip-ten, close, checkmark, jump-live artwork | Distinct paths within the common icon framing; path coordinates are intentionally not copied as tokens. |

### C.3 Player timing and transitions

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

### C.4 Standalone LIVE pill

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

### C.5 Player details panel and actions

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

#### C. Details action button states (`ev`)

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

### C.6 Skip / pre-roll promotional lockup

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

#### C. Protection gradient

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

### C.7 Post-play and picture-in-picture

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

### C.8 InSight people and music cards

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

### C.9 Catch Up to Live platter

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

### C.10 Notifications and multi-view affordances

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

### C.11 Caption geometry and style references

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

#### C. Audio/subtitle menu local overrides

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

### C.12 Other page-level presentation values

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

### C. Coverage and limitations

This document enumerates the player-specific style objects, their principal constructor overrides, explicit UI transition/timer constants, caption obstruction geometry and supporting post-play/InSight/live/multi-view components in the retained effective `PlayerPage.js`. Shared typography, material, global button and floating-menu definitions are referenced rather than duplicated; consult the shared-token extraction for their values.

The raw AST inventory should be consulted alongside these semantic tables for additional literal call sites and all source offsets. Geometry generated by server-supplied metadata, accessibility settings and native font metrics remains runtime-dependent. Static declaration presence establishes available code paths, not active feature flags, live rendered output or measured responsiveness.

## D. Additional shared component declarations

This appendix inventories 130 additional style declaration groups in `app.js`, with 2216 property rows. It expands the foundations above into cards, sports lockups, dialogs, badges, banners, overlays, notifications, QR panels and shared navigation. Rows can be aliases, inherited styles, conditional rules or asset dependencies; they are not a count of unique scalar tokens.

Original factory and local object names are retained. For compact reading, verified imports from factory46491 are shown as `S(...)`, factory44548 `Q1` references as `Color.*`, and factory3038 `o` references as `Typography.*`. Other aliases are mapped beside their group. A parenthesized `(spread)` is inheritance. Long functions/assets are listed by location rather than copied or evaluated.

### D.1 Group index

| Factory | Local object | Object byte | Property rows |
| --- | --- | --- | --- |
| 29119 | `k` | 252210 | 17 |
| 87658 | `Y` | 266791 | 21 |
| 87658 | `eW` | 284106 | 15 |
| 5922 | `eO` | 318016 | 23 |
| 21301 | `b` | 353517 | 4 |
| 21301 | `A` | 353583 | 34 |
| 73529 | `ei` | 372671 | 20 |
| 69773 | `C` | 399414 | 16 |
| 69773 | `k` | 401338 | 12 |
| 69773 | `q` | 406206 | 20 |
| 69773 | `ee` | 410877 | 20 |
| 69773 | `en` | 412947 | 16 |
| 69773 | `eo` | 416604 | 9 |
| 69773 | `ed` | 419126 | 22 |
| 69773 | `eS` | 427187 | 9 |
| 69773 | `ew` | 427424 | 15 |
| 69773 | `eC` | 427801 | 54 |
| 69773 | `eR` | 444024 | 44 |
| 69773 | `e_` | 449070 | 33 |
| 69773 | `eV` | 453506 | 4 |
| 80269 | `V` | 511290 | 42 |
| 80269 | `eo` | 516736 | 52 |
| 91236 | `B` | 571081 | 36 |
| 91180 | `eo` | 642656 | 25 |
| 21020 | `D` | 708769 | 10 |
| 46691 | `l` | 714157 | 22 |
| 47733 | `R` | 721899 | 7 |
| 87821 | `m` | 735819 | 4 |
| 76885 | `y` | 777530 | 19 |
| 45643 | `g` | 780141 | 12 |
| 69169 | `C` | 784785 | 15 |
| 97963 | `O` | 853362 | 22 |
| 61202 | `e$` | 913628 | 29 |
| 61202 | `e0` | 914628 | 29 |
| 61202 | `e1` | 915585 | 29 |
| 61202 | `e2` | 916536 | 29 |
| 61202 | `e3` | 917505 | 29 |
| 61202 | `ie` | 977204 | 6 |
| 61202 | `iR` | 1007794 | 14 |
| 61202 | `nt` | 1018341 | 61 |
| 61202 | `nb` | 1035522 | 3 |
| 61202 | `aF` | 1088864 | 40 |
| 61202 | `aG` | 1090852 | 41 |
| 61202 | `a9` | 1100420 | 4 |
| 61202 | `rg` | 1118308 | 37 |
| 60347 | `P` | 1380749 | 24 |
| 5651 | `C` | 1493352 | 11 |
| 15264 | `p` | 1498042 | 21 |
| 10751 | `s` | 1498858 | 14 |
| 16501 | `C` | 1509325 | 3 |
| 40126 | `f` | 1509575 | 8 |
| 26794 | `S` | 1517206 | 3 |
| 57214 | `g` | 1518434 | 3 |
| 57214 | `p` | 1518561 | 3 |
| 57214 | `m` | 1518688 | 3 |
| 57214 | `f` | 1518817 | 10 |
| 57214 | `y` | 1519089 | 10 |
| 57214 | `v` | 1519363 | 10 |
| 70982 | `p` | 1540358 | 4 |
| 70982 | `m` | 1540510 | 7 |
| 70982 | `f` | 1540696 | 3 |
| 99454 | `d` | 1540890 | 22 |
| 85790 | `P` | 1545628 | 7 |
| 18637 | `p` | 1554915 | 14 |
| 18637 | `m` | 1555605 | 5 |
| 17686 | `S` | 1556122 | 35 |
| 17686 | `w` | 1557216 | 35 |
| 17686 | `C` | 1558312 | 24 |
| 17686 | `b` | 1559050 | 24 |
| 5641 | `m` | 1572699 | 5 |
| 52297 | `h` | 1593885 | 14 |
| 52297 | `g` | 1594257 | 3 |
| 52297 | `p` | 1594382 | 27 |
| 11922 | `h` | 1603570 | 42 |
| 11922 | `b` | 1607481 | 28 |
| 58125 | `d` | 1617787 | 25 |
| 87981 | `p` | 1629343 | 10 |
| 36813 | `s` | 1631044 | 15 |
| 36813 | `l` | 1631424 | 16 |
| 15915 | `m` | 1632051 | 26 |
| 44418 | `f` | 1638429 | 67 |
| 44418 | `y` | 1640716 | 5 |
| 21676 | `f` | 1647550 | 15 |
| 77926 | `A` | 1650717 | 34 |
| 50776 | `w` | 1670619 | 7 |
| 21110 | `eE` | 1719820 | 15 |
| 34597 | `Z` | 1737920 | 37 |
| 34597 | `Y` | 1738957 | 5 |
| 34597 | `el` | 1774620 | 91 |
| 49286 | `u` | 1805984 | 2 |
| 49286 | `h` | 1806209 | 10 |
| 49286 | `g` | 1806499 | 2 |
| 49286 | `p` | 1806645 | 2 |
| 49286 | `m` | 1806841 | 2 |
| 23220 | `T` | 1807781 | 11 |
| 23220 | `P` | 1808134 | 8 |
| 23220 | `x` | 1808361 | 3 |
| 23220 | `I` | 1808467 | 9 |
| 23220 | `L` | 1808771 | 27 |
| 23220 | `k` | 1809723 | 6 |
| 23220 | `R` | 1809898 | 6 |
| 23220 | `D` | 1810150 | 5 |
| 65112 | `s` | 2032903 | 10 |
| 10710 | `d` | 2091807 | 4 |
| 71706 | `l` | 2100732 | 7 |
| 71706 | `d` | 2100938 | 5 |
| 42048 | `m` | 2118220 | 14 |
| 65274 | `r{0}` | 2212798 | 4 |
| 50357 | `p` | 2256011 | 37 |
| 33239 | `u` | 2262339 | 2 |
| 69583 | `T{0}` | 2298033 | 4 |
| 69583 | `T{1}` | 2298124 | 7 |
| 69583 | `T{2}` | 2298349 | 8 |
| 69583 | `T{3}` | 2298599 | 12 |
| 69583 | `T{4}` | 2298997 | 13 |
| 69583 | `P` | 2299409 | 3 |
| 69583 | `I` | 2299587 | 3 |
| 48302 | `h{0}` | 2301849 | 3 |
| 95561 | `c` | 2336207 | 6 |
| 17550 | `l` | 2354176 | 35 |
| 90118 | `d` | 2355410 | 5 |
| 75262 | `k` | 2373007 | 66 |
| 75262 | `R` | 2374934 | 5 |
| 75262 | `D{0}` | 2375229 | 5 |
| 75262 | `D{1}` | 2375413 | 4 |
| 75262 | `E` | 2376676 | 4 |
| 26769 | `g` | 2400680 | 6 |
| 90180 | `C` | 2403782 | 7 |
| 49336 | `u` | 2426587 | 22 |
| 25654 | `L` | 2581272 | 7 |

### D.2 Factory 29119 — `k`

Source: `app.js`, object byte **252210**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 24759; `x`→factory 3038; `I`→factory 44548; `L`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `k.font.title.format.typography` | `Typography.Emphasized.Title2` | 252251 |
| `k.font.title.format.color` | `Color.LabelPrimary` | 252279 |
| `k.font.title.format.textAlignment` | `.5` | 252311 |
| `k.font.title.alignment` | `.5` | 252326 |
| `k.font.title.attributes.lines` | `1` | 252347 |
| `k.font.title.maxWidth` | `S(960)` | 252359 |
| `k.font.subtitle.format.typography` | `Typography.Standard.Body` | 252411 |
| `k.font.subtitle.format.color` | `Color.LabelPrimary` | 252435 |
| `k.font.subtitle.format.textAlignment` | `.5` | 252467 |
| `k.font.subtitle.alignment` | `.5` | 252482 |
| `k.font.subtitle.attributes.lines` | `2` | 252503 |
| `k.font.subtitle.maxWidth` | `S(960)` | 252515 |
| `k.subscriptionList.alignment` | `.5` | 252559 |
| `k.subscriptionList.shrink` | `1` | 252569 |
| `k.titleToSubtitleSpacing` | `S(31)` | 252595 |
| `k.subtitleBottomSpacing` | `S(40)` | 252630 |
| `k.subscriptionItem.spacing` | `S(14)` | 252669 |

### D.3 Factory 87658 — `Y`

Source: `app.js`, object byte **266791**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `F`→factory 3038; `M`→factory 44548; `W`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `Y.cornerRadius` | `S(12)` | 266805 |
| `Y.fonts.title.typography` | `Typography.Emphasized.Headline` | 266851 |
| `Y.fonts.title.color` | `Color.LabelPrimary` | 266881 |
| `Y.fonts.title.textAlignment` | `.5` | 266913 |
| `Y.imageToTitleMargin` | `S(50)` | 266938 |
| `Y.imageHeight.unfocused` | `S(231)` | 266974 |
| `Y.imageHeight.focused` | `S(279)` | 266996 |
| `Y.buttonSpacing` | `S(14)` | 267025 |
| `Y.titleBottomSpacing` | `S(55)` | 267057 |
| `Y.versionItem.spacing` | `S(97)` | 267091 |
| `Y.versionItem.dimensions.unfocused.width` | `S(870)` | 267133 |
| `Y.versionItem.dimensions.unfocused.height` | `S(166)` | 267154 |
| `Y.versionItem.dimensions.focused.width` | `S(870)` | 267184 |
| `Y.versionItem.dimensions.focused.height` | `S(196)` | 267205 |
| `Y.spacing` | `S(190)` | 267230 |
| `Y.episodeWidth` | `S(W.nl.width-190-190-190-480)` | 267257 |
| `Y.episodePadding` | `S(97)` | 267309 |
| `Y.transactionWidth` | `S(480)` | 267339 |
| `Y.transactionBottomPadding` | `S(20)` | 267378 |
| `Y.transactionButtonSpacing` | `S(55)` | 267416 |
| `Y.fade` | `S(60)` | 267434 |

### D.4 Factory 87658 — `eW`

Source: `app.js`, object byte **284106**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `F`→factory 3038; `M`→factory 44548; `W`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eW.title.typography` | `Typography.Emphasized.Callout` | 284133 |
| `eW.title.color` | `Color.White` | 284162 |
| `eW.title.textAlignment` | `.5` | 284187 |
| `eW.title.pageAlignment` | `.5` | 284204 |
| `eW.layout.maxHeight` | `S(920)` | 284227 |
| `eW.layout.width` | `S(640)` | 284247 |
| `eW.spacings.headerToList` | `S(32)` | 284285 |
| `eW.spacings.listItems` | `S(18)` | 284308 |
| `eW.paddings.header.leading` | `S(50)` | 284348 |
| `eW.paddings.header.trailing` | `S(50)` | 284370 |
| `eW.paddings.header.top` | `S(50)` | 284387 |
| `eW.paddings.header.bottom` | `S(32)` | 284407 |
| `eW.paddings.listItems.leading` | `S(31)` | 284440 |
| `eW.paddings.listItems.trailing` | `S(31)` | 284462 |
| `eW.paddings.layoutBottom` | `S(31)` | 284489 |

### D.5 Factory 5922 — `eO`

Source: `app.js`, object byte **318016**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `y`→factory 46491; `I`→factory 8060; `eL`→factory 55242; `ek`→factory 3038; `eR`→factory 41457.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eO.width` | `y.nl.width` | 318023 |
| `eO.height` | `S(502)` | 318041 |
| `eO.padding.leading` | `y.nl.SafeArea.horizontal` | 318072 |
| `eO.padding.trailing` | `y.nl.SafeArea.horizontal` | 318106 |
| `eO.padding.top` | `S(74)` | 318135 |
| `eO.padding.bottom` | `S(52)` | 318155 |
| `eO.contentWidth` | `S(575)` | 318182 |
| `eO.teamNameWidth` | `y.nl.SafeArea.width` | 318210 |
| `eO.spacings.teamNameToSubtitle` | `S(7)` | 318259 |
| `eO.spacings.contentToButton` | `S(32)` | 318287 |
| `eO.teamLogo.size` | `S(147)` | 318316 |
| `eO.teamLogo.padding` | `S(32)` | 318338 |
| `eO.fonts.teamName.typography.fontName` | `"SFCompressed-Semibold"` | 318399 |
| `eO.fonts.teamName.typography.weight` | `eL.I.Semibold` | 318430 |
| `eO.fonts.teamName.typography.size` | `110` | 318449 |
| `eO.fonts.teamName.typography.leading` | `110` | 318461 |
| `eO.fonts.teamName.color` | `$.Q1.LabelPrimary` | 318472 |
| `eO.fonts.teamNameHeight` | `S(110)` | 318507 |
| `eO.fonts.subtitle.typography` | `Typography.Standard.Caption1` | 318551 |
| `eO.fonts.subtitle.color` | `$.Q1.LabelSecondary_RespectTransparencySetting` | 318580 |
| `eO.fonts.subtitle.vibrancyEffectStyle` | `"secondary"` | 318647 |
| `eO.favoriteIconSize` | `S(30)` | 318679 |
| `eO.favoriteButton` | `eR.JM.Regular.Dark()` | 318707 |

### D.6 Factory 21301 — `b`

Source: `app.js`, object byte **353517**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `y`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `b.dx` | `0` | 353521 |
| `b.dy` | `S(4)` | 353526 |
| `b.color` | `Color.Black0` | 353544 |
| `b.blurRadius` | `S(12)` | 353567 |

### D.7 Factory 21301 — `A`

Source: `app.js`, object byte **353583**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `d`→factory 31210; `u`→factory 3038; `m`→factory 90058; `y`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `A.(spread)` | `(0,d.G)()` | 353587 |
| `A.artwork.size.width` | `S(76)` | 353618 |
| `A.artwork.size.height` | `S(76)` | 353638 |
| `A.artwork.placeholderColor` | `new(a()).Gfx.Color(29/255,29/255,29/255)` | 353669 |
| `A.artwork.cornerRadius` | `S(38)` | 353723 |
| `A.artwork.fadeInDuration` | `240*!!m.w.Config.Performance.otherAnimationsEnabled` | 353751 |
| `A.artwork.fallbackIcon.height` | `S(48)` | 353824 |
| `A.artwork.fallbackIcon.width` | `S(28)` | 353843 |
| `A.artwork.fallbackIcon.background` | `Color.SystemGray2()` | 353867 |
| `A.artwork.fallbackIcon.fill` | `Color.Gray92` | 353891 |
| `A.artwork.fallbackIcon.leadingPadding` | `S(4)` | 353918 |
| `A.platter.color` | `(e=o.Sx.dark)=>e===o.Sx.dark?Color.White20:Color.White60` | 353947 |
| `A.platter.cornerRadius` | `S(20)` | 354015 |
| `A.platter.padding.leading` | `S(20)` | 354045 |
| `A.platter.padding.trailing` | `S(40)` | 354067 |
| `A.platter.padding.top` | `S(20)` | 354084 |
| `A.platter.padding.bottom` | `S(20)` | 354104 |
| `A.shadow.idle` | `b` | 354132 |
| `A.shadow.focused.(spread)` | `b` | 354146 |
| `A.shadow.focused.color` | `Color.Black40` | 354154 |
| `A.spacings.eyebrowToTitle` | `0` | 354194 |
| `A.spacings.artworkToTexts` | `S(16)` | 354211 |
| `A.displacementSafetyPadding` | `S(20)` | 354251 |
| `A.eyebrowFormat.typography` | `Typography.Standard.Body` | 354298 |
| `A.eyebrowFormat.color` | `Color.LabelSecondary` | 354322 |
| `A.eyebrowMaxLines` | `1` | 354360 |
| `A.titleIdleFormat.typography` | `Typography.Emphasized.Callout` | 354398 |
| `A.titleIdleFormat.color` | `Color.LabelSecondary` | 354427 |
| `A.titleFocusedFormat.typography` | `Typography.Emphasized.Callout` | 354488 |
| `A.titleFocusedFormat.color` | `Color.LabelPrimary` | 354517 |
| `A.titleMaxLines` | `1` | 354551 |
| `A.shortNoteFormat.typography` | `Typography.Standard.Body` | 354589 |
| `A.shortNoteFormat.color` | `Color.LabelSecondary` | 354613 |
| `A.shortNoteMaxLines` | `1` | 354653 |

### D.8 Factory 73529 — `ei`

Source: `app.js`, object byte **372671**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 96547; `f`→factory 46491; `ee`→factory 3038; `et`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `ei.backgroundColor` | `new(K()).Gfx.Color(17/255,35/255,56/255)` | 372688 |
| `ei.formats.title.typography` | `Typography.Emphasized.Headline` | 372764 |
| `ei.formats.title.color` | `Color.LabelPrimary` | 372795 |
| `ei.formats.titleMaxLines` | `2` | 372830 |
| `ei.formats.body.typography` | `Typography.Standard.Callout` | 372857 |
| `ei.formats.body.color` | `Color.LabelSecondary` | 372885 |
| `ei.formats.bodyMaxLines` | `3` | 372921 |
| `ei.titleToBodySpacing` | `S(12)` | 372943 |
| `ei.preFavorites.ctaWidth` | `S(620)` | 372979 |
| `ei.preFavorites.ctaPadding.vertical` | `S(24)` | 373014 |
| `ei.preFavorites.teamLogoSize` | `S(115)` | 373041 |
| `ei.preFavorites.teamLogoRowSpacing` | `S(10)` | 373074 |
| `ei.preFavorites.teamLogosToCtaSpacing` | `S(10)` | 373109 |
| `ei.postFavorites.padding` | `S(48)` | 373146 |
| `ei.postFavorites.ctaWidth` | `S(612)` | 373168 |
| `ei.postFavorites.teamViewWidth` | `S(1004)` | 373196 |
| `ei.postFavorites.teamSpacing` | `S(26)` | 373223 |
| `ei.postFavorites.teamLogoMaxSize` | `S(180)` | 373252 |
| `ei.postFavorites.maxNumberOfTeams` | `8` | 373283 |
| `ei.postFavorites.maxNumberOfTeamsFullSize` | `5` | 373310 |

### D.9 Factory 69773 — `C`

Source: `app.js`, object byte **399414**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.GridC.padding.leading` | `S(18)` | 399439 |
| `C.GridC.padding.trailing` | `S(18)` | 399461 |
| `C.GridC.padding.bottom` | `S(12)` | 399481 |
| `C.GridD.padding.leading` | `S(14)` | 399520 |
| `C.GridD.padding.trailing` | `S(14)` | 399542 |
| `C.GridD.padding.bottom` | `S(8)` | 399562 |
| `C.contentHeight` | `S(153)` | 399590 |
| `C.titleToDescriptionSpacing` | `S(5)` | 399630 |
| `C.formats.title.typography` | `Typography.Short.Caption2` | 399677 |
| `C.formats.title.color` | `Color.LabelPrimary` | 399702 |
| `C.formats.description.typography` | `Typography.Standard.Caption3` | 399754 |
| `C.formats.description.color` | `Color.LabelSecondary_RespectTransparencySetting` | 399782 |
| `C.formats.description.vibrancyEffectStyle` | `"secondary"` | 399849 |
| `C.formats.captionMaxLines` | `1` | 399879 |
| `C.formats.titleMaxLines` | `1` | 399895 |
| `C.formats.descriptionMaxLines` | `3` | 399917 |

### D.10 Factory 69773 — `k`

Source: `app.js`, object byte **401338**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 9317; `o`→factory 46069; `f`→factory 44548; `y`→factory 50429; `S`→factory 3038; `w`→factory 46491; `L`→factory 58125.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `k.imageSize` | `S(200)` | 401349 |
| `k.imageTopPadding` | `S(50)` | 401379 |
| `k.imageHairline.thickness` | `S(1)` | 401417 |
| `k.imageHairline.color` | `Color.colorWithAlpha(Color.White,.16)` | 401435 |
| `k.textHorizontalPadding` | `S(20)` | 401494 |
| `k.formats.titleIdle.typography` | `Typography.Standard.Body` | 401546 |
| `k.formats.titleIdle.color` | `Color.LabelPrimary` | 401570 |
| `k.formats.titleFocused.typography` | `Typography.Standard.Body` | 401623 |
| `k.formats.titleFocused.color` | `Color.LabelPrimary` | 401647 |
| `k.initials` | `L.r.initials` | 401677 |
| `k.background.idleColor` | `new y.yP(1,1,1,.1)` | 401712 |
| `k.background.focusedColor` | `new y.yP(1,1,1,.2)` | 401744 |

### D.11 Factory 69773 — `q`

Source: `app.js`, object byte **406206**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `q.ordinalNumber.format.typography` | `Typography.Emphasized.Title1` | 406249 |
| `q.ordinalNumber.format.color` | `Color.LabelPrimary` | 406277 |
| `q.ordinalNumber.maxLines` | `1` | 406306 |
| `q.ordinalNumber.padding.horizontal` | `S(20)` | 406328 |
| `q.ordinalNumber.gradient.image` | `K` | 406358 |
| `q.ordinalNumber.gradient.width` | `S(260)` | 406366 |
| `q.ordinalNumber.gradient.height` | `S(188)` | 406387 |
| `q.contentLogo.maxWidth` | `S(172)` | 406425 |
| `q.contentLogo.maxHeight` | `S(60)` | 406449 |
| `q.contentLogo.padding.vertical` | `S(58)` | 406480 |
| `q.fallbackTitle.format.typography` | `Typography.Standard.Body` | 406537 |
| `q.fallbackTitle.format.textAlignment` | `.5` | 406569 |
| `q.fallbackTitle.format.color` | `Color.LabelPrimary` | 406578 |
| `q.footer.format.typography` | `Typography.Standard.Caption3` | 406634 |
| `q.footer.format.color` | `Color.LabelSecondary_RespectTransparencySetting` | 406662 |
| `q.footer.format.vibrancyEffectStyle` | `"secondary"` | 406729 |
| `q.footer.focusedColor` | `Color.LabelPrimary` | 406756 |
| `q.footer.maxLines` | `1` | 406783 |
| `q.footer.padding.horizontal` | `S(20)` | 406805 |
| `q.footer.padding.vertical` | `S(14)` | 406827 |

### D.12 Factory 69773 — `ee`

Source: `app.js`, object byte **410877**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `S`→factory 3038; `w`→factory 46491; `I`→factory 43822.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `ee.contentPadding.horizontal` | `S(24)` | 410905 |
| `ee.contentPadding.bottom` | `S(14)` | 410925 |
| `ee.spacing` | `S(4)` | 410947 |
| `ee.eyebrow.format.typography` | `Typography.Standard.Caption3` | 410995 |
| `ee.eyebrow.format.color` | `Color.LabelSecondary_RespectTransparencySetting` | 411023 |
| `ee.eyebrow.format.textAlignment` | `0` | 411084 |
| `ee.eyebrow.format.vibrancyEffectStyle` | `"secondary"` | 411106 |
| `ee.eyebrow.maxLines` | `2` | 411129 |
| `ee.titleLarge.format.typography.(spread)` | `Typography.Standard.Title2` | 411175 |
| `ee.titleLarge.format.typography.weight` | `$.I.Semibold` | 411202 |
| `ee.titleLarge.format.color` | `Color.LabelPrimary` | 411222 |
| `ee.titleLarge.format.textAlignment` | `0` | 411254 |
| `ee.titleLarge.maxLines` | `1` | 411267 |
| `ee.titleLarge.minSizeFactor` | `.7` | 411283 |
| `ee.titleSmall.format.typography.(spread)` | `Typography.Standard.Callout` | 411330 |
| `ee.titleSmall.format.typography.weight` | `$.I.Semibold` | 411358 |
| `ee.titleSmall.format.color` | `Color.LabelPrimary` | 411378 |
| `ee.titleSmall.format.textAlignment` | `0` | 411410 |
| `ee.titleSmall.maxLines` | `3` | 411423 |
| `ee.titleSmall.minSizeFactor` | `.7` | 411439 |

### D.13 Factory 69773 — `en`

Source: `app.js`, object byte **412947**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `en.contentLogo.maxWidth` | `S(172)` | 412970 |
| `en.contentLogo.maxHeight` | `S(60)` | 412994 |
| `en.contentLogo.paddingBottom` | `S(16)` | 413021 |
| `en.contentPadding.horizontal` | `S(20)` | 413062 |
| `en.contentPadding.bottom` | `S(14)` | 413082 |
| `en.fallbackTitle.format.typography` | `Typography.Standard.Body` | 413138 |
| `en.fallbackTitle.format.textAlignment` | `.5` | 413170 |
| `en.fallbackTitle.format.color` | `Color.LabelPrimary` | 413179 |
| `en.footer.format.typography` | `Typography.Standard.Caption3` | 413235 |
| `en.footer.format.color` | `Color.LabelSecondary_RespectTransparencySetting` | 413263 |
| `en.footer.format.vibrancyEffectStyle` | `"secondary"` | 413330 |
| `en.footer.focusedColor` | `Color.LabelPrimary` | 413357 |
| `en.footer.maxLines` | `2` | 413384 |
| `en.channelLogo.height` | `S(16)` | 413407 |
| `en.channelLogo.padding.horizontal` | `S(20)` | 413440 |
| `en.channelLogo.padding.vertical` | `S(20)` | 413462 |

### D.14 Factory 69773 — `eo`

Source: `app.js`, object byte **416604**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eo.contentLogo.maxWidth` | `S(116)` | 416627 |
| `eo.contentLogo.maxHeight` | `S(27)` | 416651 |
| `eo.contentLogo.padding.vertical` | `S(20)` | 416682 |
| `eo.channelLogo.background.height` | `S(130)` | 416729 |
| `eo.channelLogo.background.material.type` | `g().Gfx.DrawingContext.MaterialType.Regular` | 416758 |
| `eo.channelLogo.background.material.theme` | `g().Gfx.DrawingContext.MaterialTheme.Dark` | 416808 |
| `eo.channelLogo.maxWidth` | `S(172)` | 416861 |
| `eo.channelLogo.maxHeight` | `S(60)` | 416885 |
| `eo.channelLogo.padding.vertical` | `S(20)` | 416916 |

### D.15 Factory 69773 — `ed`

Source: `app.js`, object byte **419126**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `y`→factory 50429; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `ed.GridC.padding` | `S(18)` | 419142 |
| `ed.GridC.titleFont.typography` | `Typography.Standard.Caption1` | 419185 |
| `ed.GridC.titleFont.color` | `Color.LabelPrimary` | 419213 |
| `ed.GridC.titleToMetadataSpacing` | `S(5)` | 419256 |
| `ed.GridC.teamLogo.size` | `S(40)` | 419283 |
| `ed.GridC.teamLogo.padding` | `S(14)` | 419304 |
| `ed.GridD.padding` | `S(14)` | 419334 |
| `ed.GridD.titleFont.typography` | `Typography.Standard.Caption2` | 419377 |
| `ed.GridD.titleFont.color` | `Color.LabelPrimary` | 419405 |
| `ed.GridD.titleToMetadataSpacing` | `S(5)` | 419448 |
| `ed.GridD.teamLogo.size` | `S(32)` | 419475 |
| `ed.GridD.teamLogo.padding` | `S(10)` | 419496 |
| `ed.providerLogo.size` | `S(36)` | 419530 |
| `ed.providerLogo.borderThickness` | `S(1)` | 419559 |
| `ed.providerLogo.borderColor` | `y.yP.from(Color.colorWithAlpha(Color.White,.16))` | 419583 |
| `ed.providerLogo.spacing` | `S(8)` | 419638 |
| `ed.providerLogo.padding.bottom` | `S(6)` | 419666 |
| `ed.eyebrowFont.typography.(spread)` | `Typography.Standard.Caption3` | 419716 |
| `ed.eyebrowFont.typography.openTypeFeatures` | `[g().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,g().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 419755 |
| `ed.eyebrowFont.color` | `Color.LabelSecondary` | 419874 |
| `ed.eyebrowFont.vibrancyEffectStyle` | `"secondary"` | 419914 |
| `ed.metadataPadding.bottom` | `S(4)` | 419952 |

### D.16 Factory 69773 — `eS`

Source: `app.js`, object byte **427187**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eS.padding.top` | `S(40)` | 427201 |
| `eS.padding.bottom` | `S(32)` | 427221 |
| `eS.padding.leading` | `S(40)` | 427242 |
| `eS.padding.trailing` | `S(40)` | 427264 |
| `eS.subtitleSpacing` | `S(18)` | 427294 |
| `eS.flexibleLogo.maxWidth` | `S(128)` | 427330 |
| `eS.flexibleLogo.maxHeight` | `S(63)` | 427354 |
| `eS.flexibleLogo.gradient.width` | `S(257)` | 427383 |
| `eS.flexibleLogo.gradient.height` | `S(167)` | 427404 |

### D.17 Factory 69773 — `ew`

Source: `app.js`, object byte **427424**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `ew.D.padding.top` | `S(60)` | 427441 |
| `ew.D.padding.bottom` | `S(24)` | 427461 |
| `ew.D.padding.leading` | `S(60)` | 427482 |
| `ew.D.padding.trailing` | `S(60)` | 427504 |
| `ew.F.padding.top` | `S(60)` | 427535 |
| `ew.F.padding.bottom` | `S(52)` | 427555 |
| `ew.F.padding.leading` | `S(60)` | 427576 |
| `ew.F.padding.trailing` | `S(60)` | 427598 |
| `ew.contentZoom` | `.35` | 427625 |
| `ew.select.horizontal` | `S(40)` | 427648 |
| `ew.select.vertical` | `S(44)` | 427670 |
| `ew.flexibleLogo.maxWidth` | `S(222)` | 427707 |
| `ew.flexibleLogo.maxHeight` | `S(83)` | 427731 |
| `ew.flexibleLogo.gradient.width` | `S(257)` | 427760 |
| `ew.flexibleLogo.gradient.height` | `S(167)` | 427781 |

### D.18 Factory 69773 — `eC`

Source: `app.js`, object byte **427801**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `y`→factory 50429; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eC.D.font.typography` | `Typography.Standard.Caption1` | 427830 |
| `eC.D.font.color` | `Color.LabelPrimary` | 427858 |
| `eC.D.font.textAlignment` | `.5` | 427890 |
| `eC.D.font.pageAlignment` | `1` | 427907 |
| `eC.D.maxLines` | `2` | 427920 |
| `eC.D.maxWidth.card` | `S(480)` | 427937 |
| `eC.D.maxWidth.wide` | `S(840)` | 427956 |
| `eC.F.fonts.title.typography` | `Typography.Standard.Title2` | 428008 |
| `eC.F.fonts.title.color` | `Color.LabelPrimary` | 428034 |
| `eC.F.fonts.title.textAlignment` | `0` | 428066 |
| `eC.F.fonts.title.pageAlignment` | `1` | 428082 |
| `eC.F.fonts.metadata.typography` | `Typography.Standard.Caption1` | 428115 |
| `eC.F.fonts.metadata.color` | `Color.LabelPrimary` | 428143 |
| `eC.F.fonts.metadata.textAlignment` | `0` | 428175 |
| `eC.F.fonts.metadata.pageAlignment` | `.5` | 428191 |
| `eC.F.fonts.ratingHeight` | `S(22)` | 428209 |
| `eC.F.fonts.symbolSize` | `S(18)` | 428233 |
| `eC.F.fonts.metadataSymbolSpacing` | `S(12)` | 428268 |
| `eC.F.fonts.description.typography` | `Typography.Standard.Caption1` | 428313 |
| `eC.F.fonts.description.color` | `Color.LabelSecondary_RespectTransparencySetting` | 428341 |
| `eC.F.fonts.description.vibrancyEffectStyle` | `"secondary"` | 428408 |
| `eC.F.fonts.description.textAlignment` | `0` | 428434 |
| `eC.F.fonts.description.pageAlignment` | `1` | 428450 |
| `eC.F.fonts.descriptionMaxLines` | `3` | 428474 |
| `eC.F.fonts.explainability.typography` | `Typography.Standard.Caption1` | 428511 |
| `eC.F.fonts.explainability.color` | `Color.LabelPrimary` | 428539 |
| `eC.F.fonts.explainability.textAlignment` | `0` | 428571 |
| `eC.F.fonts.explainability.pageAlignment` | `.5` | 428587 |
| `eC.F.channelLogo.size` | `S(40)` | 428611 |
| `eC.F.channelLogo.borderThickness` | `S(1)` | 428640 |
| `eC.F.channelLogo.borderColor` | `y.yP.from(Color.colorWithAlpha(Color.White,.16))` | 428664 |
| `eC.F.channelLogo.spacing` | `S(12)` | 428719 |
| `eC.F.card.width` | `S(480)` | 428745 |
| `eC.F.card.metadataHeight` | `S(40)` | 428774 |
| `eC.F.card.contentLogo.maxWidth` | `S(300)` | 428809 |
| `eC.F.card.contentLogo.maxHeight` | `S(80)` | 428833 |
| `eC.F.card.contentLogo.area` | `S(45e3)` | 428851 |
| `eC.F.card.logoToBadge` | `S(20)` | 428879 |
| `eC.F.card.scoreboardToBadge` | `S(14)` | 428910 |
| `eC.F.card.metadataToLogo` | `S(14)` | 428938 |
| `eC.F.card.metadataToScoreboard` | `S(14)` | 428972 |
| `eC.F.card.descriptionToMetadata` | `S(10)` | 429007 |
| `eC.F.card.explainabilityToDescription` | `S(14)` | 429048 |
| `eC.F.wide.width` | `S(560)` | 429074 |
| `eC.F.wide.metadataHeight` | `S(40)` | 429103 |
| `eC.F.wide.contentLogo.maxWidth` | `S(360)` | 429138 |
| `eC.F.wide.contentLogo.maxHeight` | `S(96)` | 429162 |
| `eC.F.wide.contentLogo.area` | `S(54e3)` | 429180 |
| `eC.F.wide.logoToBadge` | `S(20)` | 429208 |
| `eC.F.wide.scoreboardToBadge` | `S(14)` | 429239 |
| `eC.F.wide.metadataToLogo` | `S(18)` | 429267 |
| `eC.F.wide.metadataToScoreboard` | `S(18)` | 429301 |
| `eC.F.wide.descriptionToMetadata` | `S(14)` | 429336 |
| `eC.F.wide.explainabilityToDescription` | `S(18)` | 429377 |

### D.19 Factory 69773 — `eR`

Source: `app.js`, object byte **444024**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `l`→factory 84556; `f`→factory 44548; `y`→factory 50429; `v`→factory 16393; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eR.artwork.Standard.width` | `S(213)` | 444050 |
| `eR.artwork.Standard.height` | `S(120)` | 444071 |
| `eR.artwork.Standard.cornerRadius` | `S(10)` | 444098 |
| `eR.artwork.Rectangle.width` | `S(80)` | 444129 |
| `eR.artwork.Rectangle.height` | `S(120)` | 444149 |
| `eR.artwork.Rectangle.cornerRadius` | `S(10)` | 444176 |
| `eR.artwork.Square.width` | `S(120)` | 444204 |
| `eR.artwork.Square.height` | `S(120)` | 444225 |
| `eR.artwork.Square.cornerRadius` | `S(10)` | 444252 |
| `eR.artwork.Circle.width` | `S(120)` | 444280 |
| `eR.artwork.Circle.height` | `S(120)` | 444301 |
| `eR.artwork.Circle.cornerRadius` | `S(60)` | 444328 |
| `eR.eyebrow.typography.(spread)` | `Typography.Standard.Caption2` | 444375 |
| `eR.eyebrow.typography.openTypeFeatures` | `[g().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,g().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 444414 |
| `eR.eyebrow.color` | `Color.LabelTertiary` | 444533 |
| `eR.title.typography` | `Typography.Standard.Body` | 444580 |
| `eR.title.color` | `Color.LabelPrimary` | 444604 |
| `eR.titleMaxLines` | `2` | 444638 |
| `eR.metadata.typography` | `Typography.Standard.Caption2` | 444669 |
| `eR.metadata.color` | `Color.LabelSecondary` | 444697 |
| `eR.description.format.typography` | `Typography.Tall.Caption2` | 444759 |
| `eR.description.format.color` | `Color.LabelPrimary` | 444783 |
| `eR.description.maxLines` | `6` | 444812 |
| `eR.description.padding.top` | `S(20)` | 444827 |
| `eR.description.symbolWidth` | `S(24)` | 444853 |
| `eR.padding` | `S(20)` | 444875 |
| `eR.alignment` | `y.OD.leading` | 444898 |
| `eR.paddingWithoutArtwork` | `S(32)` | 444933 |
| `eR.background.idleColor` | `new y.yP(1,1,1,.1)` | 444968 |
| `eR.background.focusedColor` | `new y.yP(1,1,1,.2)` | 445000 |
| `eR.titleTopPadding` | `S(4)` | 445036 |
| `eR.metadataTopPadding` | `S(3)` | 445067 |
| `eR.metadataTopPaddingWithEyebrow` | `S(8)` | 445109 |
| `eR.badgePadding` | `S(14)` | 445134 |
| `eR.badgeBottomPadding` | `S(3)` | 445166 |
| `eR.initials.backgroundColor` | `y.yP.from([.5,.5,.5,1])` | 445204 |
| `eR.initials.format.typography.(spread)` | `Typography.Rounded.Caption1` | 445259 |
| `eR.initials.format.typography.size` | `63` | 445285 |
| `eR.initials.format.color` | `Color.LabelPrimary` | 445295 |
| `eR.initials.format.textAlignment` | `.5` | 445327 |
| `eR.initials.format.pageAlignment` | `.5` | 445344 |
| `eR.initials.minSize` | `v.l.minimumFontSize` | 445357 |
| `eR.channelLogo.size` | `S(30)` | 445396 |
| `eR.channelLogo.spacing` | `S(8)` | 445417 |

### D.20 Factory 69773 — `e_`

Source: `app.js`, object byte **449070**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46069; `f`→factory 44548; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e_.formats.titleOnly.idle.typography` | `Typography.Standard.Callout` | 449116 |
| `e_.formats.titleOnly.idle.color` | `Color.OpacityZero` | 449143 |
| `e_.formats.titleOnly.idle.textAlignment` | `.5` | 449174 |
| `e_.formats.titleOnly.focused.typography` | `Typography.Standard.Callout` | 449207 |
| `e_.formats.titleOnly.focused.color` | `Color.LabelPrimary` | 449234 |
| `e_.formats.titleOnly.focused.shadow.dx` | `0` | 449263 |
| `e_.formats.titleOnly.focused.shadow.dy` | `S(4)` | 449268 |
| `e_.formats.titleOnly.focused.shadow.color` | `Color.themedColor(Color.Black0,Color.Black40)` | 449286 |
| `e_.formats.titleOnly.focused.shadow.blurRadius` | `S(4)` | 449340 |
| `e_.formats.titleOnly.focused.textAlignment` | `.5` | 449367 |
| `e_.formats.titleAndSubtitle.titleIdle.typography` | `Typography.Standard.Caption1` | 449421 |
| `e_.formats.titleAndSubtitle.titleIdle.color` | `Color.LabelPrimary` | 449449 |
| `e_.formats.titleAndSubtitle.titleIdle.textAlignment` | `0` | 449481 |
| `e_.formats.titleAndSubtitle.titleFocused.typography` | `Typography.Standard.Caption1` | 449518 |
| `e_.formats.titleAndSubtitle.titleFocused.color` | `Color.LabelPrimary` | 449546 |
| `e_.formats.titleAndSubtitle.titleFocused.shadow.dx` | `0` | 449575 |
| `e_.formats.titleAndSubtitle.titleFocused.shadow.dy` | `S(4)` | 449580 |
| `e_.formats.titleAndSubtitle.titleFocused.shadow.color` | `Color.themedColor(Color.Black0,Color.Black40)` | 449598 |
| `e_.formats.titleAndSubtitle.titleFocused.shadow.blurRadius` | `S(4)` | 449652 |
| `e_.formats.titleAndSubtitle.titleFocused.textAlignment` | `0` | 449679 |
| `e_.formats.titleAndSubtitle.subtitleIdle.typography` | `Typography.Standard.Caption2` | 449716 |
| `e_.formats.titleAndSubtitle.subtitleIdle.color` | `Color.LabelSecondary` | 449744 |
| `e_.formats.titleAndSubtitle.subtitleIdle.textAlignment` | `0` | 449778 |
| `e_.formats.titleAndSubtitle.subtitleFocused.typography` | `Typography.Standard.Caption2` | 449818 |
| `e_.formats.titleAndSubtitle.subtitleFocused.color` | `Color.LabelPrimary` | 449846 |
| `e_.formats.titleAndSubtitle.subtitleFocused.shadow.dx` | `0` | 449875 |
| `e_.formats.titleAndSubtitle.subtitleFocused.shadow.dy` | `S(4)` | 449880 |
| `e_.formats.titleAndSubtitle.subtitleFocused.shadow.color` | `Color.themedColor(Color.Black0,Color.Black40)` | 449898 |
| `e_.formats.titleAndSubtitle.subtitleFocused.shadow.blurRadius` | `S(4)` | 449952 |
| `e_.formats.titleAndSubtitle.subtitleFocused.textAlignment` | `0` | 449979 |
| `e_.formats.titleAndSubtitle.spacing` | `S(2)` | 449991 |
| `e_.paddingGridC` | `S(18)` | 450018 |
| `e_.paddingGridD` | `S(14)` | 450044 |

### D.21 Factory 69773 — `eV`

Source: `app.js`, object byte **453506**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `f`→factory 44548; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eV.backgroundColor` | `Color.SystemGray4` | 453523 |
| `eV.tableHeaderToContentSpacing` | `S(2)` | 453568 |
| `eV.legendSpacing` | `S(30)` | 453594 |
| `eV.teamSpacing` | `S(4)` | 453619 |

### D.22 Factory 80269 — `V`

Source: `app.js`, object byte **511290**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 70871; `p`→factory 46491; `v`→factory 38594; `M`→factory 3038; `_`→factory 44548; `O`→factory 31210; `W`→factory 41457.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `V.measure.leadingContentWidth` | `S(835)` | 511320 |
| `V.measure.buttonWidth` | `S(480)` | 511346 |
| `V.measure.trailingContentWidth` | `S(700)` | 511381 |
| `V.title.maxLines` | `2` | 511412 |
| `V.title.format.typography` | `Typography.Emphasized.Title1` | 511441 |
| `V.title.format.color` | `Color.LabelSecondary` | 511469 |
| `V.title.format.vibrancyEffectStyle` | `"secondary"` | 511509 |
| `V.title.format.textAlignment` | `.5` | 511535 |
| `V.subtitle.maxHeight` | `S(400)` | 511561 |
| `V.subtitle.multilineTextStyle.moreTextStyle.body.typography` | `Typography.Standard.Headline` | 511635 |
| `V.subtitle.multilineTextStyle.moreTextStyle.body.color` | `Color.LabelSecondary` | 511663 |
| `V.subtitle.multilineTextStyle.moreTextStyle.body.vibrancyEffectStyle` | `"secondary"` | 511703 |
| `V.subtitle.multilineTextStyle.moreTextStyle.body.textAlignment` | `.5` | 511729 |
| `V.subtitle.multilineTextStyle.moreTextStyle.ellipsis.typography` | `Typography.Emphasized.Headline` | 511763 |
| `V.subtitle.multilineTextStyle.moreTextStyle.ellipsis.color` | `Color.LabelSecondary` | 511793 |
| `V.subtitle.multilineTextStyle.moreTextStyle.alignment` | `.5` | 511825 |
| `V.subtitle.multilineTextStyle.textFocusColor.light` | `Color.LabelPrimary()` | 511851 |
| `V.subtitle.multilineTextStyle.textFocusColor.dark` | `Color.LabelPrimary()` | 511876 |
| `V.subtitle.multilineTextStyle.ellipsisFocusColor.dark` | `Color.LabelPrimary()` | 511922 |
| `V.subtitle.multilineTextStyle.ellipsisFocusColor.light` | `Color.LabelPrimary()` | 511948 |
| `V.subtitle.multilineTextStyle.backgroundFocusColor.dark` | `Color.White40` | 511996 |
| `V.subtitle.multilineTextStyle.backgroundFocusColor.light` | `Color.White40` | 512015 |
| `V.subtitle.multilineTextStyle.backgroundUnfocusColor` | `new v.Gfx.Color(1,1,1,0)` | 512052 |
| `V.subtitle.multilineTextStyle.horizontalBoxPadding` | `S(15)` | 512098 |
| `V.subtitle.multilineTextStyle.verticalBoxPadding` | `S(15)` | 512130 |
| `V.subtitle.multilineTextStyle.cornerRadius` | `S(20)` | 512156 |
| `V.subtitle.multilineTextStyle.pressedStyle` | `O.e.Deflate` | 512182 |
| `V.artwork.url` | `B` | 512209 |
| `V.artwork.width` | `S(617)` | 512217 |
| `V.artwork.height` | `S(621)` | 512238 |
| `V.glyph.symbol.path` | `Computed value; 1252 source bytes; retained at source location` | 512273 |
| `V.glyph.symbol.size.width` | `116.319` | 513538 |
| `V.glyph.symbol.size.height` | `98.596` | 513553 |
| `V.glyph.symbolSize` | `524` | 513573 |
| `V.glyph.color` | `Color.White30` | 513583 |
| `V.button` | `W.JM.Regular.Platterless()` | 513604 |
| `V.spacing.leadingContentMargin` | `S(150)` | 513661 |
| `V.spacing.trailingContentMargin` | `S(125)` | 513697 |
| `V.spacing.titleToSubtitle` | `S(43)` | 513727 |
| `V.spacing.subtitleToButtons` | `S(44)` | 513758 |
| `V.spacing.buttonToButton` | `S(16)` | 513786 |
| `V.spacing.leadingContentToTrailingContent` | `S(110)` | 513831 |

### D.23 Factory 80269 — `eo`

Source: `app.js`, object byte **516736**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 70871; `p`→factory 46491; `v`→factory 38594; `M`→factory 3038; `_`→factory 44548; `O`→factory 31210; `W`→factory 41457; `Q`→factory 49479; `er`→factory 31398.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eo.measure.leadingContentWidth` | `S(725)` | 516766 |
| `eo.measure.fieldButtonWidth` | `S(600)` | 516797 |
| `eo.measure.doneButtonWidth` | `S(500)` | 516827 |
| `eo.measure.sponsorPlatterContentWidth.default` | `S(670)` | 516877 |
| `eo.measure.sponsorPlatterContentWidth.max` | `S(1e3)` | 516895 |
| `eo.title.format.typography` | `Typography.Standard.Title3` | 516945 |
| `eo.title.format.color` | `Color.LabelPrimary` | 516971 |
| `eo.title.format.textAlignment` | `.5` | 517003 |
| `eo.sponsorPlatter.background.color` | `new v.Gfx.Color(130/255,130/255,130/255,.1)` | 517043 |
| `eo.sponsorPlatter.background.cornerRadius` | `S(50)` | 517100 |
| `eo.sponsorPlatter.avatar.size` | `S(150)` | 517127 |
| `eo.sponsorPlatter.avatar.cornerRadius` | `S(75)` | 517154 |
| `eo.sponsorPlatter.title.format.typography` | `Typography.Emphasized.Body` | 517202 |
| `eo.sponsorPlatter.title.format.color` | `Color.LabelPrimary` | 517228 |
| `eo.sponsorPlatter.title.format.textAlignment` | `.5` | 517260 |
| `eo.sponsorPlatter.subtitle.maxHeight` | `S(300)` | 517286 |
| `eo.sponsorPlatter.subtitle.multilineText.horizontalBoxPadding` | `S(15)` | 517336 |
| `eo.sponsorPlatter.subtitle.multilineText.verticalBoxPadding` | `S(15)` | 517368 |
| `eo.sponsorPlatter.subtitle.multilineText.textFocusColor.light` | `Color.LabelPrimary()` | 517403 |
| `eo.sponsorPlatter.subtitle.multilineText.textFocusColor.dark` | `Color.LabelPrimary()` | 517428 |
| `eo.sponsorPlatter.subtitle.multilineText.ellipsisFocusColor.dark` | `Color.LabelPrimary()` | 517474 |
| `eo.sponsorPlatter.subtitle.multilineText.ellipsisFocusColor.light` | `Color.LabelPrimary()` | 517500 |
| `eo.sponsorPlatter.subtitle.multilineText.backgroundFocusColor.dark` | `Color.White40` | 517548 |
| `eo.sponsorPlatter.subtitle.multilineText.backgroundFocusColor.light` | `Color.White40` | 517567 |
| `eo.sponsorPlatter.subtitle.multilineText.backgroundUnfocusColor` | `new v.Gfx.Color(1,1,1,0)` | 517604 |
| `eo.sponsorPlatter.subtitle.multilineText.cornerRadius` | `S(20)` | 517642 |
| `eo.sponsorPlatter.subtitle.multilineText.pressedStyle` | `O.e.Deflate` | 517668 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.body.typography` | `Typography.Standard.Caption2` | 517720 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.body.color` | `Color.LabelTertiary` | 517748 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.body.textAlignment` | `.5` | 517781 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.ellipsis.typography` | `Typography.Emphasized.Caption2` | 517815 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.ellipsis.color` | `Color.LabelTertiary` | 517845 |
| `eo.sponsorPlatter.subtitle.multilineText.moreTextStyle.alignment` | `1` | 517876 |
| `eo.button.edit` | `er.Q.RollingDecorator()` | 517895 |
| `eo.button.done` | `W.JM.Regular.Default()` | 517924 |
| `eo.button.data` | `er.Q.GDPR()` | 517952 |
| `eo.spacing.leadingMargin.default` | `S(125)` | 517997 |
| `eo.spacing.leadingMargin.max` | `S(60)` | 518015 |
| `eo.spacing.trailingMargin.default` | `S(220)` | 518053 |
| `eo.spacing.trailingMargin.max` | `S(80)` | 518071 |
| `eo.spacing.leadingContentToSponsorPlatter` | `S(150)` | 518116 |
| `eo.spacing.titleToButtons` | `S(60)` | 518145 |
| `eo.spacing.buttonToButton` | `S(16)` | 518173 |
| `eo.spacing.buttonsToDoneButton` | `S(60)` | 518206 |
| `eo.spacing.avatarToAvatar` | `S(15)` | 518234 |
| `eo.spacing.avatarsToPlatterTitle` | `S(25)` | 518269 |
| `eo.spacing.platterTitleToPlatterSubtitle` | `S(25)` | 518312 |
| `eo.spacing.platterSubtitleToSeeHowYourDataIsManagedButton` | `S(30)` | 518372 |
| `eo.spacing.sponsorPlatterContentPadding.leading` | `S(60)` | 518423 |
| `eo.spacing.sponsorPlatterContentPadding.trailing` | `S(60)` | 518445 |
| `eo.spacing.sponsorPlatterContentPadding.top` | `S(80)` | 518462 |
| `eo.spacing.sponsorPlatterContentPadding.bottom` | `S(62)` | 518482 |

### D.24 Factory 91236 — `B`

Source: `app.js`, object byte **571081**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 66888; `y`→factory 46491; `P`→factory 3038; `x`→factory 44548; `I`→factory 41457.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `B.title.format.typography` | `Typography.Standard.Title3` | 571116 |
| `B.title.format.color` | `Color.LabelPrimary` | 571142 |
| `B.title.format.textAlignment` | `.5` | 571174 |
| `B.title.maxLines` | `2` | 571188 |
| `B.title.maxLength` | `80` | 571200 |
| `B.subtitle.format.typography` | `Typography.Standard.Callout` | 571241 |
| `B.subtitle.format.color` | `Color.LabelTertiary` | 571268 |
| `B.subtitle.format.vibrancyEffectStyle` | `"tertiary"` | 571307 |
| `B.subtitle.format.textAlignment` | `.5` | 571332 |
| `B.subtitle.maxLines` | `2` | 571346 |
| `B.restrictionsBox.width` | `S(350)` | 571372 |
| `B.restrictionsBox.height` | `S(200)` | 571393 |
| `B.restrictionsBox.symbolFormat.typography` | `Typography.Short.Caption1` | 571440 |
| `B.restrictionsBox.symbolFormat.color` | `Color.LabelPrimary` | 571465 |
| `B.restrictionsBox.symbolFormat.textAlignment` | `0` | 571497 |
| `B.restrictionsBox.rating.height` | `S(22)` | 571516 |
| `B.restrictionsBox.rating.color` | `new x.Q1(Color.LabelPrimary)` | 571535 |
| `B.restrictionsBox.rating.padding.top` | `S(25)` | 571576 |
| `B.restrictionsBox.rating.padding.bottom` | `S(20)` | 571596 |
| `B.restrictionsBox.label.format.typography` | `Typography.Emphasized.Caption1` | 571645 |
| `B.restrictionsBox.label.format.color` | `Color.LabelTertiary` | 571675 |
| `B.restrictionsBox.label.format.textAlignment` | `0` | 571708 |
| `B.restrictionsBox.label.minSize` | `Typography.Emphasized.Caption2` | 571720 |
| `B.restrictionsBox.label.maxLines` | `5` | 571753 |
| `B.restrictionsBox.label.padding.trailing` | `S(25)` | 571773 |
| `B.restrictionsBox.label.padding.bottom` | `S(25)` | 571793 |
| `B.restrictionsBox.label.padding.leading` | `S(25)` | 571814 |
| `B.restrictionsBox.background.cornerRadius` | `S(24)` | 571854 |
| `B.restrictionsBox.background.color` | `Color.White10` | 571873 |
| `B.buttons.(spread)` | `I.JM.Regular.Platterless()` | 571900 |
| `B.buttons.width` | `S(508)` | 571933 |
| `B.spacing.titleToSubtitle` | `S(35)` | 571973 |
| `B.spacing.subtitleToRestrictions` | `S(55)` | 572009 |
| `B.spacing.restrictionToRestriction` | `S(30)` | 572047 |
| `B.spacing.restrictionsToButtons` | `S(65)` | 572082 |
| `B.spacing.buttonToButton` | `S(16)` | 572110 |

### D.25 Factory 91180 — `eo`

Source: `app.js`, object byte **642656**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `b`→factory 44548; `A`→factory 84556; `ea`→factory 3038; `er`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eo.chickletTv` | `()=>"Assets/Icons/Apple_TV_App_Icon.png"` | 642668 |
| `eo.chickletRounding` | `S(18)` | 642726 |
| `eo.continueButton` | `()=>(0,A.Xe)("ATVE.GDPRReprompt.Continue")` | 642755 |
| `eo.titleFormat.typography` | `Typography.Emphasized.Title1` | 642831 |
| `eo.titleFormat.color` | `Color.LabelPrimary` | 642860 |
| `eo.titleFormat.textAlignment` | `.5` | 642892 |
| `eo.titleFormat.paragraphSpacingAfter` | `0` | 642917 |
| `eo.bodyFormat.typography` | `Typography.Standard.Body` | 642953 |
| `eo.bodyFormat.color` | `Color.LabelSecondary` | 642978 |
| `eo.bodyFormat.textAlignment` | `.5` | 643012 |
| `eo.bodyFormat.paragraphSpacingAfter` | `0` | 643037 |
| `eo.captionFormat.typography` | `Typography.Standard.Caption2` | 643076 |
| `eo.captionFormat.color` | `Color.LabelSecondary` | 643105 |
| `eo.captionFormat.textAlignment` | `.5` | 643139 |
| `eo.captionFormat.paragraphSpacingAfter` | `0` | 643164 |
| `eo.flex.buttonSpacing` | `S(40)` | 643188 |
| `eo.flex.width` | `S(870)` | 643208 |
| `eo.flex.title.marginBottom` | `S(25)` | 643243 |
| `eo.flex.body.marginBottom` | `S(55)` | 643277 |
| `eo.flex.caption.marginBottom` | `S(12)` | 643314 |
| `eo.buttonWidth` | `S(500)` | 643342 |
| `eo.chicklet.width` | `S(700)` | 643373 |
| `eo.chicklet.height` | `S(700)` | 643395 |
| `eo.leftMargin` | `S(150)` | 643422 |
| `eo.rightMargin` | `S(125)` | 643449 |

### D.26 Factory 21020 — `D`

Source: `app.js`, object byte **708769**. Values are static source declarations; offsets in the last column locate each original value.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `D.bufferStallTimeout` | `5e3` | 708789 |
| `D.fadeInDuration` | `1e3` | 708808 |
| `D.fadeOutDuration` | `1e3` | 708828 |
| `D.videoStartDelay` | `0` | 708848 |
| `D.verticalAlignment` | `.5` | 708868 |
| `D.gradientColor` | `new(d()).Gfx.Color("#111111")` | 708885 |
| `D.fadeInDelay` | `0` | 708927 |
| `D.scaleVideoToCover` | `!0` | 708947 |
| `D.liveStreamDuration` | `3e4` | 708969 |
| `D.progressUpdateInterval` | `250` | 708996 |

### D.27 Factory 46691 — `l`

Source: `app.js`, object byte **714157**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `a`→factory 3038; `r`→factory 44548; `o`→factory 46491; `s`→factory 73969.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `l.titleFormat.typography` | `Typography.Standard.Title2` | 714190 |
| `l.titleFormat.color` | `Color.White` | 714216 |
| `l.titleFormat.textAlignment` | `.5` | 714241 |
| `l.bodyFormat.typography` | `Typography.Standard.Body` | 714277 |
| `l.bodyFormat.color` | `Color.LabelSecondary` | 714301 |
| `l.bodyFormat.textAlignment` | `0` | 714335 |
| `l.infoFormat.typography` | `Typography.Standard.Headline` | 714370 |
| `l.infoFormat.textAlignment` | `0` | 714406 |
| `l.page.width` | `S(640)` | 714422 |
| `l.page.padding.top` | `S(60)` | 714449 |
| `l.title.padding.bottom` | `S(90)` | 714487 |
| `l.oneLiner.width` | `S(640)` | 714518 |
| `l.oneLiner.padding.bottom` | `S(80)` | 714548 |
| `l.footer.badge` | `s.R.commonSenseWithText(120,Color.White)` | 714577 |
| `l.footer.width` | `S(120)` | 714623 |
| `l.footer.height` | `S(32)` | 714644 |
| `l.footer.hairlineColor` | `Color.Semi50()` | 714671 |
| `l.footer.padding.top` | `S(85)` | 714698 |
| `l.footer.padding.bottom` | `S(45)` | 714718 |
| `l.badge.image` | `(e,t=new r.Q1(Color.LabelPrimary),i=(0,r.O4)())=>s.R.commonSense(e,t.getColor(i))` | 714746 |
| `l.badge.width` | `S(61)` | 714833 |
| `l.badge.height` | `S(54)` | 714853 |

### D.28 Factory 47733 — `R`

Source: `app.js`, object byte **721899**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 38594; `l`→factory 46491; `L`→factory 3038; `k`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `R.textWidth` | `S(960)` | 721910 |
| `R.titleFormat.typography` | `Typography.Standard.Headline` | 721956 |
| `R.titleFormat.color` | `Color.LabelPrimary` | 721984 |
| `R.titleFormat.textAlignment` | `.5` | 722016 |
| `R.bodyFormat.typography` | `Typography.Standard.Body` | 722052 |
| `R.bodyFormat.color` | `Color.LabelPrimary` | 722076 |
| `R.bodyFormat.textAlignment` | `.5` | 722108 |

### D.29 Factory 87821 — `m`

Source: `app.js`, object byte **735819**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 96184; `h`→factory 3038; `g`→factory 44548; `p`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.titleFormat.typography` | `Typography.Standard.Title2` | 735852 |
| `m.titleFormat.color` | `Color.White` | 735878 |
| `m.titleFormat.textAlignment` | `.5` | 735903 |
| `m.itemSpacing` | `S(40)` | 735920 |

### D.30 Factory 76885 — `y`

Source: `app.js`, object byte **777530**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 38594; `o`→factory 95977; `l`→factory 44548; `m`→factory 3038; `f`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `y.pageTitle.typography` | `Typography.Emphasized.Headline` | 777561 |
| `y.pageTitle.color` | `Color.LabelPrimary` | 777591 |
| `y.pageTitle.textAlignment` | `.5` | 777623 |
| `y.multilineText.titleSpacing` | `S(10)` | 777656 |
| `y.multilineText.horizontalBoxPadding` | `S(15)` | 777690 |
| `y.multilineText.verticalBoxPadding` | `S(15)` | 777722 |
| `y.multilineText.backgroundFocusColor.dark` | `new n.Gfx.Color(1,1,1,.4)` | 777762 |
| `y.multilineText.backgroundFocusColor.light` | `new n.Gfx.Color(1,1,1,.4)` | 777794 |
| `y.multilineText.backgroundUnfocusColor` | `new n.Gfx.Color(1,1,1,0)` | 777844 |
| `y.multilineText.moreTextStyle.subtitle.typography` | `Typography.Standard.Caption2` | 777913 |
| `y.multilineText.moreTextStyle.subtitle.color` | `Color.LabelPrimary` | 777941 |
| `y.multilineText.moreTextStyle.body.typography` | `Typography.Standard.Caption2` | 777986 |
| `y.multilineText.moreTextStyle.body.color` | `Color.LabelSecondary` | 778014 |
| `y.multilineText.moreTextStyle.ellipsis.typography` | `Typography.Standard.Caption2` | 778065 |
| `y.multilineText.moreTextStyle.ellipsis.color` | `Color.LabelPrimary` | 778093 |
| `y.multilineText.moreTextExpandedStyle.subtitle.typography` | `Typography.Standard.Caption2` | 778166 |
| `y.multilineText.moreTextExpandedStyle.subtitle.color` | `Color.LabelPrimary` | 778194 |
| `y.multilineText.moreTextExpandedStyle.body.typography` | `Typography.Standard.Caption2` | 778239 |
| `y.multilineText.moreTextExpandedStyle.body.color` | `Color.LabelSecondary` | 778267 |

### D.31 Factory 45643 — `g`

Source: `app.js`, object byte **780141**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 97580; `c`→factory 3038; `u`→factory 44548; `h`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `g.titleFormat.typography` | `Typography.Standard.Title2` | 780174 |
| `g.titleFormat.color` | `Color.White` | 780200 |
| `g.titleFormat.textAlignment` | `.5` | 780225 |
| `g.bodyFormat.typography` | `Typography.Standard.Body` | 780261 |
| `g.bodyFormat.color` | `Color.LabelSecondary` | 780285 |
| `g.bodyFormat.textAlignment` | `0` | 780319 |
| `g.infoFormat.typography` | `Typography.Standard.Headline` | 780354 |
| `g.titlePadding` | `S(90)` | 780391 |
| `g.freshnessLogoPointSize` | `61` | 780427 |
| `g.freshnessLogoSpacing` | `S(5)` | 780451 |
| `g.page.width` | `S(640)` | 780475 |
| `g.page.height` | `S(640)` | 780496 |

### D.32 Factory 69169 — `C`

Source: `app.js`, object byte **784785**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `o`→factory 90180; `S`→factory 3038; `w`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.title.padding.top` | `S(200)` | 784806 |
| `C.title.padding.bottom` | `S(110)` | 784827 |
| `C.title.format.typography` | `Typography.Emphasized.Title3` | 784869 |
| `C.title.format.color` | `Color.LabelPrimary` | 784897 |
| `C.title.format.textAlignment` | `.5` | 784929 |
| `C.groups.width` | `6*S(250)+5*S(48)` | 784949 |
| `C.groups.padding.bottomNoNextTitle` | `S(55)` | 785007 |
| `C.groups.padding.bottomLast` | `S(100)` | 785031 |
| `C.groups.title.padding.top` | `S(50)` | 785066 |
| `C.groups.title.padding.bottom` | `S(20)` | 785086 |
| `C.groups.title.format.typography` | `Typography.Emphasized.Subtitle1` | 785127 |
| `C.groups.title.format.color` | `Color.LabelPrimary` | 785158 |
| `C.groups.title.format.textAlignment` | `.5` | 785190 |
| `C.spacing.imageToImage` | `S(48)` | 785219 |
| `C.spacing.rowToRow` | `S(55)` | 785241 |

### D.33 Factory 97963 — `O`

Source: `app.js`, object byte **853362**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 92687; `S`→factory 44548; `R`→factory 38594; `E`→factory 16393; `F`→factory 3038; `M`→factory 46491; `_`→factory 73969.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `O.appleTVPlusLogo.[S.Sx.dark]` | `_.R.appleTV(174,Color.Black)` | 853392 |
| `O.appleTVPlusLogo.[S.Sx.light]` | `_.R.appleTV(174,Color.White)` | 853433 |
| `O.appleTVChannelLogo.[S.Sx.dark]` | `_.R.appleTVChannels(188,Color.Black)` | 853494 |
| `O.appleTVChannelLogo.[S.Sx.light]` | `_.R.appleTVChannels(188,Color.White)` | 853543 |
| `O.channelLogo.maxWidth` | `S(330)` | 853602 |
| `O.channelLogo.maxHeight` | `S(66)` | 853626 |
| `O.titleFormat` | `(e=(0,S.O4)())=>new E.l({typography:Typography.Emphasized.Callout,color:Color.LabelPrimary(e),textAlignment:.5})` | 853652 |
| `O.disclaimer` | `(e=(0,S.O4)())=>new E.l({typography:Typography.Standard.Caption2,color:Color.LabelSecondary(e),textAlignment:.5})` | 853768 |
| `O.descriptionFormat` | `(e=(0,S.O4)())=>new E.l({typography:Typography.Emphasized.Caption1,color:Color.LabelTertiary(e),textAlignment:.5,vibrancyEffectStyle:"tertiary"})` | 853892 |
| `O.flex.buttonSpacing` | `S(14)` | 854050 |
| `O.flex.titleSpacing` | `S(16)` | 854076 |
| `O.flex.topSpacing` | `S(48)` | 854100 |
| `O.flex.footerSpacing` | `S(48)` | 854127 |
| `O.upsellWidgetWidth` | `S(548)` | 854159 |
| `O.upsellPadding.leading` | `S(60)` | 854196 |
| `O.upsellPadding.trailing` | `S(60)` | 854218 |
| `O.extraCustomDisclaimerBaselinePadding` | `S(87)` | 854269 |
| `O.platter.blurRadius` | `S(100)` | 854302 |
| `O.platter.baseColor` | `e=>e===S.Sx.dark?new(D()).Gfx.Color(Color.Black):Color.colorFromRGB(232,232,232)` | 854326 |
| `O.platter.blurAlpha` | `.4` | 854415 |
| `O.platter.fallbackAlpha` | `.7` | 854432 |
| `O.platter.cornerRadius` | `S(12)` | 854448 |

### D.34 Factory 61202 — `e$`

Source: `app.js`, object byte **913628**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `eZ`→factory 80816.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e$.name` | `eZ._.PresetName.TransparentBackground` | 913634 |
| `e$.description` | `"ATVE.Settings.Accessibility.SubtitlesAndCaptioning.Preset.TransparentBackground"` | 913684 |
| `e$.font` | `eZ._.Font.SanFranciscoMedium` | 913771 |
| `e$.monospacedSerifFont` | `eZ._.Font.Courier` | 913820 |
| `e$.serifFont` | `eZ._.Font.Courier` | 913848 |
| `e$.monospacedSansSerifFont` | `eZ._.Font.Menlo` | 913890 |
| `e$.sansSerifFont` | `eZ._.Font.SanFranciscoMedium` | 913920 |
| `e$.casualFont` | `eZ._.Font.Avenir` | 913960 |
| `e$.scriptFont` | `eZ._.Font.SanFranciscoMedium` | 913988 |
| `e$.smallCapsFont` | `eZ._.Font.Copperplate` | 914031 |
| `e$.size` | `eZ._.Size.Medium` | 914058 |
| `e$.textColor` | `eZ._.Color.White` | 914085 |
| `e$.backgroundColor` | `eZ._.Color.Black` | 914118 |
| `e$.backgroundOpacity` | `eZ._.Opacity.Fifty` | 914153 |
| `e$.textOpacity` | `eZ._.TextOpacity.Opaque` | 914184 |
| `e$.edgeStyle` | `eZ._.EdgeStyle.None` | 914218 |
| `e$.highlightColor` | `eZ._.Color.Black` | 914253 |
| `e$.highlightOpacity` | `eZ._.Opacity.Zero` | 914287 |
| `e$.userCreated` | `!1` | 914317 |
| `e$.videoShouldOverrideBackgroundOpacity` | `!1` | 914357 |
| `e$.videoShouldOverrideBackgroundColor` | `!0` | 914395 |
| `e$.videoShouldOverrideEdgeStyle` | `!0` | 914427 |
| `e$.videoShouldOverrideFont` | `!0` | 914454 |
| `e$.videoShouldOverrideSize` | `!0` | 914481 |
| `e$.videoShouldOverrideTextColor` | `!0` | 914513 |
| `e$.videoShouldOverrideTextHighlight` | `!0` | 914549 |
| `e$.videoShouldOverrideTextOpacity` | `!0` | 914583 |
| `e$.uniqueId` | `1` | 914595 |
| `e$.cornerRadius` | `S(12)` | 914610 |

### D.35 Factory 61202 — `e0`

Source: `app.js`, object byte **914628**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `eZ`→factory 80816.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e0.name` | `eZ._.PresetName.LargeText` | 914634 |
| `e0.description` | `"ATVE.Settings.Accessibility.SubtitlesAndCaptioning.Preset.LargeText"` | 914672 |
| `e0.font` | `eZ._.Font.Helvetica` | 914747 |
| `e0.monospacedSerifFont` | `eZ._.Font.Courier` | 914787 |
| `e0.serifFont` | `eZ._.Font.Courier` | 914815 |
| `e0.monospacedSansSerifFont` | `eZ._.Font.Menlo` | 914857 |
| `e0.sansSerifFont` | `eZ._.Font.Helvetica` | 914887 |
| `e0.casualFont` | `eZ._.Font.Avenir` | 914918 |
| `e0.scriptFont` | `eZ._.Font.SanFranciscoMedium` | 914946 |
| `e0.smallCapsFont` | `eZ._.Font.Copperplate` | 914989 |
| `e0.size` | `eZ._.Size.Large` | 915016 |
| `e0.textColor` | `eZ._.Color.White` | 915042 |
| `e0.backgroundColor` | `eZ._.Color.Black` | 915075 |
| `e0.backgroundOpacity` | `eZ._.Opacity.Fifty` | 915110 |
| `e0.textOpacity` | `eZ._.TextOpacity.Opaque` | 915141 |
| `e0.edgeStyle` | `eZ._.EdgeStyle.None` | 915175 |
| `e0.highlightColor` | `eZ._.Color.Black` | 915210 |
| `e0.highlightOpacity` | `eZ._.Opacity.Zero` | 915244 |
| `e0.userCreated` | `!1` | 915274 |
| `e0.videoShouldOverrideBackgroundOpacity` | `!1` | 915314 |
| `e0.videoShouldOverrideBackgroundColor` | `!0` | 915352 |
| `e0.videoShouldOverrideEdgeStyle` | `!0` | 915384 |
| `e0.videoShouldOverrideFont` | `!0` | 915411 |
| `e0.videoShouldOverrideSize` | `!0` | 915438 |
| `e0.videoShouldOverrideTextColor` | `!0` | 915470 |
| `e0.videoShouldOverrideTextHighlight` | `!0` | 915506 |
| `e0.videoShouldOverrideTextOpacity` | `!0` | 915540 |
| `e0.uniqueId` | `2` | 915552 |
| `e0.cornerRadius` | `S(12)` | 915567 |

### D.36 Factory 61202 — `e1`

Source: `app.js`, object byte **915585**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eZ`→factory 80816.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e1.name` | `eZ._.PresetName.Classic` | 915591 |
| `e1.description` | `"ATVE.Settings.Accessibility.SubtitlesAndCaptioning.Preset.Classic"` | 915627 |
| `e1.font` | `eZ._.Font.Courier` | 915700 |
| `e1.monospacedSerifFont` | `eZ._.Font.Courier` | 915738 |
| `e1.serifFont` | `eZ._.Font.Courier` | 915766 |
| `e1.monospacedSansSerifFont` | `eZ._.Font.Menlo` | 915808 |
| `e1.sansSerifFont` | `eZ._.Font.SanFranciscoMedium` | 915838 |
| `e1.casualFont` | `eZ._.Font.Avenir` | 915878 |
| `e1.scriptFont` | `eZ._.Font.SanFranciscoMedium` | 915906 |
| `e1.smallCapsFont` | `eZ._.Font.Copperplate` | 915949 |
| `e1.size` | `eZ._.Size.Medium` | 915976 |
| `e1.textColor` | `eZ._.Color.White` | 916003 |
| `e1.backgroundColor` | `eZ._.Color.Black` | 916036 |
| `e1.backgroundOpacity` | `eZ._.Opacity.Zero` | 916071 |
| `e1.textOpacity` | `eZ._.TextOpacity.Opaque` | 916101 |
| `e1.edgeStyle` | `eZ._.EdgeStyle.None` | 916135 |
| `e1.highlightColor` | `eZ._.Color.Black` | 916170 |
| `e1.highlightOpacity` | `eZ._.Opacity.Hundred` | 916204 |
| `e1.userCreated` | `!1` | 916237 |
| `e1.videoShouldOverrideBackgroundOpacity` | `!1` | 916277 |
| `e1.videoShouldOverrideBackgroundColor` | `!0` | 916315 |
| `e1.videoShouldOverrideEdgeStyle` | `!0` | 916347 |
| `e1.videoShouldOverrideFont` | `!0` | 916374 |
| `e1.videoShouldOverrideSize` | `!0` | 916401 |
| `e1.videoShouldOverrideTextColor` | `!0` | 916433 |
| `e1.videoShouldOverrideTextHighlight` | `!0` | 916469 |
| `e1.videoShouldOverrideTextOpacity` | `!0` | 916503 |
| `e1.uniqueId` | `3` | 916515 |
| `e1.cornerRadius` | `0` | 916530 |

### D.37 Factory 61202 — `e2`

Source: `app.js`, object byte **916536**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eZ`→factory 80816.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e2.name` | `eZ._.PresetName.OutlineText` | 916542 |
| `e2.description` | `"ATVE.Settings.Accessibility.SubtitlesAndCaptioning.Preset.OutlineText"` | 916582 |
| `e2.font` | `eZ._.Font.SanFranciscoMedium` | 916659 |
| `e2.monospacedSerifFont` | `eZ._.Font.Courier` | 916708 |
| `e2.serifFont` | `eZ._.Font.Courier` | 916736 |
| `e2.monospacedSansSerifFont` | `eZ._.Font.Menlo` | 916778 |
| `e2.sansSerifFont` | `eZ._.Font.SanFranciscoMedium` | 916808 |
| `e2.casualFont` | `eZ._.Font.Avenir` | 916848 |
| `e2.scriptFont` | `eZ._.Font.SanFranciscoMedium` | 916876 |
| `e2.smallCapsFont` | `eZ._.Font.Copperplate` | 916919 |
| `e2.size` | `eZ._.Size.Large` | 916946 |
| `e2.textColor` | `eZ._.Color.White` | 916972 |
| `e2.backgroundColor` | `eZ._.Color.Black` | 917005 |
| `e2.backgroundOpacity` | `eZ._.Opacity.Zero` | 917040 |
| `e2.textOpacity` | `eZ._.TextOpacity.Opaque` | 917070 |
| `e2.edgeStyle` | `eZ._.EdgeStyle.Uniform` | 917104 |
| `e2.highlightColor` | `eZ._.Color.Black` | 917142 |
| `e2.highlightOpacity` | `eZ._.Opacity.Zero` | 917176 |
| `e2.userCreated` | `!1` | 917206 |
| `e2.videoShouldOverrideBackgroundOpacity` | `!1` | 917246 |
| `e2.videoShouldOverrideBackgroundColor` | `!0` | 917284 |
| `e2.videoShouldOverrideEdgeStyle` | `!0` | 917316 |
| `e2.videoShouldOverrideFont` | `!0` | 917343 |
| `e2.videoShouldOverrideSize` | `!0` | 917370 |
| `e2.videoShouldOverrideTextColor` | `!0` | 917402 |
| `e2.videoShouldOverrideTextHighlight` | `!0` | 917438 |
| `e2.videoShouldOverrideTextOpacity` | `!0` | 917472 |
| `e2.uniqueId` | `4` | 917484 |
| `e2.cornerRadius` | `0` | 917499 |

### D.38 Factory 61202 — `e3`

Source: `app.js`, object byte **917505**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eZ`→factory 80816.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `e3.name` | `eZ._.PresetName.OutlineText` | 917511 |
| `e3.description` | `"ATVE.Settings.Accessibility.SubtitlesAndCaptioning.Preset.OutlineText"` | 917551 |
| `e3.font` | `eZ._.Font.SanFranciscoMedium` | 917628 |
| `e3.monospacedSerifFont` | `eZ._.Font.Courier` | 917677 |
| `e3.serifFont` | `eZ._.Font.Courier` | 917705 |
| `e3.monospacedSansSerifFont` | `eZ._.Font.Menlo` | 917747 |
| `e3.sansSerifFont` | `eZ._.Font.SanFranciscoMedium` | 917777 |
| `e3.casualFont` | `eZ._.Font.Avenir` | 917817 |
| `e3.scriptFont` | `eZ._.Font.SanFranciscoMedium` | 917845 |
| `e3.smallCapsFont` | `eZ._.Font.Copperplate` | 917888 |
| `e3.size` | `eZ._.Size.Medium` | 917915 |
| `e3.textColor` | `eZ._.Color.White` | 917942 |
| `e3.backgroundColor` | `eZ._.Color.Black` | 917975 |
| `e3.backgroundOpacity` | `eZ._.Opacity.Zero` | 918010 |
| `e3.textOpacity` | `eZ._.TextOpacity.Opaque` | 918040 |
| `e3.edgeStyle` | `eZ._.EdgeStyle.Uniform` | 918074 |
| `e3.highlightColor` | `eZ._.Color.Black` | 918112 |
| `e3.highlightOpacity` | `eZ._.Opacity.Zero` | 918146 |
| `e3.userCreated` | `!1` | 918176 |
| `e3.videoShouldOverrideBackgroundOpacity` | `!1` | 918216 |
| `e3.videoShouldOverrideBackgroundColor` | `!0` | 918254 |
| `e3.videoShouldOverrideEdgeStyle` | `!0` | 918286 |
| `e3.videoShouldOverrideFont` | `!0` | 918313 |
| `e3.videoShouldOverrideSize` | `!0` | 918340 |
| `e3.videoShouldOverrideTextColor` | `!0` | 918372 |
| `e3.videoShouldOverrideTextHighlight` | `!0` | 918408 |
| `e3.videoShouldOverrideTextOpacity` | `!0` | 918442 |
| `e3.uniqueId` | `5` | 918454 |
| `e3.cornerRadius` | `0` | 918469 |

### D.39 Factory 61202 — `ie`

Source: `app.js`, object byte **977204**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `ie.padding.leading` | `S(190)` | 977222 |
| `ie.padding.trailing` | `S(225)` | 977246 |
| `ie.sizes.informationWidth` | `S(760)` | 977286 |
| `ie.sizes.buttonWidth` | `S(530)` | 977313 |
| `ie.spacings.informationToButtons` | `S(250)` | 977360 |
| `ie.spacings.buttons` | `S(14)` | 977383 |

### D.40 Factory 61202 — `iR`

Source: `app.js`, object byte **1007794**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `ta`→factory 96638; `t0`→factory 44548; `t2`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `iR.margin.vertical` | `S(74)` | 1007812 |
| `iR.textPadding.right` | `S(20)` | 1007846 |
| `iR.iconPadding.left` | `S(20)` | 1007879 |
| `iR.iconPadding.right` | `S(10)` | 1007899 |
| `iR.iconPadding.top` | `S(14)` | 1007917 |
| `iR.iconPadding.bottom` | `S(14)` | 1007938 |
| `iR.icon.width` | `S(38)` | 1007965 |
| `iR.icon.height` | `S(38)` | 1007986 |
| `iR.getBackgroundColor` | `()=>ta.O.accessibility.reduceTransparency()?Color.Black:Color.Black60` | 1008020 |
| `iR.text.typography` | `Typography.Standard.Caption1` | 1008116 |
| `iR.text.color` | `Color.LabelPrimary` | 1008145 |
| `iR.text.paragraphSpacingAfter` | `0` | 1008186 |
| `iR.text.pageAlignment` | `.5` | 1008202 |
| `iR.cornerRadius` | `S(30)` | 1008220 |

### D.41 Factory 61202 — `nt`

Source: `app.js`, object byte **1018341**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `t0`→factory 44548; `t2`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `nt.platter.width` | `S(900)` | 1018357 |
| `nt.platter.height` | `S(845)` | 1018379 |
| `nt.content.padding.top` | `S(72)` | 1018417 |
| `nt.content.padding.trailing` | `S(80)` | 1018440 |
| `nt.content.padding.bottom` | `S(80)` | 1018461 |
| `nt.content.padding.leading` | `S(80)` | 1018483 |
| `nt.content.titleSpacing` | `S(3)` | 1018511 |
| `nt.content.metaDataLine.padding.leading` | `S(80)` | 1018555 |
| `nt.content.metaDataLine.padding.trailing` | `S(80)` | 1018578 |
| `nt.content.metaDataLine.padding.top` | `S(13)` | 1018596 |
| `nt.content.subHeader.padding.top` | `S(25)` | 1018636 |
| `nt.content.description.padding.top` | `S(32)` | 1018678 |
| `nt.content.accessibility.image.padding.top` | `S(24)` | 1018729 |
| `nt.content.accessibility.spacing` | `S(4)` | 1018753 |
| `nt.content.widget.padding.trailing` | `S(80)` | 1018793 |
| `nt.content.widget.padding.leading` | `S(80)` | 1018815 |
| `nt.content.title.format.typography` | `Typography.Emphasized.Headline` | 1018866 |
| `nt.content.title.format.color` | `Color.LabelPrimary` | 1018897 |
| `nt.content.title.format.pageAlignment` | `.5` | 1018930 |
| `nt.content.subHeading.typography` | `Typography.Standard.Caption1` | 1018968 |
| `nt.content.subHeading.color` | `Color.LabelPrimary` | 1018997 |
| `nt.content.subHeading.pageAlignment` | `0` | 1019030 |
| `nt.content.subContent.typography` | `Typography.Standard.Caption1` | 1019066 |
| `nt.content.subContent.color` | `Color.LabelSecondary` | 1019095 |
| `nt.content.subContent.pageAlignment` | `0` | 1019130 |
| `nt.content.header.format.typography` | `Typography.Emphasized.Callout` | 1019170 |
| `nt.content.header.format.color` | `Color.LabelPrimary` | 1019200 |
| `nt.content.header.format.pageAlignment` | `0` | 1019233 |
| `nt.content.header.padding.top` | `S(32)` | 1019250 |
| `nt.content.descriptionTitle.typography` | `Typography.Emphasized.Callout` | 1019304 |
| `nt.content.descriptionTitle.color` | `Color.LabelPrimary` | 1019334 |
| `nt.content.descriptionTitle.textAlignment` | `0` | 1019367 |
| `nt.content.descriptionTitle.pageAlignment` | `1` | 1019383 |
| `nt.content.descriptionTitle.paragraphSpacingAfter` | `0` | 1019407 |
| `nt.content.default.typography` | `Typography.Standard.Callout` | 1019440 |
| `nt.content.default.color` | `Color.LabelPrimary` | 1019468 |
| `nt.content.default.textAlignment` | `0` | 1019501 |
| `nt.content.default.pageAlignment` | `1` | 1019517 |
| `nt.content.default.paragraphSpacingAfter` | `0` | 1019541 |
| `nt.content.fadeOutArgs.top.inner` | `S(80)` | 1019569 |
| `nt.content.fadeOutArgs.top.outer` | `S(-30)` | 1019589 |
| `nt.content.fadeOutArgs.top.ellipsis` | `!0` | 1019613 |
| `nt.content.fadeOutArgs.top.fadeWithScroll` | `S(50)` | 1019631 |
| `nt.content.fadeOutArgs.top.clipExpand` | `S(0)` | 1019656 |
| `nt.content.fadeOutArgs.bottom.inner` | `S(80)` | 1019684 |
| `nt.content.fadeOutArgs.bottom.outer` | `S(-30)` | 1019704 |
| `nt.content.fadeOutArgs.bottom.ellipsis` | `!0` | 1019728 |
| `nt.content.fadeOutArgs.bottom.fadeWithScroll` | `S(50)` | 1019746 |
| `nt.content.fadeOutArgs.bottom.clipExpand` | `S(0)` | 1019771 |
| `nt.content.axCursor.padding.default.leading` | `S(12)` | 1019822 |
| `nt.content.axCursor.padding.default.trailing` | `S(12)` | 1019845 |
| `nt.content.axCursor.padding.default.top` | `S(-13)` | 1019863 |
| `nt.content.axCursor.padding.default.bottom` | `S(12)` | 1019885 |
| `nt.content.axCursor.padding.header.leading` | `S(10)` | 1019916 |
| `nt.content.axCursor.padding.header.trailing` | `S(10)` | 1019939 |
| `nt.content.axCursor.padding.header.top` | `S(-20)` | 1019957 |
| `nt.content.axCursor.padding.header.bottom` | `S(10)` | 1019979 |
| `nt.content.axCursor.padding.title.leading` | `S(12)` | 1020009 |
| `nt.content.axCursor.padding.title.trailing` | `S(12)` | 1020032 |
| `nt.content.axCursor.padding.title.top` | `S(-62)` | 1020050 |
| `nt.content.axCursor.padding.title.bottom` | `S(12)` | 1020072 |

### D.42 Factory 61202 — `nb`

Source: `app.js`, object byte **1035522**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `nb.image.height` | `S(60)` | 1035537 |
| `nb.image.width` | `S(600)` | 1035557 |
| `nb.image.fadeInDuration` | `300` | 1035587 |

### D.43 Factory 61202 — `aF`

Source: `app.js`, object byte **1088864**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `ta`→factory 96638; `tJ`→factory 50429; `t0`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `aF.pageBackground` | `Color.Black30` | 1088880 |
| `aF.transition.duration` | `250` | 1088915 |
| `aF.transition.scaling` | `.75` | 1088927 |
| `aF.platter.width` | `S(860)` | 1088947 |
| `aF.platter.heightWithLegend` | `S(920)` | 1088979 |
| `aF.platter.heightWithoutLegend` | `S(845)` | 1089014 |
| `aF.platter.backgroundColor` | `()=>Color.SystemGray4()` | 1089045 |
| `aF.platter.borderColor` | `()=>tJ.yP.from(ta.O.accessibility.focusStyle?Color.borderColorHighContrast():Color.borderColor())` | 1089081 |
| `aF.platter.borderWidth` | `S(1)` | 1089191 |
| `aF.platter.shadow.dx` | `0` | 1089215 |
| `aF.platter.shadow.dy` | `S(40)` | 1089220 |
| `aF.platter.shadow.color` | `Color.Black30` | 1089240 |
| `aF.platter.shadow.blurRadius` | `S(50)` | 1089265 |
| `aF.platter.cornerRadius` | `S(32)` | 1089293 |
| `aF.content.listWidth` | `S(860)` | 1089327 |
| `aF.content.listHeight` | `S(685)` | 1089353 |
| `aF.content.teamRowHeight` | `S(72)` | 1089382 |
| `aF.content.dividerHeight` | `S(4)` | 1089410 |
| `aF.content.listPadding.top` | `S(2)` | 1089440 |
| `aF.content.listPadding.bottom` | `S(20)` | 1089460 |
| `aF.content.listScrollArgs.pointerConfig.enabled` | `!0` | 1089514 |
| `aF.content.listScrollArgs.top` | `S(224)` | 1089522 |
| `aF.content.listScrollArgs.bottom` | `S(224)` | 1089544 |
| `aF.content.fadeOutArgs.top.outer` | `-S(6)` | 1089584 |
| `aF.content.fadeOutArgs.top.inner` | `S(74)` | 1089604 |
| `aF.content.fadeOutArgs.top.ellipsis` | `!0` | 1089627 |
| `aF.content.fadeOutArgs.top.clipExpand` | `S(50)` | 1089641 |
| `aF.content.fadeOutArgs.top.fadeWithScroll` | `S(50)` | 1089670 |
| `aF.content.fadeOutArgs.top.orthogonalMargin` | `S(100)` | 1089701 |
| `aF.content.fadeOutArgs.bottom.outer` | `S(0)` | 1089731 |
| `aF.content.fadeOutArgs.bottom.inner` | `S(106)` | 1089750 |
| `aF.content.fadeOutArgs.bottom.ellipsis` | `!0` | 1089774 |
| `aF.content.fadeOutArgs.bottom.fadeWithScroll` | `S(50)` | 1089792 |
| `aF.content.fadeOutArgs.bottom.clipExpand` | `S(50)` | 1089817 |
| `aF.content.fadeOutArgs.bottom.orthogonalMargin` | `S(100)` | 1089848 |
| `aF.footer.height` | `S(75)` | 1089881 |
| `aF.footer.dividerColor` | `()=>tJ.yP.from(ta.O.accessibility.focusStyle?Color.borderColorHighContrast():Color.borderColor())` | 1089908 |
| `aF.footer.dividerHeight` | `S(1)` | 1090020 |
| `aF.footer.padding.top` | `S(19)` | 1090046 |
| `aF.footer.padding.bottom` | `S(26)` | 1090067 |

### D.44 Factory 61202 — `aG`

Source: `app.js`, object byte **1090852**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `tB`→factory 78038; `tJ`→factory 50429; `t0`→factory 44548; `t2`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `aG.slideShow.autoAdvanceDelay` | `1e4` | 1090881 |
| `aG.slideShow.autoAdvanceAfterVideo` | `1e3` | 1090907 |
| `aG.slideShow.fadeInDuration` | `500` | 1090926 |
| `aG.slideShow.fadeOutDuration` | `600` | 1090946 |
| `aG.slideShow.videoStartDelay` | `500` | 1090966 |
| `aG.slideShow.foregroundFadeDelay` | `3e3` | 1090990 |
| `aG.slideShow.videoAlignment` | `.5` | 1091009 |
| `aG.slideShow.pageControlOffset` | `S(24)` | 1091030 |
| `aG.slideShow.protectionGradient.height` | `S(616)` | 1091071 |
| `aG.slideShow.protectionGradient.solidHeight` | `S(60)` | 1091098 |
| `aG.slideShow.protectionGradient.midPointOpacity` | `.5` | 1091128 |
| `aG.slideShow.foregroundFade.delay` | `3e3` | 1091154 |
| `aG.slideShow.foregroundFade.duration` | `300` | 1091167 |
| `aG.slideShow.foregroundFade.subtitleArea` | `new tB.r(eV.nl.SafeArea.padding.leading,eV.nl.SafeArea.padding.top,eV.nl.SafeArea.width,eV.nl.SafeArea.height-S(100))` | 1091184 |
| `aG.slideShow.scaleVideoToCover` | `!1` | 1091329 |
| `aG.header.format.typography` | `Typography.Emphasized.Body` | 1091369 |
| `aG.header.format.color` | `Color.LabelPrimary` | 1091396 |
| `aG.header.format.textAlignment` | `0` | 1091429 |
| `aG.header.format.shadow.dx` | `0` | 1091442 |
| `aG.header.format.shadow.dy` | `S(2)` | 1091447 |
| `aG.header.format.shadow.color` | `Color.Black50` | 1091466 |
| `aG.header.format.shadow.blurRadius` | `S(5)` | 1091491 |
| `aG.header.topToBaseline` | `S(85)` | 1091521 |
| `aG.flavor.padding.top` | `eV.nl.SafeArea.vertical` | 1091557 |
| `aG.flavor.padding.bottom` | `S(70)` | 1091588 |
| `aG.flavor.padding.leading` | `eV.nl.SafeArea.horizontal` | 1091610 |
| `aG.flavor.padding.trailing` | `eV.nl.SafeArea.horizontal` | 1091645 |
| `aG.flavor.spacing` | `S(6)` | 1091680 |
| `aG.flavor.width` | `S(900)` | 1091699 |
| `aG.flavor.titleFormat.typography` | `Typography.Emphasized.Caption1` | 1091747 |
| `aG.flavor.titleFormat.color` | `Color.LabelSecondary` | 1091778 |
| `aG.flavor.titleMaxLines` | `1` | 1091815 |
| `aG.flavor.subtitleFormat.typography` | `Typography.Emphasized.Callout` | 1091853 |
| `aG.flavor.subtitleFormat.color` | `Color.LabelPrimary` | 1091883 |
| `aG.flavor.subtitleMaxLines` | `2` | 1091921 |
| `aG.flavor.textOnlyGradient.stops` | `[{location:0,color:tJ.yP.black,alpha:0},{location:1,color:tJ.yP.black,alpha:.6}]` | 1091947 |
| `aG.flavor.textOnlyGradient.vector` | `[.09*eV.nl.width,.63*eV.nl.height,.04*eV.nl.width,eV.nl.height]` | 1092035 |
| `aG.trailersPlaylist.foregroundFade.delay` | `1e4` | 1092141 |
| `aG.trailersPlaylist.foregroundFade.duration` | `300` | 1092154 |
| `aG.trailersPlaylist.foregroundFade.subtitleArea` | `new tB.r(S(620),S(160),S(1220),S(740))` | 1092171 |
| `aG.trailersPlaylist.foregroundFadeIn.whenVideoDurationRemains` | `5e3` | 1092286 |

### D.45 Factory 61202 — `a9`

Source: `app.js`, object byte **1100420**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `a7`→factory 46827.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `a9.info.format` | `a7.qv.fonts.bodyRegular` | 1100434 |
| `a9.info.maxLines` | `3` | 1100467 |
| `a9.descriptionFormat` | `a7.qv.fonts.description` | 1100488 |
| `a9.fadeInTime` | `400` | 1100523 |

### D.46 Factory 61202 — `rg`

Source: `app.js`, object byte **1118308**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `eV`→factory 46491; `t0`→factory 44548; `t2`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `rg.platter.cornerRadius` | `S(30)` | 1118331 |
| `rg.platter.minWidth` | `S(200)` | 1118354 |
| `rg.platter.maxWidth` | `S(600)` | 1118378 |
| `rg.platter.minHeight` | `S(112)` | 1118403 |
| `rg.platter.padding.top` | `S(23)` | 1118431 |
| `rg.platter.padding.bottom` | `S(23)` | 1118452 |
| `rg.platter.padding.leading` | `S(30)` | 1118474 |
| `rg.platter.padding.trailing` | `S(40)` | 1118497 |
| `rg.platter.arrow.middle.width` | `S(58)` | 1118533 |
| `rg.platter.arrow.middle.height` | `S(18)` | 1118554 |
| `rg.platter.arrow.corner.width` | `S(54)` | 1118583 |
| `rg.platter.arrow.corner.height` | `S(54)` | 1118604 |
| `rg.platter.arrow.tipHeight` | `S(18)` | 1118629 |
| `rg.platter.arrow.bottomPadding` | `S(36)` | 1118657 |
| `rg.platter.color` | `Color.colorFromRGB(30,30,30,.9)` | 1118678 |
| `rg.formats.title.typography` | `Typography.Emphasized.Caption1` | 1118747 |
| `rg.formats.title.color` | `Color.LabelPrimary` | 1118778 |
| `rg.formats.title.paragraphSpacingAfter` | `0` | 1118819 |
| `rg.formats.title.pageAlignment` | `0` | 1118835 |
| `rg.formats.titleOnly.typography` | `Typography.Standard.Caption1` | 1118870 |
| `rg.formats.titleOnly.color` | `Color.LabelPrimary` | 1118899 |
| `rg.formats.titleOnly.paragraphSpacingAfter` | `0` | 1118940 |
| `rg.formats.titleOnly.pageAlignment` | `0` | 1118956 |
| `rg.formats.body.typography.(spread)` | `Typography.Standard.Caption2` | 1118990 |
| `rg.formats.body.typography.leading` | `26` | 1119021 |
| `rg.formats.body.color` | `Color.LabelSecondary` | 1119031 |
| `rg.formats.body.pageAlignment` | `0` | 1119066 |
| `rg.formats.body.paragraphSpacingAfter` | `0` | 1119090 |
| `rg.timing.easeInTime` | `500` | 1119114 |
| `rg.timing.easeOutTime` | `500` | 1119130 |
| `rg.timing.delay` | `500` | 1119140 |
| `rg.timing.showTime` | `7e3` | 1119153 |
| `rg.timing.quickEaseOutTime` | `200` | 1119174 |
| `rg.timing.animationTargetEaseIn.scale` | `1` | 1119207 |
| `rg.timing.animationTargetEaseIn.opacity` | `1` | 1119217 |
| `rg.timing.animationTargetEaseOut.scale` | `.8` | 1119250 |
| `rg.timing.animationTargetEaseOut.opacity` | `0` | 1119261 |

### D.47 Factory 60347 — `P`

Source: `app.js`, object byte **1380749**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 46491; `g`→factory 84184; `p`→factory 760; `m`→factory 96638.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `P.InfoTransition.transparent` | `!0` | 1380778 |
| `P.InfoTransition.push` | `()=>new BlurTransition({duration:500,scale:m.O.accessibility.getReduceMotion()?1:.9,fade:0,separation:1,backgroundDim:.35})` | 1380786 |
| `P.InfoTransition.pop` | `()=>new BlurTransition({duration:500,scale:m.O.accessibility.getReduceMotion()?1:.9,fade:0,separation:0,backgroundDim:.35})` | 1380914 |
| `P.InfoTransitionSimple.push` | `Computed rule; 557 source bytes; retained at source location` | 1381066 |
| `P.InfoTransitionSimple.pop` | `Computed rule; 585 source bytes; retained at source location` | 1381628 |
| `P.ModalTransition.push` | `Computed rule; 263 source bytes; retained at source location` | 1382237 |
| `P.ModalTransition.pop` | `Computed rule; 263 source bytes; retained at source location` | 1382505 |
| `P.BasicTransition.loader` | `Computed rule; 473 source bytes; retained at source location` | 1382794 |
| `P.BasicTransition.crossfade` | `(e=300,t=0,i=!1)=>new g.M({duration:e,separation:t,fadeBottom:i,curve:"linear"})` | 1383278 |
| `P.BasicTransition.slideAndFade` | `()=>new g.M({duration:600,separation:0,fadeBottom:!1,topStartingPosition:new p.V(0,.05),curve:"easeOut"})` | 1383372 |
| `P.BasicTransition.pop` | `Computed rule; 482 source bytes; retained at source location` | 1383482 |
| `P.BasicTransition.getBackgroundBlur` | `()=>m.O.accessibility.reduceTransparency()?S(1920):S(180)` | 1383983 |
| `P.PlayerTransition.crossfade` | `()=>new g.M({duration:300,separation:0,curve:"linear"})` | 1384084 |
| `P.ProductCardSwipe.push` | `Computed rule; 177 source bytes; retained at source location` | 1384164 |
| `P.ProductCardSwipe.pop` | `Computed rule; 178 source bytes; retained at source location` | 1384346 |
| `P.EpicStage.push` | `Computed rule; 656 source bytes; retained at source location` | 1384542 |
| `P.EpicStage.pop` | `Computed rule; 691 source bytes; retained at source location` | 1385203 |
| `P.EpicStageLivePreview.push` | `Computed rule; 1167 source bytes; retained at source location` | 1385923 |
| `P.PostPlay.push` | `Computed rule; 709 source bytes; retained at source location` | 1387107 |
| `P.PostPlay.pop` | `Computed rule; 743 source bytes; retained at source location` | 1387821 |
| `P.EpicStageCrossFade.push` | `Computed rule; 732 source bytes; retained at source location` | 1388591 |
| `P.EpicStageCrossFade.pop` | `Computed rule; 1039 source bytes; retained at source location` | 1389328 |
| `P.TrailersPlaylist.push` | `Computed method; original body retained at source location` | 1390387 |
| `P.TrailersPlaylist.pop` | `Computed method; original body retained at source location` | 1391099 |

### D.48 Factory 5651 — `C`

Source: `app.js`, object byte **1493352**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 44548; `o`→factory 84556; `S`→factory 3038; `w`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.Badge.absoluteWidth` | `25` | 1493374 |
| `C.Badge.color` | `new n.Q1(Color.LabelSecondary)` | 1493383 |
| `C.Badge.verticalAlignment` | `.5` | 1493431 |
| `C.Text.font.typography` | `Typography.Standard.Body` | 1493466 |
| `C.Text.font.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1493490 |
| `C.Text.font.paragraphSpacingAfter` | `0` | 1493559 |
| `C.Flex.spacing` | `S(5)` | 1493578 |
| `C.AccessibilityCursor.x` | `-S(5)` | 1493614 |
| `C.AccessibilityCursor.y` | `-S(5)` | 1493629 |
| `C.AccessibilityCursor.w` | `S(10)` | 1493644 |
| `C.AccessibilityCursor.h` | `S(10)` | 1493659 |

### D.49 Factory 15264 — `p`

Source: `app.js`, object byte **1498042**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `l`→factory 3038; `c`→factory 97291; `g`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.minWidth` | `S(300)` | 1498052 |
| `p.height` | `S(100)` | 1498073 |
| `p.chevron` | `e=>e===DirectionalHintWidget_DirectionalHintWidget.Direction.Up?c.A:u` | 1498095 |
| `p.chevronColor.idle` | `Color.Gray85` | 1498184 |
| `p.chevronColor.highlighted` | `Color.White` | 1498208 |
| `p.chevronSize.width` | `S(48)` | 1498239 |
| `p.chevronSize.height` | `S(16)` | 1498259 |
| `p.format.typography` | `Typography.Standard.Caption2` | 1498300 |
| `p.format.color` | `Color.Gray85` | 1498328 |
| `p.format.shadow.dx` | `0` | 1498351 |
| `p.format.shadow.dy` | `S(2)` | 1498356 |
| `p.format.shadow.color` | `Color.Black50` | 1498374 |
| `p.format.shadow.blurRadius` | `S(5)` | 1498398 |
| `p.textColor.idle` | `Color.Gray85` | 1498429 |
| `p.textColor.highlighted` | `Color.White` | 1498453 |
| `p.startOffset` | `e=>e===DirectionalHintWidget_DirectionalHintWidget.Direction.Up?S(20):-S(20)` | 1498477 |
| `p.spacing` | `S(10)` | 1498576 |
| `p.padding` | `e=>e===DirectionalHintWidget_DirectionalHintWidget.Direction.Up?{top:S(31)}:{bottom:S(31)}` | 1498597 |
| `p.fadeInChevron` | `500` | 1498716 |
| `p.fadeInChevronText` | `300` | 1498738 |
| `p.fadeOutChevron` | `150` | 1498757 |

### D.50 Factory 10751 — `s`

Source: `app.js`, object byte **1498858**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `a`→factory 90058; `r`→factory 44548; `o`→factory 301.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `s.imageFadeInDuration` | `240*!!a.w.Config.Performance.otherAnimationsEnabled` | 1498879 |
| `s.shadow.idle.dx` | `0` | 1498948 |
| `s.shadow.idle.dy` | `S(4)` | 1498953 |
| `s.shadow.idle.color` | `()=>Color.Black40` | 1498971 |
| `s.shadow.idle.blurRadius` | `o.x.C.cornerRadius` | 1498999 |
| `s.shadow.focused.dx` | `0` | 1499031 |
| `s.shadow.focused.dy` | `S(40)` | 1499036 |
| `s.shadow.focused.color` | `()=>Color.Black30` | 1499055 |
| `s.shadow.focused.blurRadius` | `S(50)` | 1499083 |
| `s.fixedShadow.idle.dx` | `0` | 1499120 |
| `s.fixedShadow.idle.dy` | `S(4)` | 1499125 |
| `s.fixedShadow.idle.color` | `()=>Color.Black40` | 1499143 |
| `s.fixedShadow.idle.blurRadius` | `o.x.C.cornerRadius` | 1499171 |
| `s.outlineColor` | `()=>Color.borderColor()` | 1499205 |

### D.51 Factory 16501 — `C`

Source: `app.js`, object byte **1509325**. Values are static source declarations; offsets in the last column locate each original value.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.startDelay` | `1500` | 1509337 |
| `C.fadeInDuration` | `500` | 1509357 |
| `C.fadeOutDuration` | `250` | 1509377 |

### D.52 Factory 40126 — `f`

Source: `app.js`, object byte **1509575**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 5641; `h`→factory 46491; `p`→factory 3038; `m`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `f.chinHeight` | `S(30)` | 1509587 |
| `f.title.format.typography` | `Typography.Standard.Caption3` | 1509634 |
| `f.title.format.color` | `Color.LabelPrimary` | 1509662 |
| `f.title.maxLines` | `1` | 1509691 |
| `f.title.padding.horizontal` | `S(20)` | 1509713 |
| `f.title.verticalShift` | `S(16)` | 1509741 |
| `f.title.marqueeStyle.(spread)` | `u.b` | 1509771 |
| `f.title.marqueeStyle.spacing` | `S(60)` | 1509783 |

### D.53 Factory 26794 — `S`

Source: `app.js`, object byte **1517206**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `p`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `S.width` | `S(118)` | 1517213 |
| `S.height` | `S(16)` | 1517234 |
| `S.spacing` | `S(13)` | 1517255 |

### D.54 Factory 57214 — `g`

Source: `app.js`, object byte **1518434**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `g.backgroundColor` | `Color.Black30` | 1518451 |
| `g.borderColor` | `Color.colorWithAlpha(Color.White,.2)` | 1518476 |
| `g.shadowColor` | `Color.colorWithAlpha(Color.White,.2)` | 1518523 |

### D.55 Factory 57214 — `p`

Source: `app.js`, object byte **1518561**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.backgroundColor` | `Color.Black50` | 1518578 |
| `p.borderColor` | `Color.colorWithAlpha(Color.White,.2)` | 1518603 |
| `p.shadowColor` | `Color.colorWithAlpha(Color.White,.2)` | 1518650 |

### D.56 Factory 57214 — `m`

Source: `app.js`, object byte **1518688**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.backgroundColor` | `Color.SystemPink()` | 1518705 |
| `m.borderColor` | `Color.colorWithAlpha(Color.White,.2)` | 1518735 |
| `m.shadowColor` | `Color.colorFromRGB(255,45,85,.2)` | 1518782 |

### D.57 Factory 57214 — `f`

Source: `app.js`, object byte **1518817**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 12146; `l`→factory 46491; `u`→factory 3038; `h`→factory 55242.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `f.padding.horizontal` | `S(14)` | 1518838 |
| `f.height` | `S(42)` | 1518859 |
| `f.cornerRadius` | `S(17)` | 1518885 |
| `f.borderWidth` | `S(2)` | 1518910 |
| `f.textFormat.typography.(spread)` | `Typography.Standard.Caption1` | 1518957 |
| `f.textFormat.typography.size` | `23` | 1518984 |
| `f.textFormat.typography.weight` | `h.I.Semibold` | 1518994 |
| `f.iconSize` | `S(17)` | 1519019 |
| `f.iconPadding` | `S(7)` | 1519044 |
| `f.shadowBlurRadius` | `S(12)` | 1519073 |

### D.58 Factory 57214 — `y`

Source: `app.js`, object byte **1519089**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 12146; `l`→factory 46491; `u`→factory 3038; `h`→factory 55242.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `y.padding.horizontal` | `S(11)` | 1519110 |
| `y.height` | `S(34)` | 1519131 |
| `y.cornerRadius` | `S(14)` | 1519157 |
| `y.borderWidth` | `S(1.5)` | 1519182 |
| `y.textFormat.typography.(spread)` | `Typography.Standard.Caption1` | 1519231 |
| `y.textFormat.typography.size` | `21` | 1519258 |
| `y.textFormat.typography.weight` | `h.I.Semibold` | 1519268 |
| `y.iconSize` | `S(16)` | 1519293 |
| `y.iconPadding` | `S(6)` | 1519318 |
| `y.shadowBlurRadius` | `S(12)` | 1519347 |

### D.59 Factory 57214 — `v`

Source: `app.js`, object byte **1519363**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 12146; `l`→factory 46491; `u`→factory 3038; `h`→factory 55242.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `v.padding.horizontal` | `S(9)` | 1519384 |
| `v.height` | `S(26)` | 1519404 |
| `v.cornerRadius` | `S(11)` | 1519430 |
| `v.borderWidth` | `S(1)` | 1519455 |
| `v.textFormat.typography.(spread)` | `Typography.Standard.Caption1` | 1519502 |
| `v.textFormat.typography.size` | `16` | 1519529 |
| `v.textFormat.typography.weight` | `h.I.Semibold` | 1519539 |
| `v.iconSize` | `S(13)` | 1519564 |
| `v.iconPadding` | `S(5)` | 1519589 |
| `v.shadowBlurRadius` | `S(8)` | 1519618 |

### D.60 Factory 70982 — `p`

Source: `app.js`, object byte **1540358**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `a`→factory 50429; `s`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.ratingHeight` | `S(22)` | 1540372 |
| `p.ratingSpacing` | `S(4)` | 1540399 |
| `p.attributionOutline.thickness` | `S(1)` | 1540441 |
| `p.attributionOutline.color` | `a.yP.from(Color.colorWithAlpha(Color.White,.16))` | 1540459 |

### D.61 Factory 70982 — `m`

Source: `app.js`, object byte **1540510**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `o`→factory 29942; `s`→factory 44548; `d`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.attributionSize` | `S(40)` | 1540527 |
| `m.spacing` | `S(12)` | 1540548 |
| `m.format.typography` | `Typography.Standard.Caption1` | 1540588 |
| `m.format.color` | `Color.LabelPrimary` | 1540616 |
| `m.format.textAlignment` | `0` | 1540648 |
| `m.format.pageAlignment` | `.5` | 1540664 |
| `m.format.paragraphSpacingAfter` | `0` | 1540689 |

### D.62 Factory 70982 — `f`

Source: `app.js`, object byte **1540696**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `f.(spread)` | `m` | 1540700 |
| `f.symbolSize` | `S(18)` | 1540713 |
| `f.metadataSymbolSpacing` | `S(12)` | 1540748 |

### D.63 Factory 99454 — `d`

Source: `app.js`, object byte **1540890**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `d.BorderWidth` | `S(2,!0)` | 1540903 |
| `d.RotationPeriod` | `8e3` | 1540933 |
| `d.GlowOverscale` | `1.22` | 1540951 |
| `d.GlowVerticalScale` | `.9` | 1540974 |
| `d.GradientOverlayOpacity` | `.3` | 1541000 |
| `d.BackgroundColor.r` | `29/255` | 1541022 |
| `d.BackgroundColor.g` | `29/255` | 1541031 |
| `d.BackgroundColor.b` | `29/255` | 1541040 |
| `d.BackgroundColor.a` | `1` | 1541049 |
| `d.Bars.primary.widthFraction` | `.8` | 1541081 |
| `d.Bars.primary.opacity` | `.25` | 1541092 |
| `d.Bars.primary.startAngle` | `Math.PI/2` | 1541107 |
| `d.Bars.primary.gradientStops` | `[{position:0,alpha:0},{position:.1,alpha:.1},{position:.3,alpha:.7},{position:.5,alpha:1},{position:.7,alpha:.7},{position:.9,alpha:.1},{position:1,alpha:0}]` | 1541131 |
| `d.Bars.secondary.widthFraction` | `1` | 1541315 |
| `d.Bars.secondary.opacity` | `.35` | 1541325 |
| `d.Bars.secondary.startAngle` | `-Math.PI/4` | 1541340 |
| `d.Bars.secondary.gradientStops` | `[{position:0,alpha:0},{position:.1,alpha:.3},{position:.2,alpha:.8},{position:.5,alpha:1},{position:.8,alpha:.8},{position:.9,alpha:.3},{position:1,alpha:0}]` | 1541365 |
| `d.Bars.tertiary.widthFraction` | `.7` | 1541548 |
| `d.Bars.tertiary.opacity` | `.15` | 1541559 |
| `d.Bars.tertiary.startAngle` | `Math.PI` | 1541574 |
| `d.Bars.tertiary.gradientStops` | `[{position:0,alpha:0},{position:.25,alpha:.5},{position:.5,alpha:1},{position:.75,alpha:.5},{position:1,alpha:0}]` | 1541596 |
| `d.GradientDirections` | `[[0,0,1,1],[1,0,0,1],[1,.25,0,.75],[1,.75,0,.25],[1,1,0,1],[0,1,1,0],[0,.75,1,.25],[0,.25,1,.75]]` | 1541731 |

### D.64 Factory 85790 — `P`

Source: `app.js`, object byte **1545628**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `C`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `P.anchorHeight` | `C.nl.height-S(114)` | 1545642 |
| `P.buttonRowHeight` | `S(66)` | 1545684 |
| `P.spacings.metadata` | `S(18)` | 1545716 |
| `P.spacings.buttonRow` | `S(32)` | 1545739 |
| `P.spacings.buttons` | `S(24)` | 1545760 |
| `P.spacings.features` | `S(10)` | 1545782 |
| `P.metadataIconHeight` | `S(22)` | 1545815 |

### D.65 Factory 18637 — `p`

Source: `app.js`, object byte **1554915**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 44548; `o`→factory 16393; `s`→factory 3038; `l`→factory 50429; `d`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.spacing` | `S(8)` | 1554924 |
| `p.playGlyphHeight` | `S(15)` | 1554952 |
| `p.playAgainGlyphHeight` | `S(18)` | 1554986 |
| `p.playAgainGlyphPadding.bottom` | `S(2)` | 1555029 |
| `p.progress.width` | `S(34)` | 1555058 |
| `p.progress.height` | `S(6)` | 1555078 |
| `p.progress.background` | `()=>l.yP.from(Color.LabelQuaternary_RespectTransparencySetting())` | 1555101 |
| `p.progress.foreground` | `()=>l.yP.from(Color.LabelTertiary_RespectTransparencySetting())` | 1555177 |
| `p.progress.backgroundHighlighted` | `()=>l.yP.from(Color.LabelTertiary_RespectTransparencySetting())` | 1555262 |
| `p.progress.foregroundHighlighted` | `()=>l.yP.from(Color.LabelSecondary_RespectTransparencySetting())` | 1555347 |
| `p.format.typography` | `Typography.Standard.Caption3` | 1555439 |
| `p.format.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1555467 |
| `p.formatHighlighted.typography` | `Typography.Standard.Caption3` | 1555554 |
| `p.formatHighlighted.color` | `Color.LabelPrimary` | 1555582 |

### D.66 Factory 18637 — `m`

Source: `app.js`, object byte **1555605**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 44548; `o`→factory 16393; `s`→factory 3038; `d`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.width` | `S(50)` | 1555612 |
| `m.height` | `S(9)` | 1555632 |
| `m.format.typography` | `Typography.Regular.Body` | 1555671 |
| `m.backgroundColor` | `e=>e?Color.LabelTertiaryReversed_RespectTransparencySetting():Color.LabelTertiary_RespectTransparencySetting()` | 1555706 |
| `m.foregroundColor` | `e=>e?Color.LabelPrimaryReversed():Color.LabelPrimary()` | 1555831 |

### D.67 Factory 17686 — `S`

Source: `app.js`, object byte **1556122**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 84556; `m`→factory 3038; `f`→factory 44548; `y`→factory 55242; `v`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `S.height` | `S(57)` | 1556130 |
| `S.heightFinalScoresOff` | `S(44)` | 1556164 |
| `S.fonts.team.typography.(spread)` | `Typography.Standard.Caption3` | 1556213 |
| `S.fonts.team.typography.size` | `18` | 1556240 |
| `S.fonts.team.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,r().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 1556260 |
| `S.fonts.team.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1556379 |
| `S.fonts.team.vibrancyEffectStyle` | `"secondary"` | 1556446 |
| `S.fonts.info.typography.(spread)` | `Typography.Standard.Caption3` | 1556489 |
| `S.fonts.info.typography.size` | `16` | 1556516 |
| `S.fonts.info.typography.leading` | `17` | 1556527 |
| `S.fonts.info.color` | `Color.LabelPrimary` | 1556537 |
| `S.fonts.scores.typography.fontName` | `"SFCompressed-Semibold"` | 1556594 |
| `S.fonts.scores.typography.weight` | `y.I.Semibold` | 1556625 |
| `S.fonts.scores.typography.size` | `34` | 1556643 |
| `S.fonts.scores.typography.leading` | `38` | 1556654 |
| `S.fonts.scores.color` | `Color.LabelPrimary` | 1556664 |
| `S.fonts.scoresLoser.typography.fontName` | `"SFCompressed-Semibold"` | 1556726 |
| `S.fonts.scoresLoser.typography.weight` | `y.I.Semibold` | 1556757 |
| `S.fonts.scoresLoser.typography.size` | `34` | 1556775 |
| `S.fonts.scoresLoser.typography.leading` | `38` | 1556786 |
| `S.fonts.scoresLoser.color` | `Color.LabelSecondary` | 1556796 |
| `S.fonts.scoresLoser.vibrancyEffectStyle` | `"secondary"` | 1556836 |
| `S.fonts.symbolSize` | `S(12)` | 1556861 |
| `S.spacings.scoresToTeams` | `S(6)` | 1556899 |
| `S.spacings.favoriteIcon` | `S(4)` | 1556924 |
| `S.spacings.infoToTeamLogo` | `S(14)` | 1556951 |
| `S.spacings.infoToScoresLive` | `S(4)` | 1556981 |
| `S.spacings.infoToScoresFinal` | `S(17)` | 1557011 |
| `S.info.clockWidth.empty` | `S(88)` | 1557049 |
| `S.info.clockWidth.regular` | `S(64)` | 1557070 |
| `S.info.clockWidth.wide` | `S(156)` | 1557088 |
| `S.info.scoreWidth` | `S(50)` | 1557114 |
| `S.info.symbolSpacing` | `S(8)` | 1557141 |
| `S.team.logoSize` | `S(38)` | 1557169 |
| `S.team.favoriteIconSize` | `S(10)` | 1557199 |

### D.68 Factory 17686 — `w`

Source: `app.js`, object byte **1557216**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 84556; `m`→factory 3038; `f`→factory 44548; `y`→factory 55242; `v`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `w.height` | `S(72)` | 1557224 |
| `w.heightFinalScoresOff` | `S(54)` | 1557258 |
| `w.fonts.team.typography.(spread)` | `Typography.Standard.Caption3` | 1557307 |
| `w.fonts.team.typography.size` | `20` | 1557334 |
| `w.fonts.team.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,r().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 1557354 |
| `w.fonts.team.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1557473 |
| `w.fonts.team.vibrancyEffectStyle` | `"secondary"` | 1557540 |
| `w.fonts.info.typography.(spread)` | `Typography.Standard.Caption3` | 1557583 |
| `w.fonts.info.typography.size` | `20` | 1557610 |
| `w.fonts.info.typography.leading` | `22` | 1557621 |
| `w.fonts.info.color` | `Color.LabelPrimary` | 1557631 |
| `w.fonts.scores.typography.fontName` | `"SFCompressed-Semibold"` | 1557688 |
| `w.fonts.scores.typography.weight` | `y.I.Semibold` | 1557719 |
| `w.fonts.scores.typography.size` | `44` | 1557737 |
| `w.fonts.scores.typography.leading` | `48` | 1557748 |
| `w.fonts.scores.color` | `Color.LabelPrimary` | 1557758 |
| `w.fonts.scoresLoser.typography.fontName` | `"SFCompressed-Semibold"` | 1557820 |
| `w.fonts.scoresLoser.typography.weight` | `y.I.Semibold` | 1557851 |
| `w.fonts.scoresLoser.typography.size` | `44` | 1557869 |
| `w.fonts.scoresLoser.typography.leading` | `48` | 1557880 |
| `w.fonts.scoresLoser.color` | `Color.LabelSecondary` | 1557890 |
| `w.fonts.scoresLoser.vibrancyEffectStyle` | `"secondary"` | 1557930 |
| `w.fonts.symbolSize` | `S(15)` | 1557955 |
| `w.spacings.scoresToTeams` | `S(7)` | 1557993 |
| `w.spacings.favoriteIcon` | `S(5)` | 1558018 |
| `w.spacings.infoToTeamLogo` | `S(18)` | 1558045 |
| `w.spacings.infoToScoresLive` | `S(5)` | 1558075 |
| `w.spacings.infoToScoresFinal` | `S(28)` | 1558105 |
| `w.info.clockWidth.empty` | `S(102)` | 1558143 |
| `w.info.clockWidth.regular` | `S(78)` | 1558165 |
| `w.info.clockWidth.wide` | `S(194)` | 1558183 |
| `w.info.scoreWidth` | `S(64)` | 1558209 |
| `w.info.symbolSpacing` | `S(10)` | 1558236 |
| `w.team.logoSize` | `S(50)` | 1558265 |
| `w.team.favoriteIconSize` | `S(12)` | 1558295 |

### D.69 Factory 17686 — `C`

Source: `app.js`, object byte **1558312**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `f`→factory 44548; `y`→factory 55242; `v`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.maxWidth` | `S(400)` | 1558322 |
| `C.fonts.team.typography.fontName` | `"SFCompressed-Semibold"` | 1558378 |
| `C.fonts.team.typography.weight` | `y.I.Semibold` | 1558409 |
| `C.fonts.team.typography.size` | `56` | 1558427 |
| `C.fonts.team.typography.leading` | `56` | 1558438 |
| `C.fonts.team.color` | `Color.LabelPrimary` | 1558448 |
| `C.fonts.scores.typography.fontName` | `"SFCompressed-Semibold"` | 1558505 |
| `C.fonts.scores.typography.weight` | `y.I.Semibold` | 1558536 |
| `C.fonts.scores.typography.size` | `56` | 1558554 |
| `C.fonts.scores.typography.leading` | `56` | 1558565 |
| `C.fonts.scores.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.TabularFigures]` | 1558585 |
| `C.fonts.scores.color` | `Color.LabelPrimary` | 1558644 |
| `C.fonts.scoresLoser.typography.fontName` | `"SFCompressed-Semibold"` | 1558706 |
| `C.fonts.scoresLoser.typography.weight` | `y.I.Semibold` | 1558737 |
| `C.fonts.scoresLoser.typography.size` | `56` | 1558755 |
| `C.fonts.scoresLoser.typography.leading` | `56` | 1558766 |
| `C.fonts.scoresLoser.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.TabularFigures]` | 1558786 |
| `C.fonts.scoresLoser.color` | `Color.LabelSecondary` | 1558845 |
| `C.fonts.scoresLoser.vibrancyEffectStyle` | `"secondary"` | 1558885 |
| `C.spacings.logoToTeamName` | `S(8)` | 1558925 |
| `C.spacings.teamNameToScore` | `S(24)` | 1558953 |
| `C.spacings.teamRows` | `S(2)` | 1558975 |
| `C.team.logoSize` | `S(56)` | 1559003 |
| `C.team.favoriteIconSize` | `S(20)` | 1559033 |

### D.70 Factory 17686 — `b`

Source: `app.js`, object byte **1559050**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `f`→factory 44548; `y`→factory 55242; `v`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `b.maxWidth` | `S(700)` | 1559060 |
| `b.fonts.team.typography.fontName` | `"SFCompressed-Semibold"` | 1559116 |
| `b.fonts.team.typography.weight` | `y.I.Semibold` | 1559147 |
| `b.fonts.team.typography.size` | `76` | 1559165 |
| `b.fonts.team.typography.leading` | `76` | 1559176 |
| `b.fonts.team.color` | `Color.LabelPrimary` | 1559186 |
| `b.fonts.scores.typography.fontName` | `"SFCompressed-Semibold"` | 1559243 |
| `b.fonts.scores.typography.weight` | `y.I.Semibold` | 1559274 |
| `b.fonts.scores.typography.size` | `76` | 1559292 |
| `b.fonts.scores.typography.leading` | `76` | 1559303 |
| `b.fonts.scores.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.TabularFigures]` | 1559323 |
| `b.fonts.scores.color` | `Color.LabelPrimary` | 1559382 |
| `b.fonts.scoresLoser.typography.fontName` | `"SFCompressed-Semibold"` | 1559444 |
| `b.fonts.scoresLoser.typography.weight` | `y.I.Semibold` | 1559475 |
| `b.fonts.scoresLoser.typography.size` | `76` | 1559493 |
| `b.fonts.scoresLoser.typography.leading` | `76` | 1559504 |
| `b.fonts.scoresLoser.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.TabularFigures]` | 1559524 |
| `b.fonts.scoresLoser.color` | `Color.LabelSecondary` | 1559583 |
| `b.fonts.scoresLoser.vibrancyEffectStyle` | `"secondary"` | 1559623 |
| `b.spacings.logoToTeamName` | `S(12)` | 1559663 |
| `b.spacings.teamNameToScore` | `S(40)` | 1559692 |
| `b.spacings.teamRows` | `S(6)` | 1559714 |
| `b.team.logoSize` | `S(76)` | 1559742 |
| `b.team.favoriteIconSize` | `S(20)` | 1559772 |

### D.71 Factory 5641 — `m`

Source: `app.js`, object byte **1572699**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 46491; `s`→factory 50429.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.speed` | `S(30)` | 1572706 |
| `m.insetFadeTime` | `300` | 1572733 |
| `m.wait` | `2700` | 1572742 |
| `m.spacing` | `S(150)` | 1572755 |
| `m.restingMode` | `s.bJ.RestingMode.Masked` | 1572781 |

### D.72 Factory 52297 — `h`

Source: `app.js`, object byte **1593885**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491; `o`→factory 301; `s`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `h.cornerRadius` | `S(20)` | 1593899 |
| `h.lockupSeparation.idle` | `S(4)` | 1593935 |
| `h.lockupSeparation.focused` | `S(10)` | 1593955 |
| `h.width` | `o.x.C.shelfItemWidth` | 1593975 |
| `h.height` | `o.x.calculateShelfItemHeight(o.x.C.shelfItemWidth,16/9)+u+S(8)` | 1594003 |
| `h.descriptionHeight` | `u` | 1594091 |
| `h.shadow.idle.dx` | `0` | 1594110 |
| `h.shadow.idle.dy` | `S(4)` | 1594115 |
| `h.shadow.idle.color` | `()=>Color.Black18` | 1594133 |
| `h.shadow.idle.blurRadius` | `S(12)` | 1594161 |
| `h.shadow.focused.dx` | `0` | 1594187 |
| `h.shadow.focused.dy` | `S(40)` | 1594192 |
| `h.shadow.focused.color` | `()=>Color.Black30` | 1594211 |
| `h.shadow.focused.blurRadius` | `S(45)` | 1594239 |

### D.73 Factory 52297 — `g`

Source: `app.js`, object byte **1594257**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `g.progressPadding.horizontal` | `S(18)` | 1594286 |
| `g.progressPadding.vertical` | `S(12)` | 1594308 |
| `g.gradientColor` | `new(a()).Gfx.Color(40/255,40/255,40/255,1)` | 1594336 |

### D.74 Factory 52297 — `p`

Source: `app.js`, object byte **1594382**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491; `o`→factory 301; `s`→factory 44548; `d`→factory 3038; `c`→factory 50429.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.selectScale` | `-S(40)` | 1594395 |
| `p.padding.horizontal` | `S(18)` | 1594429 |
| `p.padding.vertical` | `S(14)` | 1594451 |
| `p.background.idle` | `c.yP.white.alpha(0)` | 1594482 |
| `p.background.focused` | `c.yP.white.alpha(.2)` | 1594510 |
| `p.background.borderOpacity` | `.025` | 1594545 |
| `p.formats.eyebrow.typography` | `Typography.Standard.Caption3` | 1594588 |
| `p.formats.eyebrow.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1594616 |
| `p.formats.title.typography` | `Typography.Standard.Caption1` | 1594691 |
| `p.formats.title.color` | `Color.LabelPrimary` | 1594719 |
| `p.formats.description.typography` | `Typography.Standard.Caption2` | 1594771 |
| `p.formats.description.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1594799 |
| `p.formats.metadata.typography` | `Typography.Standard.Caption3` | 1594877 |
| `p.formats.metadata.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1594905 |
| `p.formats.eyebrowMaxLines` | `1` | 1594970 |
| `p.formats.titleMaxLines` | `1` | 1594986 |
| `p.formats.descriptionMaxLines` | `5` | 1595008 |
| `p.formats.metadataMaxLines` | `1` | 1595027 |
| `p.spacings.content` | `S(2)` | 1595048 |
| `p.spacings.contentToMetadata` | `S(8)` | 1595078 |
| `p.metadata.badgeHeight` | `S(22)` | 1595113 |
| `p.metadata.spacing` | `S(10)` | 1595134 |
| `p.metadata.text.padding.horizontal` | `S(7)` | 1595173 |
| `p.metadata.text.cornerRadius` | `S(4)` | 1595199 |
| `p.metadata.text.borderWidth` | `S(2,!0)` | 1595223 |
| `p.verticalOffset.idle` | `0` | 1595261 |
| `p.verticalOffset.focused` | `S(14)` | 1595271 |

### D.75 Factory 11922 — `h`

Source: `app.js`, object byte **1603570**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 50429; `l`→factory 46491; `c`→factory 3038; `u`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `h.padding` | `S(20)` | 1603579 |
| `h.version.artwork.cornerRadius` | `S(12)` | 1603623 |
| `h.version.artwork.width` | `S(196)` | 1603642 |
| `h.version.artwork.height` | `S(110)` | 1603663 |
| `h.version.fonts.title.typography` | `Typography.Emphasized.Callout` | 1603711 |
| `h.version.fonts.title.color` | `Color.LabelPrimary` | 1603740 |
| `h.version.fonts.subtitle.typography` | `Typography.Standard.Caption2` | 1603789 |
| `h.version.fonts.subtitle.color` | `Color.LabelSecondary` | 1603817 |
| `h.version.fonts.subtitleFocusedColor` | `Color.LabelPrimary` | 1603860 |
| `h.version.fonts.price.typography` | `Typography.Standard.Callout` | 1603904 |
| `h.version.fonts.price.color` | `Color.LabelPrimary` | 1603931 |
| `h.version.background.idle` | `o.yP.white.alpha(.1)` | 1603969 |
| `h.version.background.focused` | `o.yP.white.alpha(.5)` | 1603998 |
| `h.version.contentToPriceSpacing` | `S(40)` | 1604042 |
| `h.listItem.artwork.cornerRadius` | `S(13)` | 1604088 |
| `h.listItem.artwork.width` | `S(160)` | 1604107 |
| `h.listItem.artwork.height` | `S(90)` | 1604128 |
| `h.listItem.fonts.title.typography` | `Typography.Standard.Caption1` | 1604175 |
| `h.listItem.fonts.title.color` | `Color.LabelPrimary` | 1604203 |
| `h.listItem.fonts.subtitle.typography` | `Typography.Standard.Caption2` | 1604252 |
| `h.listItem.fonts.subtitle.color` | `Color.LabelSecondary` | 1604280 |
| `h.listItem.fonts.subtitleFocusedColor` | `Color.LabelPrimary` | 1604323 |
| `h.listItem.fonts.price.typography` | `Typography.Standard.Caption1` | 1604367 |
| `h.listItem.fonts.price.color` | `Color.LabelPrimary` | 1604395 |
| `h.listItem.background.idle` | `o.yP.white.alpha(.1)` | 1604433 |
| `h.listItem.background.focused` | `o.yP.white.alpha(.2)` | 1604462 |
| `h.listItem.contentToPriceSpacing` | `S(40)` | 1604506 |
| `h.boxset.artwork.cornerRadius` | `S(12)` | 1604550 |
| `h.boxset.artwork.width` | `S(196)` | 1604569 |
| `h.boxset.artwork.height` | `S(110)` | 1604590 |
| `h.boxset.fonts.title.typography` | `Typography.Standard.Body` | 1604638 |
| `h.boxset.fonts.title.color` | `Color.LabelPrimary` | 1604662 |
| `h.boxset.fonts.subtitle.typography` | `Typography.Standard.Caption2` | 1604711 |
| `h.boxset.fonts.subtitle.color` | `Color.LabelSecondary` | 1604739 |
| `h.boxset.fonts.subtitleFocusedColor` | `Color.LabelPrimary` | 1604782 |
| `h.boxset.fonts.ratingIdleColor` | `Color.LabelTertiary` | 1604816 |
| `h.boxset.fonts.ratingFocusedColor` | `Color.LabelPrimary` | 1604854 |
| `h.boxset.fonts.ratingHeight` | `S(22)` | 1604885 |
| `h.boxset.background.idle` | `o.yP.white.alpha(.1)` | 1604916 |
| `h.boxset.background.focused` | `o.yP.white.alpha(.2)` | 1604945 |
| `h.boxset.titleToSubtitleSpacing` | `S(5)` | 1604990 |
| `h.boxset.ratingSpacing` | `S(10)` | 1605016 |

### D.76 Factory 11922 — `b`

Source: `app.js`, object byte **1607481**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 50429; `l`→factory 46491; `c`→factory 3038; `u`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `b.horizontalPadding.leading` | `S(20)` | 1607509 |
| `b.horizontalPadding.trailing` | `S(40)` | 1607531 |
| `b.verticalPadding` | `e=>void 0!==e&&e<=2?S(48):S(20)` | 1607561 |
| `b.platter.idleColor` | `e=>o.yP.white.alpha(.1*!!e)` | 1607626 |
| `b.platter.focusedColor` | `o.yP.white.alpha(.5)` | 1607667 |
| `b.artwork.width` | `S(242)` | 1607704 |
| `b.artwork.height` | `S(136)` | 1607725 |
| `b.artwork.cornerRadius` | `S(12)` | 1607752 |
| `b.fonts.title.typography` | `Typography.Emphasized.Callout` | 1607799 |
| `b.fonts.title.color` | `Color.LabelPrimary` | 1607828 |
| `b.fonts.price.typography` | `Typography.Standard.Callout` | 1607874 |
| `b.fonts.price.color` | `Color.LabelPrimary` | 1607901 |
| `b.fonts.strikeThrough.typography` | `Typography.Standard.Callout` | 1607955 |
| `b.fonts.strikeThrough.color` | `Color.LabelSecondary` | 1607982 |
| `b.fonts.disclaimer.typography` | `Typography.Standard.Caption2` | 1608035 |
| `b.fonts.disclaimer.color` | `Color.LabelSecondary` | 1608063 |
| `b.fonts.disclaimerFocusedColor` | `Color.LabelPrimary` | 1608108 |
| `b.fonts.action.typography` | `Typography.Emphasized.Callout` | 1608153 |
| `b.fonts.action.color` | `Color.LabelPrimary` | 1608182 |
| `b.spacings.artworkToContent` | `S(20)` | 1608230 |
| `b.spacings.contentToAction` | `S(40)` | 1608259 |
| `b.spacings.contentToDisclaimer` | `e=>e?S(20):S(7)` | 1608292 |
| `b.spacings.titleToPrice` | `S(2)` | 1608335 |
| `b.spacings.strikeThroughPrice` | `S(18)` | 1608366 |
| `b.divider.lineColor` | `Color.LabelPrimary()` | 1608399 |
| `b.divider.lineOpacity` | `.1` | 1608431 |
| `b.divider.width` | `S(1280)` | 1608440 |
| `b.divider.thickness` | `S(2)` | 1608465 |

### D.77 Factory 58125 — `d`

Source: `app.js`, object byte **1617787**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491; `o`→factory 16393; `s`→factory 3038; `l`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `d.formats.titleIdle.typography` | `Typography.Standard.Callout` | 1617827 |
| `d.formats.titleIdle.color` | `Color.LabelPrimary` | 1617854 |
| `d.formats.titleFocused.typography` | `Typography.Standard.Callout` | 1617907 |
| `d.formats.titleFocused.color` | `Color.LabelPrimary` | 1617934 |
| `d.formats.titleFocused.shadow.dx` | `0` | 1617963 |
| `d.formats.titleFocused.shadow.dy` | `S(4)` | 1617968 |
| `d.formats.titleFocused.shadow.color` | `Color.themedColor(Color.Black20,Color.Black40)` | 1617986 |
| `d.formats.titleFocused.shadow.blurRadius` | `S(4)` | 1618041 |
| `d.formats.subtitleIdle.typography` | `Typography.Standard.Caption1` | 1618089 |
| `d.formats.subtitleIdle.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1618117 |
| `d.formats.subtitleFocused.typography` | `Typography.Standard.Caption1` | 1618202 |
| `d.formats.subtitleFocused.color` | `Color.LabelPrimary` | 1618230 |
| `d.formats.subtitleFocused.shadow.dx` | `0` | 1618259 |
| `d.formats.subtitleFocused.shadow.dy` | `S(4)` | 1618264 |
| `d.formats.subtitleFocused.shadow.color` | `Color.themedColor(Color.Black20,Color.Black40)` | 1618282 |
| `d.formats.subtitleFocused.shadow.blurRadius` | `S(4)` | 1618337 |
| `d.formats.textSpacing` | `S(2)` | 1618364 |
| `d.initials.backgroundColor` | `new(a()).Gfx.Color(.5,.5,.5,1)` | 1618403 |
| `d.initials.format.typography.(spread)` | `Typography.Rounded.Caption1` | 1618465 |
| `d.initials.format.typography.size` | `116` | 1618491 |
| `d.initials.format.color` | `Color.LabelPrimary` | 1618502 |
| `d.initials.format.textAlignment` | `.5` | 1618534 |
| `d.initials.format.pageAlignment` | `.5` | 1618551 |
| `d.initials.minSize` | `o.l.minimumFontSize` | 1618564 |
| `d.distanceFromLockupBottomToTextBottom` | `S(46)` | 1618622 |

### D.78 Factory 87981 — `p`

Source: `app.js`, object byte **1629343**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 44548; `g`→factory 39554.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.active.icon` | `g.A` | 1629357 |
| `p.active.color` | `Color.SystemGreen()` | 1629367 |
| `p.active.iconColor` | `Color.White` | 1629396 |
| `p.active.iconMaterial` | `void 0` | 1629420 |
| `p.suggested.icon.path` | `Computed value; 331 source bytes; retained at source location` | 1629450 |
| `p.suggested.icon.size.width` | `83.765` | 1629794 |
| `p.suggested.icon.size.height` | `83.794` | 1629808 |
| `p.suggested.color` | `Color.colorFromRGB(235,235,235,1)` | 1629823 |
| `p.suggested.iconColor` | `Color.colorFromRGB(104,104,104,1)` | 1629866 |
| `p.suggested.iconMaterial` | `r().Gfx.DrawingContext.MaterialType.UltraThin` | 1629912 |

### D.79 Factory 36813 — `s`

Source: `app.js`, object byte **1631044**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `a`→factory 44548; `o`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `s.size` | `S(160)` | 1631050 |
| `s.zoom` | `S(20)` | 1631069 |
| `s.backgroundColor` | `Color.White20` | 1631098 |
| `s.title.idle.typography` | `Typography.Standard.Caption2` | 1631143 |
| `s.title.idle.color` | `Color.LabelSecondary` | 1631171 |
| `s.title.focused.typography` | `Typography.Standard.Caption2` | 1631221 |
| `s.title.focused.color` | `Color.White` | 1631249 |
| `s.title.focused.shadow.dx` | `0` | 1631271 |
| `s.title.focused.shadow.dy` | `S(4)` | 1631276 |
| `s.title.focused.shadow.color` | `Color.Black40` | 1631294 |
| `s.title.focused.shadow.blurRadius` | `S(4)` | 1631318 |
| `s.badge.size` | `S(36)` | 1631346 |
| `s.badge.iconSize` | `S(16)` | 1631368 |
| `s.badge.inset` | `0` | 1631387 |
| `s.placeholder.color` | `Color.White` | 1631409 |

### D.80 Factory 36813 — `l`

Source: `app.js`, object byte **1631424**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `a`→factory 44548; `o`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `l.size` | `S(250)` | 1631430 |
| `l.zoom` | `S(70)` | 1631449 |
| `l.backgroundColor` | `Color.White10` | 1631478 |
| `l.title.idle.typography` | `Typography.Standard.Callout` | 1631523 |
| `l.title.idle.color` | `Color.LabelTertiary` | 1631550 |
| `l.title.focused.typography` | `Typography.Standard.Callout` | 1631599 |
| `l.title.focused.color` | `Color.LabelPrimary` | 1631626 |
| `l.title.focused.shadow.dx` | `0` | 1631655 |
| `l.title.focused.shadow.dy` | `S(4)` | 1631660 |
| `l.title.focused.shadow.color` | `Color.Black40` | 1631678 |
| `l.title.focused.shadow.blurRadius` | `S(4)` | 1631702 |
| `l.badge.size` | `S(57)` | 1631730 |
| `l.badge.iconSize` | `S(27)` | 1631752 |
| `l.badge.inset` | `S(2)` | 1631771 |
| `l.placeholder.color` | `Color.LabelSecondary()` | 1631803 |
| `l.placeholder.iconSize` | `S(78)` | 1631834 |

### D.81 Factory 15915 — `m`

Source: `app.js`, object byte **1632051**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `g`→factory 3038; `p`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.GridC.padding.leading` | `S(18)` | 1632076 |
| `m.GridC.padding.trailing` | `S(18)` | 1632098 |
| `m.GridC.padding.top` | `S(18)` | 1632115 |
| `m.GridC.padding.bottom` | `S(14)` | 1632135 |
| `m.GridC.titleFont.typography` | `Typography.Standard.Caption1` | 1632179 |
| `m.GridC.titleFont.color` | `Color.LabelPrimary` | 1632207 |
| `m.GridC.titleMaxLines` | `1` | 1632241 |
| `m.GridC.providerLogo.height` | `S(18)` | 1632264 |
| `m.GridC.providerLogo.padding` | `S(18)` | 1632285 |
| `m.GridC.gameArtwork.width` | `S(180)` | 1632318 |
| `m.GridC.gameArtwork.height` | `S(102)` | 1632339 |
| `m.GridC.gameArtwork.cornerRadius` | `S(8)` | 1632366 |
| `m.GridC.gameArtwork.spacing` | `S(8)` | 1632386 |
| `m.GridD.padding.leading` | `S(14)` | 1632424 |
| `m.GridD.padding.trailing` | `S(14)` | 1632446 |
| `m.GridD.padding.top` | `S(14)` | 1632463 |
| `m.GridD.padding.bottom` | `S(10)` | 1632483 |
| `m.GridD.titleFont.typography` | `Typography.Standard.Caption2` | 1632527 |
| `m.GridD.titleFont.color` | `Color.LabelPrimary` | 1632555 |
| `m.GridD.titleMaxLines` | `1` | 1632589 |
| `m.GridD.providerLogo.height` | `S(14)` | 1632612 |
| `m.GridD.providerLogo.padding` | `S(14)` | 1632633 |
| `m.GridD.gameArtwork.width` | `S(142)` | 1632666 |
| `m.GridD.gameArtwork.height` | `S(80)` | 1632687 |
| `m.GridD.gameArtwork.cornerRadius` | `S(8)` | 1632713 |
| `m.GridD.gameArtwork.spacing` | `S(8)` | 1632733 |

### D.82 Factory 44418 — `f`

Source: `app.js`, object byte **1638429**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `c`→factory 50429; `u`→factory 46491; `g`→factory 3038; `p`→factory 89830.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `f.GridC.paddingWithScoreboard.leading` | `S(18)` | 1638468 |
| `f.GridC.paddingWithScoreboard.trailing` | `S(18)` | 1638490 |
| `f.GridC.paddingWithScoreboard.top` | `S(18)` | 1638507 |
| `f.GridC.paddingWithScoreboard.bottom` | `S(10)` | 1638527 |
| `f.GridC.paddingWithTitle.leading` | `S(18)` | 1638567 |
| `f.GridC.paddingWithTitle.trailing` | `S(18)` | 1638589 |
| `f.GridC.paddingWithTitle.top` | `S(18)` | 1638606 |
| `f.GridC.paddingWithTitle.bottom` | `S(14)` | 1638626 |
| `f.GridC.titleFont.typography` | `Typography.Standard.Caption1` | 1638670 |
| `f.GridC.titleFont.color` | `Color.LabelPrimary` | 1638698 |
| `f.GridC.titleMaxLines` | `1` | 1638732 |
| `f.GridC.leagueFont.typography.(spread)` | `Typography.Standard.Caption3` | 1638769 |
| `f.GridC.leagueFont.typography.size` | `19` | 1638796 |
| `f.GridC.leagueFont.typography.leading` | `S(19)` | 1638807 |
| `f.GridC.leagueFont.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,r().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 1638837 |
| `f.GridC.leagueFont.color` | `Color.LabelPrimary` | 1638956 |
| `f.GridC.leagueHeight` | `S(23)` | 1638989 |
| `f.GridC.leagueBadge.color` | `Color.Black40` | 1639021 |
| `f.GridC.leagueBadge.cornerRadius` | `S(8)` | 1639047 |
| `f.GridC.leagueBadge.padding.horizontal` | `S(8)` | 1639079 |
| `f.GridC.leagueBadge.padding.top` | `S(1)` | 1639095 |
| `f.GridC.leagueBadge.padding.bottom` | `S(2)` | 1639114 |
| `f.GridC.leaguePadding.horizontal` | `S(40)` | 1639154 |
| `f.GridC.providerLogo.height` | `S(18)` | 1639189 |
| `f.GridC.providerLogo.padding` | `S(18)` | 1639210 |
| `f.GridD.paddingWithScoreboard.leading` | `S(14)` | 1639263 |
| `f.GridD.paddingWithScoreboard.trailing` | `S(14)` | 1639285 |
| `f.GridD.paddingWithScoreboard.top` | `S(14)` | 1639302 |
| `f.GridD.paddingWithScoreboard.bottom` | `S(8)` | 1639322 |
| `f.GridD.paddingWithTitle.leading` | `S(14)` | 1639361 |
| `f.GridD.paddingWithTitle.trailing` | `S(14)` | 1639383 |
| `f.GridD.paddingWithTitle.top` | `S(14)` | 1639400 |
| `f.GridD.paddingWithTitle.bottom` | `S(10)` | 1639420 |
| `f.GridD.titleFont.typography` | `Typography.Standard.Caption2` | 1639464 |
| `f.GridD.titleFont.color` | `Color.LabelPrimary` | 1639492 |
| `f.GridD.titleMaxLines` | `1` | 1639526 |
| `f.GridD.leagueFont.typography.(spread)` | `Typography.Standard.Caption3` | 1639563 |
| `f.GridD.leagueFont.typography.size` | `14` | 1639590 |
| `f.GridD.leagueFont.typography.leading` | `S(16)` | 1639601 |
| `f.GridD.leagueFont.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,r().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 1639631 |
| `f.GridD.leagueFont.color` | `Color.LabelPrimary` | 1639750 |
| `f.GridD.leagueHeight` | `S(20)` | 1639783 |
| `f.GridD.leagueBadge.color` | `Color.Black40` | 1639815 |
| `f.GridD.leagueBadge.cornerRadius` | `S(8)` | 1639841 |
| `f.GridD.leagueBadge.padding.horizontal` | `S(6)` | 1639873 |
| `f.GridD.leagueBadge.padding.top` | `S(1)` | 1639889 |
| `f.GridD.leagueBadge.padding.bottom` | `S(2)` | 1639908 |
| `f.GridD.leaguePadding.horizontal` | `S(30)` | 1639948 |
| `f.GridD.providerLogo.height` | `S(14)` | 1639983 |
| `f.GridD.providerLogo.padding` | `S(14)` | 1640004 |
| `f.General.eyebrowFont.typography.(spread)` | `Typography.Standard.Caption3` | 1640064 |
| `f.General.eyebrowFont.typography.openTypeFeatures` | `[r().Gfx.TextFormat.OpenTypeFeature.SmallCapitals,r().Gfx.TextFormat.OpenTypeFeature.SmallCapitalsFromCapitals]` | 1640103 |
| `f.General.eyebrowFont.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1640222 |
| `f.General.eyebrowFont.vibrancyEffectStyle` | `"secondary"` | 1640289 |
| `f.General.eyebrowMaxLines` | `1` | 1640319 |
| `f.General.highlightBorder.width` | `S(2)` | 1640344 |
| `f.General.highlightBorder.color` | `c.yP.white` | 1640362 |
| `f.General.autoPlay.backgroundColor` | `Color.Black50` | 1640400 |
| `f.General.autoPlay.backgroundFadeDuration` | `250` | 1640436 |
| `f.General.autoPlay.widgetStyle.(spread)` | `p.E.Style` | 1640456 |
| `f.General.autoPlay.widgetStyle.circleLineWidth` | `S(2)` | 1640482 |
| `f.General.autoPlay.widgetStyle.autoPlayIconPointSize` | `S(63)` | 1640516 |
| `f.General.autoPlay.widgetStyle.dialDiameter` | `S(63)` | 1640542 |
| `f.General.autoPlay.widgetStyle.iconPointSize` | `Typography.Standard.Body.size` | 1640569 |
| `f.General.autoPlay.widgetStyle.backgroundColor` | `new(r()).Gfx.Color(72/255,72/255,74/255,.9)` | 1640608 |
| `f.General.videoScalingFactor` | `1.4` | 1640673 |
| `f.General.protectionGradientColor` | `Color.Black` | 1640701 |

### D.83 Factory 44418 — `y`

Source: `app.js`, object byte **1640716**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `m`→factory 16501.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `y.(spread)` | `m.h` | 1640720 |
| `y.volume` | `0` | 1640731 |
| `y.limitPlaybackToSD` | `!0` | 1640751 |
| `y.startDelay` | `250` | 1640765 |
| `y.duration` | `3e4` | 1640778 |

### D.84 Factory 21676 — `f`

Source: `app.js`, object byte **1647550**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 26160; `s`→factory 44548; `c`→factory 3038; `g`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `f.padding.leading` | `S(20)` | 1647568 |
| `f.padding.trailing` | `S(20)` | 1647590 |
| `f.padding.top` | `S(16)` | 1647607 |
| `f.padding.bottom` | `S(20)` | 1647627 |
| `f.titleFormat.typography` | `Typography.Standard.Caption1` | 1647673 |
| `f.titleFormat.color` | `Color.LabelPrimary` | 1647701 |
| `f.titleFormat.textAlignment` | `.5` | 1647733 |
| `f.titleFormat.pageAlignment` | `1` | 1647750 |
| `f.titleFormat.paragraphSpacingAfter` | `0` | 1647774 |
| `f.logoSize` | `S(192)` | 1647787 |
| `f.logoSizeSmall` | `S(176)` | 1647815 |
| `f.logoSmallPadding` | `S(26)` | 1647846 |
| `f.favoriteIconSize` | `S(Typography.Standard.Body.size)` | 1647876 |
| `f.favoriteIconPadding` | `S(20)` | 1647929 |
| `f.favoriteColor` | `Color.LabelPrimary` | 1647956 |

### D.85 Factory 77926 — `A`

Source: `app.js`, object byte **1650717**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 25654; `g`→factory 44548; `v`→factory 46491; `S`→factory 301; `C`→factory 3038; `b`→factory 41457; `x`→factory 10751.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `A.width` | `S(1760)` | 1650724 |
| `A.height` | `S(390)` | 1650746 |
| `A.padding.leading` | `S(70)` | 1650777 |
| `A.cornerRadius` | `S(30)` | 1650804 |
| `A.contentLogo.width` | `S(134)` | 1650836 |
| `A.contentLogo.height` | `S(45)` | 1650857 |
| `A.contentLogo.padding.horizontal` | `S(60)` | 1650890 |
| `A.contentLogo.padding.vertical` | `S(40)` | 1650912 |
| `A.shadow.dx` | `0` | 1650938 |
| `A.shadow.dy` | `S(4)` | 1650943 |
| `A.shadow.color` | `Color.Black40` | 1650961 |
| `A.shadow.blurRadius` | `S.x.C.cornerRadius` | 1650985 |
| `A.header.width` | `S(550)` | 1651019 |
| `A.header.spacing` | `S(24)` | 1651041 |
| `A.header.channelLogoHeight` | `S(48)` | 1651072 |
| `A.fonts.header.typography` | `Typography.Emphasized.Headline` | 1651120 |
| `A.fonts.header.color` | `Color.LabelPrimary` | 1651150 |
| `A.fonts.header.textAlignment` | `0` | 1651182 |
| `A.fonts.header.pageAlignment` | `.5` | 1651198 |
| `A.fonts.headerMaxLines` | `2` | 1651218 |
| `A.fonts.headerMinSizeFactor` | `.7` | 1651240 |
| `A.fonts.legal.typography` | `Typography.Standard.Caption2` | 1651269 |
| `A.fonts.legal.color` | `Color.LabelSecondary` | 1651297 |
| `A.fonts.legal.textAlignment` | `.5` | 1651331 |
| `A.fonts.legal.pageAlignment` | `.5` | 1651348 |
| `A.fonts.legalMinFontSize` | `Typography.Standard.Caption2.size-2` | 1651370 |
| `A.fonts.legalMaxLines` | `2` | 1651413 |
| `A.buttons.horizontalSpacing` | `S(24)` | 1651443 |
| `A.buttons.verticalSpacing` | `S(12)` | 1651472 |
| `A.buttons.primaryMinWidth` | `S(350)` | 1651501 |
| `A.buttons.primaryMaxWidth` | `S(390)` | 1651531 |
| `A.buttons.primaryStyle` | `b.JM.Regular.Dark()` | 1651558 |
| `A.buttons.secondaryIconSize` | `S(32)` | 1651596 |
| `A.buttons.secondaryStyle` | `b.jM.Regular.Dark()` | 1651624 |

### D.86 Factory 50776 — `w`

Source: `app.js`, object byte **1670619**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 32691; `y`→factory 3038; `v`→factory 44548; `S`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `w.Badge.width` | `S(25)` | 1670633 |
| `w.Badge.height` | `S(25)` | 1670653 |
| `w.Badge.absoluteWidth` | `25` | 1670680 |
| `w.Flex.spacing` | `S(5)` | 1670698 |
| `w.Text.font.typography` | `Typography.Standard.Body` | 1670742 |
| `w.Text.font.color` | `Color.LabelSecondary_RespectTransparencySetting` | 1670766 |
| `w.Text.font.paragraphSpacingAfter` | `0` | 1670835 |

### D.87 Factory 21110 — `eE`

Source: `app.js`, object byte **1719820**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `W`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `eE.channelLogoWidth` | `S(256)` | 1719838 |
| `eE.channelLogoHeight` | `S(64)` | 1719870 |
| `eE.pageControlOffset` | `W.nl.height-S(886)` | 1719901 |
| `eE.videoAlignment` | `0` | 1719942 |
| `eE.autoAdvanceDelay` | `8e3` | 1719961 |
| `eE.protectionGradient.height` | `S(616)` | 1719992 |
| `eE.protectionGradient.solidHeight` | `S(60)` | 1720018 |
| `eE.protectionGradient.midPointOpacity` | `.5` | 1720047 |
| `eE.overlayImage.width` | `S(605)` | 1720071 |
| `eE.overlayImage.height` | `S(440)` | 1720092 |
| `eE.overlayImage.padding.leading` | `0` | 1720123 |
| `eE.overlayImage.padding.trailing` | `S(5)` | 1720134 |
| `eE.overlayImage.padding.top` | `S(102)` | 1720150 |
| `eE.overlayImage.padding.bottom` | `0` | 1720171 |
| `eE.overlayImage.fadeDuration` | `240` | 1720187 |

### D.88 Factory 34597 — `Z`

Source: `app.js`, object byte **1737920**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `y`→factory 44548; `S`→factory 3038; `K`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `Z.width` | `S(358)` | 1737927 |
| `Z.height` | `S(90)` | 1737948 |
| `Z.padding` | `S(29)` | 1737969 |
| `Z.iconUsernameSpacing` | `S(12)` | 1738002 |
| `Z.clockFormat.typography.(spread)` | `Typography.Tabular.Standard.Caption2` | 1738051 |
| `Z.clockFormat.typography.openTypeFeatures` | `[F().Gfx.TextFormat.OpenTypeFeature.TabularFigures]` | 1738098 |
| `Z.clockFormat.color` | `Color.LabelTertiary()` | 1738157 |
| `Z.clockFormat.vibrancyEffectStyle` | `"tertiary"` | 1738198 |
| `Z.switchUser.format.typography` | `Typography.Standard.Caption3` | 1738250 |
| `Z.switchUser.format.color` | `Color.LabelTertiary` | 1738278 |
| `Z.switchUser.focusVerticalOffset` | `S(18)` | 1738319 |
| `Z.switchUser.timing.hide` | `200` | 1738345 |
| `Z.switchUser.timing.show` | `230` | 1738354 |
| `Z.switchUser.timing.opacity` | `100` | 1738366 |
| `Z.switchUser.timing.delay` | `100` | 1738376 |
| `Z.switchUser.timing.launchHint` | `3e3` | 1738391 |
| `Z.usernameClockSpacing` | `S(10)` | 1738418 |
| `Z.maximumEllipsisFade` | `S(100)` | 1738451 |
| `Z.profileIcon.width` | `S(50)` | 1738484 |
| `Z.profileIcon.height` | `S(50)` | 1738504 |
| `Z.profileIcon.zoom.horizontal` | `S(18)` | 1738534 |
| `Z.profileIcon.zoom.vertical` | `S(18)` | 1738556 |
| `Z.profileIcon.zoom.horizontalAnchor` | `.5` | 1738586 |
| `Z.profileIcon.zoom.verticalAnchor` | `.5` | 1738604 |
| `Z.profileIcon.shadow.dx` | `0` | 1738619 |
| `Z.profileIcon.shadow.dy` | `S(10)` | 1738624 |
| `Z.profileIcon.shadow.color` | `Color.Black30` | 1738643 |
| `Z.profileIcon.shadow.blurRadius` | `S(10)` | 1738667 |
| `Z.profileIcon.fallbackBackgroundColor` | `Color.White20` | 1738705 |
| `Z.profileIcon.borderWidth` | `S(5)` | 1738730 |
| `Z.username.format.typography` | `Typography.Standard.Caption1` | 1738780 |
| `Z.username.format.color` | `Color.LabelSecondary` | 1738808 |
| `Z.username.marquee.startDelay` | `2800` | 1738850 |
| `Z.username.marquee.fadeInTime` | `300` | 1738866 |
| `Z.username.marquee.textSpacing` | `S(75)` | 1738882 |
| `Z.username.marquee.rollSpeed` | `S(30)` | 1738905 |
| `Z.username.focusVerticalOffset` | `S(-13)` | 1738939 |

### D.89 Factory 34597 — `Y`

Source: `app.js`, object byte **1738957**. Values are static source declarations; offsets in the last column locate each original value.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `Y.value` | `""` | 1738964 |
| `Y.layout` | `new(F()).Gfx.TextLayout` | 1738974 |
| `Y.width` | `0` | 1739004 |
| `Y.height` | `0` | 1739013 |
| `Y.behavior` | `0` | 1739024 |

### D.90 Factory 34597 — `el`

Source: `app.js`, object byte **1774620**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 96638; `y`→factory 44548; `S`→factory 3038; `w`→factory 33239; `E`→factory 38594; `K`→factory 46491; `ei`→factory 19595.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `el.width` | `S(358)` | 1774627 |
| `el.height` | `S(1010)` | 1774648 |
| `el.rounding` | `S(52)` | 1774672 |
| `el.backgroundColor` | `()=>d.O.accessibility.focusStyle?Color.SystemGray5_HighContrast():Color.SystemGray5()` | 1774701 |
| `el.margin.top` | `S(35)` | 1774797 |
| `el.margin.bottom` | `S(35)` | 1774817 |
| `el.margin.leading` | `S(35)` | 1774838 |
| `el.margin.trailing` | `S(35)` | 1774860 |
| `el.shadow.color` | `Color.Black` | 1774888 |
| `el.shadow.opacity` | `()=>d.O.accessibility.focusStyle?.2:.4` | 1774907 |
| `el.shadow.offset` | `new E.Math.Point2(0,S(4))` | 1774953 |
| `el.shadow.blurRadius` | `S(48)` | 1774997 |
| `el.border.color` | `()=>d.O.accessibility.focusStyle?Color.borderColorHighContrast():Color.borderColor()` | 1775025 |
| `el.minWidth` | `S(250)` | 1775118 |
| `el.dynamicSidebarBottomBuffer` | `S(19)` | 1775159 |
| `el.timing.delayCollapseForInactivity` | `6e4` | 1775207 |
| `el.timing.delayCollapseForTeachableMoment` | `8e3` | 1775243 |
| `el.timing.durationForCoveringMenu` | `400` | 1775271 |
| `el.timing.durationForUncoveringMenu` | `100` | 1775301 |
| `el.gradient.height` | `S(33)` | 1775323 |
| `el.gradient.bottomFadeAdjustment` | `S(15)` | 1775357 |
| `el.contentClipping.bottomOffset` | `S(133)` | 1775401 |
| `el.pill.icon.width` | `S(50)` | 1775434 |
| `el.pill.icon.height` | `S(50)` | 1775454 |
| `el.pill.personIcon` | `ei.A` | 1775479 |
| `el.pill.teachableMoment.height` | `S(80)` | 1775508 |
| `el.pill.teachableMoment.marginLeading` | `S(49)` | 1775535 |
| `el.pill.height` | `S(60)` | 1775556 |
| `el.pill.minimumScale` | `.8` | 1775582 |
| `el.pill.margin.top` | `S(59)` | 1775597 |
| `el.pill.margin.bottom` | `S(0)` | 1775617 |
| `el.pill.margin.leading` | `S(59)` | 1775637 |
| `el.pill.margin.trailing` | `S(0)` | 1775659 |
| `el.pill.spacing` | `S(12)` | 1775680 |
| `el.pill.padding.top` | `S(14)` | 1775706 |
| `el.pill.padding.bottom` | `S(14)` | 1775726 |
| `el.pill.padding.leading` | `S(14)` | 1775747 |
| `el.pill.padding.trailing` | `S(34)` | 1775769 |
| `el.title.width` | `S(500)` | 1775797 |
| `el.title.height` | `S(46)` | 1775818 |
| `el.title.position.x` | `S(80)` | 1775843 |
| `el.title.position.y` | `S(14)` | 1775858 |
| `el.title.format.typography` | `Typography.Emphasized.Body` | 1775899 |
| `el.title.format.color` | `Color.LabelPrimary` | 1775925 |
| `el.brandDivider.format` | `Computed rule; 216 source bytes; retained at source location` | 1775967 |
| `el.header.height` | `S(130)` | 1776200 |
| `el.header.spacing` | `S(69)` | 1776222 |
| `el.header.margin.top` | `S(20)` | 1776247 |
| `el.header.margin.bottom` | `S(20)` | 1776267 |
| `el.header.margin.leading` | `S(19)` | 1776288 |
| `el.header.margin.trailing` | `S(19)` | 1776310 |
| `el.header.avatarScale.min` | `1` | 1776341 |
| `el.header.avatarScale.max` | `1.5` | 1776347 |
| `el.items.width` | `S(320)` | 1776366 |
| `el.items.height` | `S(897)` | 1776387 |
| `el.items.offset` | `S(40)` | 1776408 |
| `el.items.spacing` | `S(10)` | 1776429 |
| `el.items.padding.top` | `S(54)` | 1776455 |
| `el.items.padding.bottom` | `S(0)` | 1776475 |
| `el.items.padding.leading` | `S(19)` | 1776495 |
| `el.items.padding.trailing` | `S(19)` | 1776517 |
| `el.items.scrollSpeed` | `S(144)` | 1776543 |
| `el.items.heightBuffer` | `S(80)` | 1776570 |
| `el.divider.width` | `S(300)` | 1776599 |
| `el.divider.height` | `S(1)` | 1776620 |
| `el.chevron.position.x` | `S(46)` | 1776654 |
| `el.chevron.position.y` | `S(76)` | 1776669 |
| `el.chevron.hiddenPosition` | `S(94)` | 1776698 |
| `el.chevron.teachableMomentPosition` | `S(36)` | 1776735 |
| `el.chevron.pointSize` | `21` | 1776758 |
| `el.chevron.color` | `Color.LabelPrimary()` | 1776767 |
| `el.pageIcon.width` | `S(33)` | 1776804 |
| `el.pageIcon.height` | `S(33)` | 1776824 |
| `el.channelIcon.width` | `S(50)` | 1776857 |
| `el.channelIcon.height` | `S(50)` | 1776877 |
| `el.channelIcon.iconPointSize` | `21` | 1776904 |
| `el.channelIcon.background.width` | `S(44)` | 1776925 |
| `el.channelIcon.background.height` | `S(44)` | 1776945 |
| `el.sectionPadding.leading` | `S(20)` | 1776984 |
| `el.sectionPadding.trailing` | `S(20)` | 1777006 |
| `el.sectionPadding.top` | `S(14)` | 1777023 |
| `el.sectionPadding.bottom` | `S(11)` | 1777043 |
| `el.itemButtons.style` | `w.Z.Default()` | 1777076 |
| `el.itemButtons.idle.color` | `Color.White80` | 1777102 |
| `el.itemButtons.idle.fill` | `Color.White10` | 1777120 |
| `el.itemButtons.focused.color` | `Color.LabelPrimaryReversed()` | 1777149 |
| `el.itemButtons.focused.fill` | `Color.SystemGray4Reversed()` | 1777182 |
| `el.itemButtons.focusCandidate.color` | `Color.LabelPrimary()` | 1777232 |
| `el.itemButtons.focusCandidate.fill` | `Color.White10` | 1777257 |
| `el.itemButtons.highlighted.color` | `Color.LabelPrimaryReversed()` | 1777290 |
| `el.itemButtons.highlighted.fill` | `Color.SystemGray4Reversed()` | 1777323 |

### D.91 Factory 49286 — `u`

Source: `app.js`, object byte **1805984**. Values are static source declarations; offsets in the last column locate each original value.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `u.getAnimParams` | `Computed rule; 187 source bytes; retained at source location` | 1805999 |
| `u.flavorFadeRange` | `.1` | 1806203 |

### D.92 Factory 49286 — `h`

Source: `app.js`, object byte **1806209**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `c`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `h.platterSize` | `S(80)` | 1806222 |
| `h.symbolSize` | `S(29)` | 1806246 |
| `h.padding.backward.trailing` | `S(3)` | 1806287 |
| `h.padding.backward.top` | `S(1)` | 1806303 |
| `h.padding.forward.leading` | `S(2)` | 1806333 |
| `h.padding.forward.top` | `S(1)` | 1806349 |
| `h.color` | `Color.LabelPrimary` | 1806369 |
| `h.backgroundColor` | `Color.Black50` | 1806403 |
| `h.backgroundColorHighlighted` | `Color.colorWithAlpha(Color.Gray49,.4)` | 1806443 |
| `h.fadeDuration` | `200` | 1806492 |

### D.93 Factory 49286 — `g`

Source: `app.js`, object byte **1806499**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 50429; `c`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `g.stops` | `[{location:0,color:s.yP.black,alpha:0},{location:1,color:s.yP.black,alpha:.6}]` | 1806506 |
| `g.vector` | `[.07*c.nl.width,.26*c.nl.height,.03*c.nl.width,0]` | 1806592 |

### D.94 Factory 49286 — `p`

Source: `app.js`, object byte **1806645**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 50429; `c`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.stops` | `[{location:0,color:s.yP.black,alpha:0},{location:.59,color:s.yP.black,alpha:.6},{location:1,color:s.yP.black,alpha:.6}]` | 1806652 |
| `p.vector` | `[.2*c.nl.width,.29*c.nl.height,.01*c.nl.width,c.nl.height]` | 1806779 |

### D.95 Factory 49286 — `m`

Source: `app.js`, object byte **1806841**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 50429; `c`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.stops` | `[{location:0,color:s.yP.black,alpha:0},{location:1,color:s.yP.black,alpha:.6}]` | 1806848 |
| `m.vector` | `[.93*c.nl.width,.74*c.nl.height,.97*c.nl.width,c.nl.height]` | 1806934 |

### D.96 Factory 23220 — `T`

Source: `app.js`, object byte **1807781**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `w`→factory 16393; `C`→factory 3038; `b`→factory 46491; `A`→factory 90058.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T.paddingShelfItem.leading` | `S(16)` | 1807808 |
| `T.paddingShelfItem.top` | `S(14)` | 1807825 |
| `T.paddingShelfItem.bottom` | `S(8)` | 1807845 |
| `T.paddingModal.top` | `S(32)` | 1807876 |
| `T.paddingModal.bottom` | `S(26)` | 1807896 |
| `T.logo.width` | `S(64)` | 1807922 |
| `T.logo.height` | `S(64)` | 1807942 |
| `T.logo.fadeDuration` | `200*!!A.w.Config.Performance.otherAnimationsEnabled` | 1807968 |
| `T.titleFormat.typography` | `Typography.Emphasized.Body` | 1808053 |
| `T.titleFormat.color` | `Color.LabelPrimary` | 1808079 |
| `T.logoToTitleSpacing` | `S(12)` | 1808118 |

### D.97 Factory 23220 — `P`

Source: `app.js`, object byte **1808134**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `C`→factory 3038; `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `P.innerWidth` | `S(660)` | 1808146 |
| `P.height` | `S(38)` | 1808167 |
| `P.tableHeaderFormat.typography` | `Typography.Emphasized.Caption2` | 1808218 |
| `P.tableHeaderFormat.color` | `Color.LabelTertiary` | 1808248 |
| `P.padding.leading` | `S(160)` | 1808286 |
| `P.padding.trailing` | `S(40)` | 1808309 |
| `P.padding.top` | `S(4)` | 1808326 |
| `P.padding.bottom` | `S(4)` | 1808345 |

### D.98 Factory 23220 — `x`

Source: `app.js`, object byte **1808361**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `x.minimumTeamNameToDataSpacing` | `S(24)` | 1808391 |
| `x.dataColumnWidth` | `S(60)` | 1808420 |
| `x.dataColumnSpacing` | `S(24)` | 1808451 |

### D.99 Factory 23220 — `I`

Source: `app.js`, object byte **1808467**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `f`→factory 50429; `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `I.clinched.color` | `(e=u.Sx.dark)=>f.yP.from(Color.SystemGreen(e))` | 1808484 |
| `I.clinched.width` | `S(12)` | 1808536 |
| `I.clinched.padding.horizontal` | `S(1)` | 1808569 |
| `I.eliminated.color` | `(e=u.Sx.dark)=>f.yP.from(Color.SystemRed(e))` | 1808601 |
| `I.eliminated.cornerRadius` | `S(3)` | 1808658 |
| `I.eliminated.width` | `S(12)` | 1808676 |
| `I.eliminated.height` | `S(5)` | 1808696 |
| `I.eliminated.padding.horizontal` | `S(1)` | 1808728 |
| `I.none.width` | `S(14)` | 1808754 |

### D.100 Factory 23220 — `L`

Source: `app.js`, object byte **1808771**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `f`→factory 50429; `w`→factory 16393; `C`→factory 3038; `b`→factory 46491; `A`→factory 90058.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `L.padding.leading` | `S(18)` | 1808789 |
| `L.padding.trailing` | `S(40)` | 1808811 |
| `L.padding.top` | `S(11)` | 1808828 |
| `L.padding.bottom` | `S(11)` | 1808848 |
| `L.logo.width` | `S(50)` | 1808874 |
| `L.logo.height` | `S(50)` | 1808894 |
| `L.logo.padding.withFavorite.trailing` | `S(10)` | 1808939 |
| `L.logo.padding.withFavorite.leading` | `S(0)` | 1808960 |
| `L.logo.padding.withoutFavorite.trailing` | `S(10)` | 1808999 |
| `L.logo.padding.withoutFavorite.leading` | `S(22)` | 1809020 |
| `L.logo.fadeDuration` | `200*!!A.w.Config.Performance.otherAnimationsEnabled` | 1809048 |
| `L.favoriteIcon.idleColor` | `(e=u.Sx.dark)=>e===u.Sx.dark?f.yP.white:f.yP.black` | 1809125 |
| `L.favoriteIcon.focusedColor` | `()=>f.yP.black` | 1809189 |
| `L.favoriteIcon.width` | `S(20)` | 1809210 |
| `L.favoriteIcon.height` | `S(32)` | 1809230 |
| `L.favoriteIcon.padding.trailing` | `S(2)` | 1809261 |
| `L.ranking.width` | `S(38)` | 1809290 |
| `L.ranking.padding.trailing` | `S(8)` | 1809321 |
| `L.teamAbbreviationBreakpoint` | `S(124)` | 1809362 |
| `L.teamNameIdleFormat.typography` | `Typography.Emphasized.Caption1` | 1809415 |
| `L.teamNameIdleFormat.color` | `Color.LabelPrimary` | 1809445 |
| `L.teamNameFocusedFormat.typography` | `Typography.Emphasized.Caption1` | 1809507 |
| `L.teamNameFocusedFormat.color` | `Color.Black` | 1809537 |
| `L.teamDataIdleFormat.typography` | `Typography.Standard.Caption1` | 1809589 |
| `L.teamDataIdleFormat.color` | `Color.LabelPrimary` | 1809617 |
| `L.teamDataFocusedFormat.typography` | `Typography.Standard.Caption1` | 1809679 |
| `L.teamDataFocusedFormat.color` | `Color.Black` | 1809707 |

### D.101 Factory 23220 — `k`

Source: `app.js`, object byte **1809723**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `f`→factory 50429; `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `k.cornerRadius` | `S(12)` | 1809737 |
| `k.shadow.blurRadius` | `S(25)` | 1809769 |
| `k.shadow.dx` | `0` | 1809785 |
| `k.shadow.dy` | `S(20)` | 1809790 |
| `k.shadow.color` | `e=>f.yP.from(e?Color.Black40:Color.Black0)` | 1809809 |
| `k.backgroundColor` | `e=>e?Color.White:Color.White0` | 1809867 |

### D.102 Factory 23220 — `R`

Source: `app.js`, object byte **1809898**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `c`→factory 96638; `u`→factory 44548; `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `R.height` | `S(4)` | 1809906 |
| `R.padding.horizontal` | `S(20)` | 1809938 |
| `R.dashWidth` | `S(5)` | 1809962 |
| `R.dashGap` | `S(6)` | 1809982 |
| `R.lineHeight` | `S(1)` | 1810005 |
| `R.color` | `(e=u.Sx.dark)=>c.O.accessibility.focusStyle?e===u.Sx.dark?Color.White50:Color.Black50:e===u.Sx.dark?Color.White30:Color.Black30` | 1810023 |

### D.103 Factory 23220 — `D`

Source: `app.js`, object byte **1810150**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `u`→factory 44548; `C`→factory 3038; `b`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `D.padding.leading` | `S(18)` | 1810168 |
| `D.legendFormat.typography` | `Typography.Standard.Caption2` | 1810215 |
| `D.legendFormat.color` | `Color.LabelSecondary` | 1810243 |
| `D.keyToValueSpacing` | `S(8)` | 1810283 |
| `D.betweenItemsSpacing` | `S(24)` | 1810315 |

### D.104 Factory 65112 — `s`

Source: `app.js`, object byte **2032903**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `s.Default.imageSize` | `[S(900),S(652)]` | 2032923 |
| `s.Default.imageAlpha` | `.3` | 2032964 |
| `s.Default.loopingImageSize` | `[S(600),S(360)]` | 2032984 |
| `s.Subscriptions.imageSize` | `[S(900),S(652)]` | 2033040 |
| `s.Subscriptions.imageAlpha` | `1` | 2033081 |
| `s.Subscriptions.imageStyle` | `o` | 2033094 |
| `s.Subscriptions.loopingImageSize` | `[0,0]` | 2033113 |
| `s.Avatar.imageSize` | `[S(540),S(540)]` | 2033138 |
| `s.Avatar.imageAlpha` | `1` | 2033179 |
| `s.Avatar.loopingImageSize` | `[0,0]` | 2033198 |

### D.105 Factory 10710 — `d`

Source: `app.js`, object byte **2091807**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 41457.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `d.Default` | `s.JM.Regular.Default` | 2091816 |
| `d.Destructive` | `s.JM.Regular.Destructive` | 2091849 |
| `d.PrimaryAction` | `s.JM.Regular.KeyColor` | 2091888 |
| `d.SelectionMenu` | `l` | 2091924 |

### D.106 Factory 71706 — `l`

Source: `app.js`, object byte **2100732**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `l.defaultMaxHeight` | `S(920)` | 2100750 |
| `l.buttonBottomPadding` | `S(31)` | 2100784 |
| `l.defaultMinimumButtonsToShow` | `2` | 2100825 |
| `l.fade.distance` | `S(60)` | 2100842 |
| `l.fade.fadeWithScroll` | `S(30)` | 2100870 |
| `l.fade.clipExpand` | `S(22)` | 2100894 |
| `l.uniformPaddingMultiplier` | `2` | 2100933 |

### D.107 Factory 71706 — `d`

Source: `app.js`, object byte **2100938**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `d.inner` | `l.fade.distance` | 2100945 |
| `d.outer` | `S(0)` | 2100967 |
| `d.ellipsis` | `!0` | 2100988 |
| `d.fadeWithScroll` | `l.fade.fadeWithScroll` | 2101006 |
| `d.clipExpand` | `l.fade.clipExpand` | 2101039 |

### D.108 Factory 42048 — `m`

Source: `app.js`, object byte **2118220**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 18302; `a`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `m.light.settings.caption` | `n._V.symbolWithSize(r,540,540,Color.Black)` | 2118246 |
| `m.light.settings.gear` | `n._V.symbolWithSize(l,540,540,Color.Black)` | 2118293 |
| `m.light.settings.globe` | `n._V.symbolWithSize(d,540,540,Color.Black)` | 2118341 |
| `m.light.settings.location` | `n._V.symbolWithSize(o,540,540,Color.Black)` | 2118392 |
| `m.light.settings.accounts` | `n._V.symbolWithSize(s,540,540,Color.Black)` | 2118443 |
| `m.light.settings.clouds` | `[g,p]` | 2118492 |
| `m.light.alerts.geofencingIcon` | `n._V.symbolWithHeight(o,156,Color.Black)` | 2118522 |
| `m.dark.settings.caption` | `n._V.symbolWithSize(r,540,540,Color.White)` | 2118588 |
| `m.dark.settings.gear` | `n._V.symbolWithSize(l,540,540,Color.White)` | 2118635 |
| `m.dark.settings.globe` | `n._V.symbolWithSize(d,540,540,Color.White)` | 2118683 |
| `m.dark.settings.location` | `n._V.symbolWithSize(o,540,540,Color.White)` | 2118734 |
| `m.dark.settings.accounts` | `n._V.symbolWithSize(s,540,540,Color.White)` | 2118785 |
| `m.dark.settings.clouds` | `[g,p]` | 2118834 |
| `m.dark.alerts.geofencingIcon` | `n._V.symbolWithHeight(o,156,Color.White)` | 2118864 |

### D.109 Factory 65274 — `r{0}`

Source: `app.js`, object byte **2212798**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 44548; `a`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `r{0}.backgroundColor` | `e=>e===n.Sx.dark?Color.colorFromRGB(40,40,40,1):Color.colorFromRGB(211,211,211,1)` | 2212815 |
| `r{0}.radius` | `S(13)` | 2212902 |
| `r{0}.spacing` | `S(18)` | 2212923 |
| `r{0}.minimumRequiredContainerSize` | `S(180)` | 2212965 |

### D.110 Factory 50357 — `p`

Source: `app.js`, object byte **2256011**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 50429; `s`→factory 46491; `l`→factory 44548; `c`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `p.logo.maxWidth` | `S(195)` | 2256027 |
| `p.logo.height` | `S(117)` | 2256048 |
| `p.logo.cornerRadius` | `S(12)` | 2256075 |
| `p.logo.shadowColor` | `o.yP.from(Color.Black40)` | 2256100 |
| `p.logo.shadowOffset` | `[0,S(7)]` | 2256137 |
| `p.logo.shadowBlurRadius` | `S(12)` | 2256170 |
| `p.logo.fadeDuration` | `240` | 2256196 |
| `p.logo.placeholderBackground` | `e=>o.yP.from(e===l.Sx.light?Color.Material.MediumLight:Color.Material.MediumDark)` | 2256222 |
| `p.logo.outlineColor` | `o.yP.from(Color.borderColor())` | 2256315 |
| `p.hairline.thickness` | `S(1)` | 2256366 |
| `p.hairline.color` | `Color.LabelTertiary` | 2256384 |
| `p.sizes.informationWidth` | `S(760)` | 2256428 |
| `p.sizes.buttonWidth` | `S(500)` | 2256454 |
| `p.sizes.titleHeight` | `S(76)` | 2256480 |
| `p.sizes.footerHeight` | `S(76)` | 2256506 |
| `p.spacings.logoToMetadata` | `S(30)` | 2256545 |
| `p.spacings.headerToContent` | `S(30)` | 2256574 |
| `p.spacings.buttons` | `S(14)` | 2256595 |
| `p.spacings.details` | `S(30)` | 2256616 |
| `p.spacings.bodyVertical` | `S(25)` | 2256642 |
| `p.spacings.footerTop` | `S(25)` | 2256665 |
| `p.fonts.headerTitle.typography` | `Typography.Emphasized.Caption1` | 2256718 |
| `p.fonts.headerTitle.color` | `Color.LabelPrimary` | 2256748 |
| `p.fonts.headerSubtitle.typography` | `Typography.Standard.Caption1` | 2256803 |
| `p.fonts.headerSubtitle.color` | `Color.LabelSecondary` | 2256831 |
| `p.fonts.headerSingleLine.typography` | `Typography.Emphasized.Callout` | 2256890 |
| `p.fonts.headerSingleLine.color` | `Color.LabelPrimary` | 2256919 |
| `p.fonts.title.typography` | `Typography.Emphasized.Callout` | 2256965 |
| `p.fonts.title.color` | `Color.LabelPrimary` | 2256994 |
| `p.fonts.body.typography` | `Typography.Standard.Caption1` | 2257039 |
| `p.fonts.body.color` | `Color.LabelPrimary` | 2257067 |
| `p.fonts.footer.typography` | `Typography.Standard.Caption1` | 2257114 |
| `p.fonts.footer.color` | `Color.LabelSecondary` | 2257142 |
| `p.fonts.footerInfo.typography` | `Typography.Standard.Caption1` | 2257195 |
| `p.fonts.footerInfo.color` | `Color.LabelPrimary` | 2257223 |
| `p.fonts.bodyInfo.typography` | `Typography.Standard.Caption1` | 2257272 |
| `p.fonts.bodyInfo.color` | `Color.LabelPrimary` | 2257300 |

### D.111 Factory 33239 — `u`

Source: `app.js`, object byte **2262339**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 12270.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `u.Default` | `(0,d.B)(()=>c(!1))` | 2262348 |
| `u.AdjustedFont` | `(0,d.B)(()=>c(!0))` | 2262380 |

### D.112 Factory 69583 — `T{0}`

Source: `app.js`, object byte **2298033**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T{0}.width` | `S(36)` | 2298040 |
| `T{0}.height` | `S(24)` | 2298060 |
| `T{0}.top` | `S(8)` | 2298077 |
| `T{0}.leading` | `S(8)` | 2298097 |

### D.113 Factory 69583 — `T{1}`

Source: `app.js`, object byte **2298124**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `s`→factory 46491; `l`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T{1}.background.color` | `new o.Q1(Color.Semi50)` | 2298143 |
| `T{1}.padding.leading` | `S(30)` | 2298183 |
| `T{1}.padding.trailing` | `S(30)` | 2298205 |
| `T{1}.leading.spacing` | `S(12)` | 2298236 |
| `T{1}.title.format.typography` | `Typography.Standard.Body` | 2298284 |
| `T{1}.title.format.color` | `Color.White` | 2298308 |
| `T{1}.title.truncation` | `a.Fade` | 2298332 |

### D.114 Factory 69583 — `T{2}`

Source: `app.js`, object byte **2298349**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `s`→factory 46491; `l`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T{2}.background.color` | `new o.Q1(Color.White90)` | 2298368 |
| `T{2}.padding.leading` | `S(30)` | 2298409 |
| `T{2}.padding.trailing` | `S(30)` | 2298431 |
| `T{2}.leading.spacing` | `S(12)` | 2298462 |
| `T{2}.title.format.typography` | `Typography.Standard.Body` | 2298510 |
| `T{2}.title.format.color` | `Color.Black` | 2298534 |
| `T{2}.title.truncation` | `a.Rolling` | 2298558 |
| `T{2}.useShadow` | `!0` | 2298579 |

### D.115 Factory 69583 — `T{3}`

Source: `app.js`, object byte **2298599**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `s`→factory 46491; `l`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T{3}.background.color` | `new o.Q1(Color.SettingsButtonPlatter)` | 2298618 |
| `T{3}.padding.leading` | `S(30)` | 2298673 |
| `T{3}.padding.trailing` | `S(30)` | 2298695 |
| `T{3}.leading.spacing` | `S(12)` | 2298726 |
| `T{3}.title.format.typography` | `Typography.Emphasized.Body` | 2298774 |
| `T{3}.title.format.color` | `Color.LabelTertiary` | 2298800 |
| `T{3}.title.spacing` | `S(40)` | 2298829 |
| `T{3}.title.truncation` | `a.Fade` | 2298853 |
| `T{3}.decorator.format.typography` | `Typography.Standard.Body` | 2298899 |
| `T{3}.decorator.format.color` | `Color.LabelTertiary` | 2298923 |
| `T{3}.decorator.spacing` | `S(15)` | 2298952 |
| `T{3}.trailing.opacity` | `.3` | 2298984 |

### D.116 Factory 69583 — `T{4}`

Source: `app.js`, object byte **2298997**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `o`→factory 44548; `s`→factory 46491; `l`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `T{4}.background.color` | `new o.Q1(Color.White90)` | 2299016 |
| `T{4}.padding.leading` | `S(30)` | 2299057 |
| `T{4}.padding.trailing` | `S(30)` | 2299079 |
| `T{4}.leading.spacing` | `S(12)` | 2299110 |
| `T{4}.title.format.typography` | `Typography.Standard.Body` | 2299158 |
| `T{4}.title.format.color` | `Color.LabelTertiaryReversed` | 2299182 |
| `T{4}.title.spacing` | `S(40)` | 2299219 |
| `T{4}.title.truncation` | `a.Rolling` | 2299243 |
| `T{4}.decorator.format.typography` | `Typography.Standard.Body` | 2299292 |
| `T{4}.decorator.format.color` | `Color.LabelTertiaryReversed` | 2299316 |
| `T{4}.decorator.spacing` | `S(15)` | 2299353 |
| `T{4}.trailing.opacity` | `.4` | 2299385 |
| `T{4}.useShadow` | `!0` | 2299399 |

### D.117 Factory 69583 — `P`

Source: `app.js`, object byte **2299409**. Values are static source declarations; offsets in the last column locate each original value.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `P.background` | `void 0` | 2299421 |
| `P.title` | `void 0` | 2299434 |
| `P.useShadow` | `!1` | 2299451 |

### D.118 Factory 69583 — `I`

Source: `app.js`, object byte **2299587**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `s`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `I.Default` | `T` | 2299596 |
| `I.ImageOnly` | `x` | 2299608 |
| `I.minWidth` | `S(340)` | 2299619 |

### D.119 Factory 48302 — `h{0}`

Source: `app.js`, object byte **2301849**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `l`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `h{0}.width` | `S(42)` | 2301856 |
| `h{0}.height` | `S(56)` | 2301876 |
| `h{0}.spacing` | `S(43)` | 2301897 |

### D.120 Factory 95561 — `c`

Source: `app.js`, object byte **2336207**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `c.defaultSize` | `S(225)` | 2336220 |
| `c.defaultFontSize` | `106` | 2336250 |
| `c.defaultFontLeading` | `138` | 2336273 |
| `c.jOffset` | `8` | 2336285 |
| `c.backgroundGradient.start` | `new(a()).Gfx.Color(165/255,171/255,184/255)` | 2336313 |
| `c.backgroundGradient.end` | `new(a()).Gfx.Color(132/255,137/255,147/255)` | 2336361 |

### D.121 Factory 17550 — `l`

Source: `app.js`, object byte **2354176**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `n`→factory 46491; `a`→factory 44548; `o`→factory 3038; `s`→factory 38594.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `l.maxWidth` | `S(600)` | 2354186 |
| `l.minWidth` | `S(200)` | 2354209 |
| `l.maxHeight` | `S(225)` | 2354233 |
| `l.minHeight` | `S(112)` | 2354257 |
| `l.spacingForMaxHeight` | `S(23)` | 2354291 |
| `l.paddingWithTextOnly.leading` | `S(30)` | 2354333 |
| `l.paddingWithTextOnly.trailing` | `S(40)` | 2354355 |
| `l.paddingWithIconAndText.leading` | `S(21)` | 2354401 |
| `l.paddingWithIconAndText.trailing` | `S(40)` | 2354423 |
| `l.shadow.color` | `Color.colorFromRGB(0,0,0,.15)` | 2354451 |
| `l.shadow.offset` | `[0,S(15)]` | 2354487 |
| `l.shadow.blurRadius` | `S(36)` | 2354515 |
| `l.padding` | `Object.assign(Object.assign({},n.nl.SafeArea.padding),{top:S(57)})` | 2354537 |
| `l.iconTextSpacing` | `S(21)` | 2354627 |
| `l.iconWidth` | `S(70)` | 2354650 |
| `l.iconHeight` | `S(70)` | 2354674 |
| `l.iconSymbolSize` | `70` | 2354702 |
| `l.iconColor` | `Color.LabelSecondary()` | 2354715 |
| `l.cornerRadius` | `S(30)` | 2354750 |
| `l.bgColor` | `Color.SystemGray5()` | 2354771 |
| `l.bgMaterial` | `s.Gfx.DrawingContext.MaterialType.Regular` | 2354801 |
| `l.bgMaterialTheme` | `s.Gfx.DrawingContext.MaterialTheme.Dark` | 2354859 |
| `l.stroke.color` | `Color.borderColor()` | 2354913 |
| `l.stroke.width` | `S(1)` | 2354938 |
| `l.title.typography` | `Typography.Emphasized.Caption1` | 2354977 |
| `l.title.color` | `Color.LabelPrimary` | 2355007 |
| `l.title.paragraphSpacingAfter` | `0` | 2355047 |
| `l.title.pageAlignment` | `.5` | 2355063 |
| `l.titleOnly.typography` | `Typography.Standard.Caption1` | 2355098 |
| `l.titleOnly.color` | `Color.LabelPrimary` | 2355126 |
| `l.titleOnly.paragraphSpacingAfter` | `0` | 2355166 |
| `l.titleOnly.pageAlignment` | `.5` | 2355182 |
| `l.body.typography` | `Typography.Standard.Caption2` | 2355212 |
| `l.body.color` | `Color.LabelSecondary` | 2355240 |
| `l.body.pageAlignment` | `.5` | 2355274 |

### D.122 Factory 90118 — `d`

Source: `app.js`, object byte **2355410**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 38594; `l`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `d.width` | `S(106)` | 2355417 |
| `d.height` | `S(180)` | 2355438 |
| `d.background` | `new r.Gfx.Color(67/255,69/255,71/255)` | 2355463 |
| `d.spacing` | `S(20)` | 2355509 |
| `d.centerSpacing` | `S(40)` | 2355536 |

### D.123 Factory 75262 — `k`

Source: `app.js`, object byte **2373007**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `l`→factory 50429; `v`→factory 44548; `A`→factory 46491; `L`→factory 3038.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `k.orientation` | `1` | 2373020 |
| `k.width.default` | `S(530)` | 2373037 |
| `k.width.max` | `S(740)` | 2373055 |
| `k.qrDataWidth.default` | `S(530)` | 2373091 |
| `k.qrDataWidth.max` | `S(660)` | 2373109 |
| `k.cornerRadius` | `S(50)` | 2373137 |
| `k.qrCode.focusable.dataColor` | `Color.White` | 2373179 |
| `k.qrCode.focusable.cornerRadius` | `S(26)` | 2373203 |
| `k.qrCode.focusable.inset` | `S(25)` | 2373222 |
| `k.qrCode.focusable.axCursorPadding` | `S(20)` | 2373251 |
| `k.qrCode.focusable.size.default` | `S(340)` | 2373278 |
| `k.qrCode.focusable.size.max` | `S(270)` | 2373296 |
| `k.qrCode.idle.dataColor` | `Color.White` | 2373328 |
| `k.qrCode.idle.cornerRadius` | `S(26)` | 2373352 |
| `k.qrCode.idle.inset` | `S(25)` | 2373371 |
| `k.qrCode.idle.axCursorPadding` | `S(20)` | 2373400 |
| `k.qrCode.idle.size.default` | `S(340)` | 2373427 |
| `k.qrCode.idle.size.max` | `S(270)` | 2373445 |
| `k.qrCode.imageAlignment` | `.5` | 2373476 |
| `k.qrDataPadding.leading.default` | `S(45)` | 2373512 |
| `k.qrDataPadding.leading.max` | `S(40)` | 2373529 |
| `k.qrDataPadding.trailing.default` | `S(45)` | 2373561 |
| `k.qrDataPadding.trailing.max` | `S(40)` | 2373578 |
| `k.qrDataPadding.bottom` | `S(7)` | 2373599 |
| `k.qrInstructionsAlignment.horizontal` | `l.OD.center` | 2373648 |
| `k.qrInstructionsAlignment.vertical` | `l.sx.center` | 2373669 |
| `k.qrDataAlignment.horizontal` | `.5` | 2373710 |
| `k.qrDataAlignment.vertical` | `.5` | 2373722 |
| `k.platterPadding.leading.default` | `S(45)` | 2373759 |
| `k.platterPadding.leading.max` | `S(0)` | 2373776 |
| `k.platterPadding.trailing.default` | `S(45)` | 2373807 |
| `k.platterPadding.trailing.max` | `S(0)` | 2373824 |
| `k.platterPadding.top.default` | `S(95)` | 2373850 |
| `k.platterPadding.top.max` | `S(95)` | 2373867 |
| `k.platterPadding.bottom.default` | `S(88)` | 2373897 |
| `k.platterPadding.bottom.max` | `S(88)` | 2373914 |
| `k.instructionsAXCursor.padding` | `S(15)` | 2373959 |
| `k.instructionsAXCursor.cornerRadius` | `S(12)` | 2373985 |
| `k.hairline.thickness` | `S(1)` | 2374019 |
| `k.hairline.color` | `Color.LabelTertiary` | 2374037 |
| `k.spacings.qrCodeToInstructions` | `S(24)` | 2374088 |
| `k.spacings.instructionsTitleToText.default` | `S(9)` | 2374134 |
| `k.spacings.instructionsTitleToText.max` | `S(20)` | 2374150 |
| `k.spacings.instructionsToDivider.default` | `S(20)` | 2374195 |
| `k.spacings.instructionsToDivider.max` | `S(30)` | 2374212 |
| `k.spacings.textBetweenDivider` | `S(16)` | 2374245 |
| `k.spacings.dividerToButton` | `S(23)` | 2374274 |
| `k.spacings.buttonToButton` | `S(14)` | 2374302 |
| `k.colors.platter` | `Color.White5` | 2374332 |
| `k.text.instructionsTitle.format.typography` | `Typography.Emphasized.Body` | 2374397 |
| `k.text.instructionsTitle.format.color` | `Color.LabelPrimary` | 2374423 |
| `k.text.instructionsTitle.format.textAlignment` | `.5` | 2374455 |
| `k.text.instructionsTitle.minSize` | `Typography.Emphasized.Body.size` | 2374468 |
| `k.text.instructionsTitle.maxLines` | `10` | 2374502 |
| `k.text.instructions.format.typography` | `Typography.Standard.Caption1` | 2374547 |
| `k.text.instructions.format.color` | `Color.HighContrastQuaternary` | 2374575 |
| `k.text.instructions.format.textAlignment` | `.5` | 2374617 |
| `k.text.instructions.minSize` | `Typography.Standard.Caption1.size` | 2374630 |
| `k.text.instructions.maxLines` | `10` | 2374666 |
| `k.text.instructionsHighlight.format.typography` | `Typography.Emphasized.Caption1` | 2374720 |
| `k.text.instructionsHighlight.format.color` | `Color.LabelPrimary` | 2374750 |
| `k.text.instructionsHighlight.format.textAlignment` | `.5` | 2374782 |
| `k.text.instructionsHighlight.minSize` | `Typography.Emphasized.Caption1.size` | 2374795 |
| `k.text.dividerText.format.typography` | `Typography.Emphasized.Body` | 2374865 |
| `k.text.dividerText.format.color` | `Color.LabelTertiary` | 2374891 |
| `k.text.dividerText.format.textAlignment` | `.5` | 2374924 |

### D.124 Factory 75262 — `R`

Source: `app.js`, object byte **2374934**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `A`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `R.width.default` | `S(700)` | 2374950 |
| `R.width.max` | `S(950)` | 2374968 |
| `R.qr.focusable` | `S(320)` | 2374997 |
| `R.qr.idle` | `S(270)` | 2375016 |
| `R.safeArea` | `S(30)` | 2375040 |

### D.125 Factory 75262 — `D{0}`

Source: `app.js`, object byte **2375229**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `v`→factory 44548; `A`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `D{0}.focusedBackgroundColor` | `Color.White` | 2375253 |
| `D{0}.backgroundColor` | `Color.White10` | 2375280 |
| `D{0}.focusedDataColor` | `Color.Black` | 2375310 |
| `D{0}.inset` | `S(25)` | 2375327 |
| `D{0}.size` | `R.qr.focusable` | 2375345 |

### D.126 Factory 75262 — `D{1}`

Source: `app.js`, object byte **2375413**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `v`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `D{1}.focusedBackgroundColor` | `Color.White0` | 2375437 |
| `D{1}.backgroundColor` | `Color.White0` | 2375465 |
| `D{1}.focusedDataColor` | `Color.White` | 2375494 |
| `D{1}.size` | `R.qr.idle` | 2375510 |

### D.127 Factory 75262 — `E`

Source: `app.js`, object byte **2376676**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `A`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `E.totalWidth` | `S(900)` | 2376688 |
| `E.qr.focusable` | `S(320)` | 2376716 |
| `E.qr.idle` | `S(270)` | 2376735 |
| `E.safeArea` | `S(30)` | 2376759 |

### D.128 Factory 26769 — `g`

Source: `app.js`, object byte **2400680**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 44548.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `g.size` | `SpinnerWidget_SpinnerWidget.Size.Large` | 2400686 |
| `g.duration` | `1e3` | 2400734 |
| `g.holdTime` | `2e3` | 2400747 |
| `g.fadeInDuration` | `400` | 2400766 |
| `g.useAbsoluteTime` | `!1` | 2400786 |
| `g.color` | `Color.White` | 2400795 |

### D.129 Factory 90180 — `C`

Source: `app.js`, object byte **2403782**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `v`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `C.defaultBulletIndent` | `S(20)` | 2403803 |
| `C.defaultAccessibilityCursorSize` | `S(15)` | 2403847 |
| `C.rollAnimation.speed` | `S(30)` | 2403881 |
| `C.rollAnimation.textSpacing` | `S(150)` | 2403906 |
| `C.rollAnimation.startDelay` | `2700` | 2403931 |
| `C.rollAnimation.fadeInTime` | `300` | 2403947 |
| `C.rollAnimation.fadeInOut` | `.05` | 2403961 |

### D.130 Factory 49336 — `u`

Source: `app.js`, object byte **2426587**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `r`→factory 44548; `o`→factory 84556; `d`→factory 3038; `c`→factory 46491.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `u.bodyFormatWithVoiceDictation.typography` | `Typography.Standard.Body` | 2426637 |
| `u.bodyFormatWithVoiceDictation.color` | `Color.LabelSecondary` | 2426661 |
| `u.bodyFormatWithVoiceDictation.textAlignment` | `.5` | 2426695 |
| `u.bodyFormatWithVoiceDictation.pageAlignment` | `.5` | 2426712 |
| `u.voiceFormatUnfocused.typography` | `Typography.Standard.Headline` | 2426758 |
| `u.voiceFormatUnfocused.color` | `Color.LabelSecondary` | 2426786 |
| `u.voiceFormatUnfocused.textAlignment` | `.5` | 2426820 |
| `u.voiceFormatUnfocused.pageAlignment` | `.5` | 2426837 |
| `u.voiceFormatFocused.typography` | `Typography.Standard.Caption2` | 2426881 |
| `u.voiceFormatFocused.color` | `Color.LabelPrimary` | 2426909 |
| `u.voiceFormatFocused.textAlignment` | `.5` | 2426941 |
| `u.voiceFormatFocused.pageAlignment` | `.5` | 2426958 |
| `u.focusedMicSize.height` | `S(22)` | 2426986 |
| `u.focusedMicSize.width` | `S(22)` | 2427005 |
| `u.unfocusedMicSize.height` | `S(40)` | 2427044 |
| `u.unfocusedMicSize.width` | `S(40)` | 2427063 |
| `u.listWidgetSize.height` | `S(85)` | 2427100 |
| `u.micImageAlignment.pageAlignment` | `.5` | 2427147 |
| `u.padding.top` | `S(56)` | 2427164 |
| `u.padding.bottom` | `S(56)` | 2427184 |
| `u.glyphSpacing` | `S(10)` | 2427211 |
| `u.widgetWidth` | `S(680)` | 2427236 |

### D.131 Factory 25654 — `L`

Source: `app.js`, object byte **2581272**. Values are static source declarations; offsets in the last column locate each original value.

Source import aliases: `d`→factory 21456.

| Property | Value / dependency / rule | Original value byte |
| --- | --- | --- |
| `L.cb` | `e=>e` | 2581276 |
| `L.animParams.type` | `d.Y5.Spring` | 2581298 |
| `L.animParams.damping` | `30` | 2581318 |
| `L.animParams.stiffness` | `150` | 2581331 |
| `L.animParams.mass` | `1` | 2581340 |
| `L.mode` | `o.Mode.Peek` | 2581348 |
| `L.pointerConfig.enabled` | `!1` | 2581383 |

### D. End of declaration appendix

21 long/computed/asset values in this appendix retain a source locator instead of a source-body transcription. Their names and dependency positions are inventoried; their final rendered values are not resolved. Shared foundation factories already expanded above were excluded here to reduce duplicate rows.

## E. Language-specific keyboard tokens

These declarations refine the shared TextInput keyboard styles in section B. Language key ordering and key labels alone are content, not design tokens. The rows below preserve glyph-part identities where needed to explain per-glyph offsets and formatting. Repeated paths are different parts/occurrences, distinguished by byte position. Alias references remain local to the named bundle. This is static declaration evidence, not proof of the active input language.

### E. `monolith-bengali.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 80790 | `tabs.layout` | `i.Un` | 201 |
| 80790 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 80790 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 80790 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |

### E. `monolith-gujarati.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 3893 | `tabs.layout` | `i.Un` | 200 |
| 3893 | `tabs.contextMenu.layout` | `l.rx.Row` | 292 |
| 3893 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 308 |
| 3893 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 332 |

### E. `monolith-hindi.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 73724 | `tabs.layout` | `d.Un` | 201 |

### E. `monolith-kana.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 97135 | `typography.weight` | `n.I.Bold` | 204 |
| 97135 | `typography.size` | `25` | 220 |
| 97135 | `typography.leading` | `56` | 228 |
| 97135 | `textAlignment` | `.5` | 240 |
| 97135 | `pageAlignment` | `.5` | 257 |
| 97135 | `compositions.わ.parts.character` | `"\u308F"` | 308 |
| 97135 | `compositions.わ.parts.offset` | `new s.V(0,(0,i.yA)(1))` | 327 |
| 97135 | `compositions.わ.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Small))` | 357 |
| 97135 | `compositions.わ.parts.character` | `"\u30FC"` | 420 |
| 97135 | `compositions.わ.parts.offset` | `new s.V((0,i.yA)(15),(0,i.yA)(28))` | 439 |
| 97135 | `compositions.わ.parts.textFormat` | `new r.l(Object.assign({},d))` | 481 |
| 97135 | `compositions.小.parts.character` | `"\u5C0F"` | 540 |
| 97135 | `compositions.小.parts.offset` | `new s.V(0,(0,i.yA)(9))` | 559 |
| 97135 | `compositions.小.parts.textFormat` | `new r.l(Object.assign({},d))` | 589 |
| 97135 | `compositions.小.parts.character` | `"\u3099"` | 631 |
| 97135 | `compositions.小.parts.offset` | `new s.V((0,i.yA)(-15),(0,i.yA)(-5))` | 650 |
| 97135 | `compositions.小.parts.textFormat` | `new r.l(Object.assign({},d))` | 693 |
| 97135 | `compositions.小.parts.character` | `"\u309C"` | 735 |
| 97135 | `compositions.小.parts.offset` | `new s.V((0,i.yA)(20),(0,i.yA)(-5))` | 754 |
| 97135 | `compositions.小.parts.textFormat` | `new r.l(Object.assign({},d))` | 796 |
| 97135 | `compositions.，.parts.character` | `"\uFF0C"` | 857 |
| 97135 | `compositions.，.parts.offset` | `new s.V((0,i.yA)(19),0)` | 876 |
| 97135 | `compositions.，.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Default))` | 907 |
| 97135 | `compositions.。.parts.character` | `"\u3002"` | 991 |
| 97135 | `compositions.。.parts.offset` | `new s.V((0,i.yA)(14),0)` | 1010 |
| 97135 | `compositions.。.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Default))` | 1041 |
| 97135 | `compositions.、.parts.character` | `"\u3001"` | 1125 |
| 97135 | `compositions.、.parts.offset` | `new s.V((0,i.yA)(14),0)` | 1144 |
| 97135 | `compositions.、.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Default))` | 1175 |
| 97135 | `compositions.「.parts.character` | `"\u300C"` | 1259 |
| 97135 | `compositions.「.parts.offset` | `new s.V((0,i.yA)(-14),0)` | 1278 |
| 97135 | `compositions.「.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Default))` | 1310 |
| 97135 | `compositions.」.parts.character` | `"\u300D"` | 1394 |
| 97135 | `compositions.」.parts.offset` | `new s.V((0,i.yA)(14),0)` | 1413 |
| 97135 | `compositions.」.parts.textFormat` | `new r.l(Object.assign({},l.$P.StandardKey.Default))` | 1444 |
| 97135 | `tabs.layout` | `c.H` | 1593 |
| 97135 | `tabs.contextMenu.layout` | `y.rx.Plus` | 1708 |
| 97135 | `tabs.contextMenu.cursorType` | `y.hM.Square` | 1725 |
| 97135 | `tabs.contextMenu.keySizing` | `y.mW.Fixed` | 1748 |

### E. `monolith-kannada.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 24576 | `tabs.layout` | `i.Un` | 201 |
| 24576 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 24576 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 24576 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |

### E. `monolith-malayalam.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 77831 | `tabs.layout` | `o.Un` | 201 |
| 77831 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 77831 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 77831 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |
| 77831 | `tabs.layout` | `o.Lf` | 934 |

### E. `monolith-original-kana.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 1244 | `tabs.layout` | `s.$s` | 209 |
| 1244 | `tabs.layout` | `s.$s` | 792 |

### E. `monolith-oriya.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 53966 | `tabs.layout` | `i.Un` | 201 |
| 53966 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 53966 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 53966 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |

### E. `monolith-pinyin.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 54455 | `，.parts.character` | `"\uFF0C"` | 234 |
| 54455 | `，.parts.offset` | `new l.V((0,o.yA)(14),0)` | 253 |
| 54455 | `，.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 284 |
| 54455 | `。.parts.character` | `"\u3002"` | 368 |
| 54455 | `。.parts.offset` | `new l.V((0,o.yA)(12),0)` | 387 |
| 54455 | `。.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 418 |
| 54455 | `、.parts.character` | `"\u3001"` | 502 |
| 54455 | `、.parts.offset` | `new l.V((0,o.yA)(12),0)` | 521 |
| 54455 | `、.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 552 |
| 54455 | `【.parts.character` | `"\u3010"` | 636 |
| 54455 | `【.parts.offset` | `new l.V((0,o.yA)(-14),(0,o.yA)(4))` | 655 |
| 54455 | `【.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 697 |
| 54455 | `】.parts.character` | `"\u3011"` | 781 |
| 54455 | `】.parts.offset` | `new l.V((0,o.yA)(14),(0,o.yA)(4))` | 800 |
| 54455 | `】.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 841 |
| 54455 | `《.parts.character` | `"\u300A"` | 925 |
| 54455 | `《.parts.offset` | `new l.V((0,o.yA)(-10),(0,o.yA)(4))` | 944 |
| 54455 | `《.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 986 |
| 54455 | `》.parts.character` | `"\u300B"` | 1070 |
| 54455 | `》.parts.offset` | `new l.V((0,o.yA)(10),(0,o.yA)(4))` | 1089 |
| 54455 | `》.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1130 |
| 54455 | `tabs.layout` | `r.Lf` | 1288 |

### E. `monolith-romaji.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 90786 | `−.parts.character` | `"\u2212"` | 234 |
| 90786 | `−.parts.offset` | `new y.V(0,0)` | 253 |
| 90786 | `−.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 273 |
| 90786 | `，.parts.character` | `"\uFF0C"` | 357 |
| 90786 | `，.parts.offset` | `new y.V(19,0)` | 376 |
| 90786 | `，.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 397 |
| 90786 | `。.parts.character` | `"\u3002"` | 481 |
| 90786 | `。.parts.offset` | `new y.V((0,c.yA)(14),0)` | 500 |
| 90786 | `。.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 531 |
| 90786 | `、.parts.character` | `"\u3001"` | 615 |
| 90786 | `、.parts.offset` | `new y.V((0,c.yA)(14),0)` | 634 |
| 90786 | `、.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 665 |
| 90786 | `「.parts.character` | `"\u300C"` | 749 |
| 90786 | `「.parts.offset` | `new y.V((0,c.yA)(-14),0)` | 768 |
| 90786 | `「.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 800 |
| 90786 | `」.parts.character` | `"\u300D"` | 884 |
| 90786 | `」.parts.offset` | `new y.V((0,c.yA)(14),0)` | 903 |
| 90786 | `」.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 934 |
| 90786 | `tabs.layout` | `r.Lf` | 1092 |

### E. `monolith-stroke.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 31002 | `，.parts.character` | `"\uFF0C"` | 234 |
| 31002 | `，.parts.offset` | `new y.V(0,(0,c.yA)(4))` | 253 |
| 31002 | `，.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 283 |
| 31002 | `。.parts.character` | `"\u3002"` | 367 |
| 31002 | `。.parts.offset` | `new y.V(0,(0,c.yA)(4))` | 386 |
| 31002 | `。.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 416 |
| 31002 | `、.parts.character` | `"\u3001"` | 500 |
| 31002 | `、.parts.offset` | `new y.V(0,(0,c.yA)(4))` | 519 |
| 31002 | `、.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 549 |
| 31002 | `「.parts.character` | `"\u300C"` | 633 |
| 31002 | `「.parts.offset` | `new y.V((0,c.yA)(-14),0)` | 652 |
| 31002 | `「.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 684 |
| 31002 | `」.parts.character` | `"\u300D"` | 768 |
| 31002 | `」.parts.offset` | `new y.V((0,c.yA)(14),0)` | 787 |
| 31002 | `」.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 818 |
| 31002 | `《.parts.character` | `"\u300A"` | 902 |
| 31002 | `《.parts.offset` | `new y.V((0,c.yA)(-10),0)` | 921 |
| 31002 | `《.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 953 |
| 31002 | `》.parts.character` | `"\u300B"` | 1037 |
| 31002 | `》.parts.offset` | `new y.V((0,c.yA)(10),0)` | 1056 |
| 31002 | `》.parts.textFormat` | `new o.l(Object.assign({},n.$P.StandardKey.Default))` | 1087 |
| 31002 | `tabs.layout` | `r.Lf` | 1254 |

### E. `monolith-sucheng.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 48269 | `，.parts.character` | `"\uFF0C"` | 235 |
| 48269 | `，.parts.offset` | `new l.V(0,(0,c.yA)(4))` | 254 |
| 48269 | `，.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 284 |
| 48269 | `。.parts.character` | `"\u3002"` | 368 |
| 48269 | `。.parts.offset` | `new l.V(0,(0,c.yA)(4))` | 387 |
| 48269 | `。.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 417 |
| 48269 | `、.parts.character` | `"\u3001"` | 501 |
| 48269 | `、.parts.offset` | `new l.V(0,(0,c.yA)(4))` | 520 |
| 48269 | `、.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 550 |
| 48269 | `「.parts.character` | `"\u300C"` | 634 |
| 48269 | `「.parts.offset` | `new l.V((0,c.yA)(-14),0)` | 653 |
| 48269 | `「.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 685 |
| 48269 | `」.parts.character` | `"\u300D"` | 769 |
| 48269 | `」.parts.offset` | `new l.V((0,c.yA)(14),0)` | 788 |
| 48269 | `」.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 819 |
| 48269 | `《.parts.character` | `"\u300A"` | 903 |
| 48269 | `《.parts.offset` | `new l.V((0,c.yA)(-10),0)` | 922 |
| 48269 | `《.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 954 |
| 48269 | `》.parts.character` | `"\u300B"` | 1038 |
| 48269 | `》.parts.offset` | `new l.V((0,c.yA)(10),0)` | 1057 |
| 48269 | `》.parts.textFormat` | `new o.l(Object.assign({},y.$P.StandardKey.Default))` | 1088 |
| 48269 | `tabs.layout` | `r.XT` | 1255 |
| 48269 | `tabs.contextMenu.layout` | `s.rx.Row` | 1371 |
| 48269 | `tabs.contextMenu.cursorType` | `s.hM.Square` | 1387 |
| 48269 | `tabs.contextMenu.keySizing` | `s.mW.Fixed` | 1410 |

### E. `monolith-tamil.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 60983 | `tabs.layout` | `o.Lf` | 201 |
| 60983 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 60983 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 60983 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |
| 60983 | `tabs.layout` | `o.Lf` | 801 |

### E. `monolith-telugu.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 13518 | `tabs.layout` | `i.Un` | 201 |
| 13518 | `tabs.contextMenu.layout` | `l.rx.Row` | 299 |
| 13518 | `tabs.contextMenu.cursorType` | `l.hM.Default` | 315 |
| 13518 | `tabs.contextMenu.keySizing` | `l.mW.Dynamic` | 339 |

### E. `monolith-thai.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 10356 | `ำ.parts.character` | `"\u25CC"` | 265 |
| 10356 | `ำ.parts.offset` | `new r.V(-(0,n.yA)(12),(0,n.yA)(2))` | 284 |
| 10356 | `ำ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 326 |
| 10356 | `ำ.parts.character` | `"\u0E33"` | 395 |
| 10356 | `ำ.parts.offset` | `new r.V((0,n.yA)(12),0)` | 414 |
| 10356 | `ำ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 445 |
| 10356 | `ั.parts.character` | `"\u25CC"` | 529 |
| 10356 | `ั.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 548 |
| 10356 | `ั.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 578 |
| 10356 | `ั.parts.character` | `"\u0E31"` | 647 |
| 10356 | `ั.parts.offset` | `new r.V((0,n.yA)(14),-(0,n.yA)(2))` | 666 |
| 10356 | `ั.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 708 |
| 10356 | `ิ.parts.character` | `"\u25CC"` | 792 |
| 10356 | `ิ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 811 |
| 10356 | `ิ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 841 |
| 10356 | `ิ.parts.character` | `"\u0E34"` | 910 |
| 10356 | `ิ.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 929 |
| 10356 | `ิ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 971 |
| 10356 | `ี.parts.character` | `"\u25CC"` | 1055 |
| 10356 | `ี.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 1074 |
| 10356 | `ี.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 1104 |
| 10356 | `ี.parts.character` | `"\u0E35"` | 1173 |
| 10356 | `ี.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 1192 |
| 10356 | `ี.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 1234 |
| 10356 | `ึ.parts.character` | `"\u25CC"` | 1318 |
| 10356 | `ึ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 1337 |
| 10356 | `ึ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 1367 |
| 10356 | `ึ.parts.character` | `"\u0E36"` | 1436 |
| 10356 | `ึ.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 1455 |
| 10356 | `ึ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 1497 |
| 10356 | `ื.parts.character` | `"\u25CC"` | 1581 |
| 10356 | `ื.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 1600 |
| 10356 | `ื.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 1630 |
| 10356 | `ื.parts.character` | `"\u0E37"` | 1699 |
| 10356 | `ื.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 1718 |
| 10356 | `ื.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 1760 |
| 10356 | `็.parts.character` | `"\u25CC"` | 1844 |
| 10356 | `็.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 1863 |
| 10356 | `็.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 1893 |
| 10356 | `็.parts.character` | `"\u0E47"` | 1962 |
| 10356 | `็.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 1981 |
| 10356 | `็.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 2023 |
| 10356 | `่.parts.character` | `"\u25CC"` | 2107 |
| 10356 | `่.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 2126 |
| 10356 | `่.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 2156 |
| 10356 | `่.parts.character` | `"\u0E48"` | 2225 |
| 10356 | `่.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 2244 |
| 10356 | `่.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 2286 |
| 10356 | `้.parts.character` | `"\u25CC"` | 2370 |
| 10356 | `้.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 2389 |
| 10356 | `้.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 2419 |
| 10356 | `้.parts.character` | `"\u0E49"` | 2488 |
| 10356 | `้.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 2507 |
| 10356 | `้.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 2549 |
| 10356 | `๊.parts.character` | `"\u25CC"` | 2633 |
| 10356 | `๊.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 2652 |
| 10356 | `๊.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 2682 |
| 10356 | `๊.parts.character` | `"\u0E4A"` | 2751 |
| 10356 | `๊.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 2770 |
| 10356 | `๊.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 2812 |
| 10356 | `๋.parts.character` | `"\u25CC"` | 2896 |
| 10356 | `๋.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 2915 |
| 10356 | `๋.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 2945 |
| 10356 | `๋.parts.character` | `"\u0E4B"` | 3014 |
| 10356 | `๋.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 3033 |
| 10356 | `๋.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 3075 |
| 10356 | `ุ.parts.character` | `"\u25CC"` | 3159 |
| 10356 | `ุ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 3178 |
| 10356 | `ุ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 3208 |
| 10356 | `ุ.parts.character` | `"\u0E38"` | 3277 |
| 10356 | `ุ.parts.offset` | `new r.V((0,n.yA)(6),(0,n.yA)(2))` | 3296 |
| 10356 | `ุ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 3336 |
| 10356 | `ู.parts.character` | `"\u25CC"` | 3420 |
| 10356 | `ู.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 3439 |
| 10356 | `ู.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 3469 |
| 10356 | `ู.parts.character` | `"\u0E39"` | 3538 |
| 10356 | `ู.parts.offset` | `new r.V((0,n.yA)(6),(0,n.yA)(2))` | 3557 |
| 10356 | `ู.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 3597 |
| 10356 | `์.parts.character` | `"\u25CC"` | 3681 |
| 10356 | `์.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 3700 |
| 10356 | `์.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 3730 |
| 10356 | `์.parts.character` | `"\u0E4C"` | 3799 |
| 10356 | `์.parts.offset` | `new r.V((0,n.yA)(10),-(0,n.yA)(2))` | 3818 |
| 10356 | `์.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 3860 |
| 10356 | `ฺ.parts.character` | `"\u25CC"` | 3944 |
| 10356 | `ฺ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 3963 |
| 10356 | `ฺ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 3993 |
| 10356 | `ฺ.parts.character` | `"\u0E3A"` | 4062 |
| 10356 | `ฺ.parts.offset` | `new r.V((0,n.yA)(8),0)` | 4081 |
| 10356 | `ฺ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 4111 |
| 10356 | `ํ.parts.character` | `"\u25CC"` | 4195 |
| 10356 | `ํ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 4214 |
| 10356 | `ํ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 4244 |
| 10356 | `ํ.parts.character` | `"\u0E4D"` | 4313 |
| 10356 | `ํ.parts.offset` | `new r.V((0,n.yA)(12),(0,n.yA)(2))` | 4332 |
| 10356 | `ํ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 4373 |
| 10356 | `ำ.parts.character` | `"\u25CC"` | 4459 |
| 10356 | `ำ.parts.offset` | `new r.V(-(0,n.yA)(11),(0,n.yA)(2))` | 4478 |
| 10356 | `ำ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 4520 |
| 10356 | `ำ.parts.character` | `"\u0E33"` | 4589 |
| 10356 | `ำ.parts.offset` | `new r.V(-(0,n.yA)(5),-(0,n.yA)(2))` | 4608 |
| 10356 | `ำ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 4650 |
| 10356 | `ั.parts.character` | `"\u25CC"` | 4734 |
| 10356 | `ั.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 4753 |
| 10356 | `ั.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 4783 |
| 10356 | `ั.parts.character` | `"\u0E31"` | 4852 |
| 10356 | `ั.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 4871 |
| 10356 | `ั.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 4914 |
| 10356 | `ิ.parts.character` | `"\u25CC"` | 4998 |
| 10356 | `ิ.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 5017 |
| 10356 | `ิ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 5047 |
| 10356 | `ิ.parts.character` | `"\u0E34"` | 5116 |
| 10356 | `ิ.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 5135 |
| 10356 | `ิ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 5178 |
| 10356 | `ี.parts.character` | `"\u25CC"` | 5262 |
| 10356 | `ี.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 5281 |
| 10356 | `ี.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 5311 |
| 10356 | `ี.parts.character` | `"\u0E35"` | 5380 |
| 10356 | `ี.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 5399 |
| 10356 | `ี.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 5442 |
| 10356 | `ึ.parts.character` | `"\u25CC"` | 5526 |
| 10356 | `ึ.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 5545 |
| 10356 | `ึ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 5575 |
| 10356 | `ึ.parts.character` | `"\u0E36"` | 5644 |
| 10356 | `ึ.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 5663 |
| 10356 | `ึ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 5706 |
| 10356 | `ื.parts.character` | `"\u25CC"` | 5790 |
| 10356 | `ื.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 5809 |
| 10356 | `ื.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 5839 |
| 10356 | `ื.parts.character` | `"\u0E37"` | 5908 |
| 10356 | `ื.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 5927 |
| 10356 | `ื.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 5970 |
| 10356 | `็.parts.character` | `"\u25CC"` | 6054 |
| 10356 | `็.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 6073 |
| 10356 | `็.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 6103 |
| 10356 | `็.parts.character` | `"\u0E47"` | 6172 |
| 10356 | `็.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(4))` | 6191 |
| 10356 | `็.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 6234 |
| 10356 | `่.parts.character` | `"\u25CC"` | 6318 |
| 10356 | `่.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 6337 |
| 10356 | `่.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 6367 |
| 10356 | `่.parts.character` | `"\u0E48"` | 6436 |
| 10356 | `่.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 6455 |
| 10356 | `่.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 6498 |
| 10356 | `้.parts.character` | `"\u25CC"` | 6582 |
| 10356 | `้.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 6601 |
| 10356 | `้.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 6631 |
| 10356 | `้.parts.character` | `"\u0E49"` | 6700 |
| 10356 | `้.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 6719 |
| 10356 | `้.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 6762 |
| 10356 | `๊.parts.character` | `"\u25CC"` | 6846 |
| 10356 | `๊.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 6865 |
| 10356 | `๊.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 6895 |
| 10356 | `๊.parts.character` | `"\u0E4A"` | 6964 |
| 10356 | `๊.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 6983 |
| 10356 | `๊.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 7026 |
| 10356 | `๋.parts.character` | `"\u25CC"` | 7110 |
| 10356 | `๋.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 7129 |
| 10356 | `๋.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 7159 |
| 10356 | `๋.parts.character` | `"\u0E4B"` | 7228 |
| 10356 | `๋.parts.offset` | `new r.V(-(0,n.yA)(11),-(0,n.yA)(3))` | 7247 |
| 10356 | `๋.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 7290 |
| 10356 | `ุ.parts.character` | `"\u25CC"` | 7374 |
| 10356 | `ุ.parts.offset` | `new r.V(0,-(0,n.yA)(4))` | 7393 |
| 10356 | `ุ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 7424 |
| 10356 | `ุ.parts.character` | `"\u0E38"` | 7493 |
| 10356 | `ุ.parts.offset` | `new r.V(-(0,n.yA)(14),(0,n.yA)(0))` | 7512 |
| 10356 | `ุ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 7554 |
| 10356 | `ู.parts.character` | `"\u25CC"` | 7638 |
| 10356 | `ู.parts.offset` | `new r.V(0,-(0,n.yA)(4))` | 7657 |
| 10356 | `ู.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 7688 |
| 10356 | `ู.parts.character` | `"\u0E39"` | 7757 |
| 10356 | `ู.parts.offset` | `new r.V(-(0,n.yA)(14),(0,n.yA)(0))` | 7776 |
| 10356 | `ู.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 7818 |
| 10356 | `์.parts.character` | `"\u25CC"` | 7902 |
| 10356 | `์.parts.offset` | `new r.V(0,(0,n.yA)(3))` | 7921 |
| 10356 | `์.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 7951 |
| 10356 | `์.parts.character` | `"\u0E4C"` | 8020 |
| 10356 | `์.parts.offset` | `new r.V(-(0,n.yA)(10),-(0,n.yA)(3))` | 8039 |
| 10356 | `์.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 8082 |
| 10356 | `ฺ.parts.character` | `"\u25CC"` | 8166 |
| 10356 | `ฺ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 8185 |
| 10356 | `ฺ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 8215 |
| 10356 | `ฺ.parts.character` | `"\u0E3A"` | 8284 |
| 10356 | `ฺ.parts.offset` | `new r.V(-(0,n.yA)(10),(0,n.yA)(2))` | 8303 |
| 10356 | `ฺ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 8345 |
| 10356 | `ํ.parts.character` | `"\u25CC"` | 8429 |
| 10356 | `ํ.parts.offset` | `new r.V(0,(0,n.yA)(2))` | 8448 |
| 10356 | `ํ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Placeholder))` | 8478 |
| 10356 | `ํ.parts.character` | `"\u0E4D"` | 8547 |
| 10356 | `ํ.parts.offset` | `new r.V(-(0,n.yA)(10),-(0,n.yA)(2))` | 8566 |
| 10356 | `ํ.parts.textFormat` | `new o.l(Object.assign({},c.$P.StandardKey.Default))` | 8609 |

### E. `monolith-zhuyin.js`

| Factory | Source property | Value / dependency | Property byte |
| --- | --- | --- | --- |
| 89629 | `ˉ.parts.character` | `"\u02C9"` | 232 |
| 89629 | `ˉ.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 251 |
| 89629 | `ˉ.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 281 |
| 89629 | `ˊ.parts.character` | `"\u02CA"` | 363 |
| 89629 | `ˊ.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 382 |
| 89629 | `ˊ.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 412 |
| 89629 | `ˇ.parts.character` | `"\u02C7"` | 494 |
| 89629 | `ˇ.parts.offset` | `new l.V(0,(0,f.yA)(-3))` | 513 |
| 89629 | `ˇ.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 544 |
| 89629 | `ˋ.parts.character` | `"\u02CB"` | 626 |
| 89629 | `ˋ.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 645 |
| 89629 | `ˋ.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 675 |
| 89629 | `˙.parts.character` | `"\u02D9"` | 759 |
| 89629 | `˙.parts.offset` | `new l.V(0,(0,f.yA)(-5))` | 778 |
| 89629 | `˙.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 809 |
| 89629 | `，.parts.character` | `"\uFF0C"` | 893 |
| 89629 | `，.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 912 |
| 89629 | `，.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 942 |
| 89629 | `。.parts.character` | `"\u3002"` | 1026 |
| 89629 | `。.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 1045 |
| 89629 | `。.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1075 |
| 89629 | `、.parts.character` | `"\u3001"` | 1159 |
| 89629 | `、.parts.offset` | `new l.V(0,(0,f.yA)(4))` | 1178 |
| 89629 | `、.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1208 |
| 89629 | `「.parts.character` | `"\u300C"` | 1292 |
| 89629 | `「.parts.offset` | `new l.V((0,f.yA)(-14),0)` | 1311 |
| 89629 | `「.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1343 |
| 89629 | `」.parts.character` | `"\u300D"` | 1427 |
| 89629 | `」.parts.offset` | `new l.V((0,f.yA)(14),0)` | 1446 |
| 89629 | `」.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1477 |
| 89629 | `『.parts.character` | `"\u300E"` | 1561 |
| 89629 | `『.parts.offset` | `new l.V((0,f.yA)(-14),0)` | 1580 |
| 89629 | `『.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1612 |
| 89629 | `』.parts.character` | `"\u300F"` | 1696 |
| 89629 | `』.parts.offset` | `new l.V((0,f.yA)(14),0)` | 1715 |
| 89629 | `』.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1746 |
| 89629 | `《.parts.character` | `"\u300A"` | 1830 |
| 89629 | `《.parts.offset` | `new l.V((0,f.yA)(-10),0)` | 1849 |
| 89629 | `《.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 1881 |
| 89629 | `》.parts.character` | `"\u300B"` | 1965 |
| 89629 | `》.parts.offset` | `new l.V((0,f.yA)(10),0)` | 1984 |
| 89629 | `》.parts.textFormat` | `new c.l(Object.assign({},n.$P.StandardKey.Default))` | 2015 |
| 89629 | `tabs.layout` | `s.Lf` | 2182 |

## F. Evidence, extraction coverage and remaining boundaries

### F.1 Saved source identity

Original decoded source root used for this extraction: `C:\Users\Muhib\AppData\Local\Temp\bloob-jetpack-effective-20261014`. The temporary path is a local work location, not a permanent repository source archive. Bundle names plus SHA-256 and UTF-8 offsets identify the inspected bytes independently of a beautified display.

| Parsed JavaScript bundle | Bytes | SHA-256 |
| --- | --- | --- |
| `1024.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `1037.js` | 29783 | `51f561e2fc568ebcde87af2e26ce26cb77e67fe578fdd86f54cb44a0559c8816` |
| `1290.js` | 29874 | `9aa74ad6108ce5620ae56f06f0c33019b213e12e3ae8d002a5f75551c40b2b08` |
| `1884.js` | 68659 | `bbb207a545ca5d831a27665114c758acd1cebe0e45fd1fd00552672b3f08dbf2` |
| `2718.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `4239.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `5438.js` | 58705 | `5aacdbff786daea3c055036965c1dc78d84b8aff0466a591aeaffb795d1463e3` |
| `5909.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `6606.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `6759.js` | 1040690 | `743703b3d53e1b6d0356d4e32729831104808b82d575ba15e3f158576fd4728a` |
| `6937.js` | 20283 | `77834eb9a198a156f6ce8fdae1dff6d06568c213ec54bcd11ac31d55a3832fb1` |
| `774.js` | 201372 | `b1a11273285c46b0b6a22f6fe0fdb4815b5e624d61b2ee96ee174a98d9247ab1` |
| `8556.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `8861.js` | 51741 | `ae4cd466c8aca422adc72fd8ede51490664f434e19bf57adaa367ca43faba177` |
| `9021.js` | 37747 | `5ac57df9505424efa34dca1459b7832bf1ec1b6e51d932ca5c2145a0c97ce070` |
| `9090.js` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `9279.js` | 68557 | `9d0ca6a1f8c425e01f4e2ac6a5d04d9e6435aa8c7536d399bf1baa348e145ba1` |
| `advisories-tr.js` | 1446 | `df23bbf0ef90f289140eb9bd386333c9b8aa70047beec99f3d8670df814fe268` |
| `app.js` | 2799500 | `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4` |
| `bootstrap.js` | 9421 | `1b839620f19912cfca6c9ae416943ffc88279ad69ebdcfdc6b290e0a7c101b91` |
| `Dwell.js` | 41050 | `227d67800de19d0b75c0bf7599d4ba5b21f2d14c68930ead3936fe3e8c7d7abd` |
| `InAppPurchasing.js` | 20649 | `9befbbdb2981a0bdde58b475b053331c232cd8d62dd14df5ca5a09b80c648c34` |
| `InAppRating.js` | 2652 | `0a57c176189989744f51d1a197954debeca2896d54691846837f1e5943cc898f` |
| `LibraryPage.js` | 50152 | `9c5f064924fa4562cd69490b8f160319a21c041e89021c4270f948e72644fbf7` |
| `locale-ar.js` | 2645 | `d298b1e888234e49895e4daf6d2767636f28887f4faea16fd792f9db36df63a8` |
| `locale-bn.js` | 3242 | `45a6bad526ce35e7e6ada6551101165c67aef85553ad492fdfcb7299ceb99560` |
| `locale-ca.js` | 1867 | `f344cb4cd2fbedac3ce19e0bff503008a56b868d8fd27931774c970bded02c9a` |
| `locale-cs.js` | 1727 | `9c16af839d0c5f8040340d1832940c101f501d34f11af2c1994b720ffbdaa33b` |
| `locale-da.js` | 1291 | `6f2ec052e7f03f1b502b31cc6fae3f92b3689744b4e51b5cd4898f940fb12f64` |
| `locale-de.js` | 1462 | `9868fd075c8806ebf1da4ce28eb3f97e12a58b1e94915e0dffe820c40936abf9` |
| `locale-el.js` | 4463 | `c82ba93f2bd30bff9f32a2c6b0c5b623b0735eee38aac2cc70b8c760a56c4000` |
| `locale-en-au.js` | 1226 | `3065ee945af04da90e8afb11e990c68468d6e282a63166b555c2a75d6da9ba94` |
| `locale-en-ca.js` | 1147 | `3de391345044cfc0588ec317d7987ba77a8c06b9ff53b3e35e093c4278055c35` |
| `locale-en-gb.js` | 1090 | `112cc89be18fe6cbbccc7cb71753cef57056309440086c2eb714a36f5c080520` |
| `locale-en.js` | 443 | `1193180931fb80969354076323f61fbb1034569f11022df8e9e5b08faa449a1e` |
| `locale-es-mx.js` | 1526 | `d8c7aa9a7826d2c3802c4bbc01fd283b86eefa38a87478618c9f0c92018bafea` |
| `locale-es.js` | 1486 | `44c2a827bb191d76ef238769f033792cd11cc85bf8ee5b707d60bbeaa4d15db6` |
| `locale-fi.js` | 1776 | `5b2b9e8be8b836cc7db84c17c634ed0300a66727896223266968117f47d82570` |
| `locale-fr-ca.js` | 1246 | `7c7fabfb67651dc778f2fd7e368dff00acf4f894b58d10eeb8a699dedc0f1ac0` |
| `locale-fr.js` | 1081 | `60a910437d15fa0db628a3bf7330cd1872d5eac32ca7afb08be451b87c33a91e` |
| `locale-gu.js` | 2887 | `429f777817d55fd2684ceee3b19f4a9edb39487a3ed503ee02631c8677a7fdf9` |
| `locale-he.js` | 3167 | `e5befc657965e1386ced45fb681909c9f40859f0b97858ba74b0c471ed76d08b` |
| `locale-hi.js` | 2948 | `444f258fdcbc83e2b2946ab72fa4273d35152d75cd4a7026c17b629ff90fd86f` |
| `locale-hr.js` | 1301 | `6c0c4ccd895b432c5922f78a09b40a1b554945c8a7c5d0431d522959b19f77ce` |
| `locale-hu.js` | 1608 | `f3232953fb49d5f383ec9757acbc8fa7074d935caf740e92c4b0167ac2f68a72` |
| `locale-id.js` | 1082 | `ba7249beadf947abba88771bc25dacde6b72f33217f434e798dd7a5a3c1af2d1` |
| `locale-it.js` | 1036 | `b6ec5b220a14438dff2d4291310524f5d23ad035de0b29934d4600e1bff0c4ec` |
| `locale-ja.js` | 1846 | `34302c27b4a3d94487a7f01b013a0b5909518102c5cc16bfd7f9a526aaa7a14d` |
| `locale-kn.js` | 2813 | `e200e2181d36dd1c07bc006084ae75abd1d887aa56f7422d503e143b41c94538` |
| `locale-ko.js` | 1681 | `5e623825c9341529cb721ca819925c878b99e3e596bb60daf5eac364bf91924a` |
| `locale-ml.js` | 2978 | `be98807223c5247b53e4cc33cb3f7941c11e50b68c62624a4b35f41dc6221b0d` |
| `locale-mr.js` | 3911 | `454e54136b78524506d6dd25e61dcf0c72196146d98c1692df68fe27a563dbef` |
| `locale-ms.js` | 1061 | `ce5cbe388e20b2ff06511c2aed598e2bcac46cd0c10d9e0a8f046ef6008eb337` |
| `locale-nb.js` | 1075 | `01513f8c42719dd3f98609a161dca4f2a6e25c1f4afe60bc3f7a5d98be96b64a` |
| `locale-nl.js` | 1351 | `0a26c8f75c1547b7c56cd22bf85c733091d157ca72313c632566c19761488a04` |
| `locale-or.js` | 3135 | `828fab6cdedbbe0d1dbc63ada3408826854cc7f7bd5ea5436b9704c81bc4ec20` |
| `locale-pa.js` | 2655 | `a3ad28af373346357a8df3d085f14a7dd71efd23b636c53587c15efb3cb4ef4b` |
| `locale-pl.js` | 1614 | `780e97ed0eb3babfdda52b0850949ce7b4706eab51186d70e99155bd256a0bfa` |
| `locale-pt-br.js` | 1512 | `1c89f10f2fb3d77af2b25de93e9f2ad1dc6d9abaa18863621c57cf15915e4e8e` |
| `locale-pt.js` | 1725 | `d05edecc7680a1220f4eb14d01e72e6d2bcfc5a87f6dd211c65002bc0d22a4b7` |
| `locale-ro.js` | 1203 | `24665c6d31bdd70137d1e4b2ae99454544add4e6eea664a84cde69e4d899fcd6` |
| `locale-ru.js` | 4408 | `5dfde5f73918a6504f061f4eb447a65d8080bf88d348339bfd2e808318a9928b` |
| `locale-sk.js` | 1645 | `c936f69399db3151a59277257037d0c7b9d68c0bbc300d37370b8a625bf472e6` |
| `locale-sl.js` | 1821 | `7bb61e51a9c12ff6c97eae09affdbdfea5cef8fcd287c88210aefbc4d10e3fad` |
| `locale-sv.js` | 1169 | `a52af27c5ba2ad724e3dbd4496c993761041933c6cc4d1ac4655b75b038c33e4` |
| `locale-ta.js` | 3132 | `e8efc23572ea15d7712150471a6d5c19c218de280d7e7e3bdc14c6c9edaadd29` |
| `locale-te.js` | 2710 | `f72f82624e0e343a5c3b21aecdbb36e2f3b1f5c5a72ca97e3dccb54937bbc45d` |
| `locale-th.js` | 2485 | `f7148bbf556d42fdbea6288319d29bc32ff6d6491b851af7f36c3d6ae4448520` |
| `locale-tr.js` | 1591 | `0cbfa36274a0c27dc6c0ffa0f048f6b2cbe2119540d96a39b2b9ac1ef28ebb6e` |
| `locale-uk.js` | 4623 | `b585b9c53db4561625b7fba8fd701f97474c97fe41a4a361cd8b11d01a1fac92` |
| `locale-ur.js` | 2304 | `4f181507f93f5e9be7512ecf3ec565542beec3513bde6984349015813ca7e180` |
| `locale-vi.js` | 1311 | `d4376cf75b059d55b3b5a3e7caa9fa353625d6258c5004a2addc5fdd29607624` |
| `locale-zh-cn.js` | 2153 | `0b988df9b87dfc9a776ad9d556817c139ad5d66ecb2e39f9878fc8a964bd03c4` |
| `locale-zh-hk.js` | 2110 | `6932a76a7e3cc03024ce949c0cf4750cf11b448a7ef1eed633dc73580cf247e7` |
| `locale-zh-tw.js` | 2090 | `24318096ccf7fa3c2213135fb1c10d6f0e65c37d513bbb4185686abccdcf8d98` |
| `main.js` | 23464 | `4fac46cc5db14a36d6c35907f4f561f39d9ce15113295f60c0179ffccd9b3016` |
| `ManageSportsPage.js` | 14294 | `c817883164f774aa6ba5d6d673adccb3c27d5747b299b28046d8811a2933ca80` |
| `monolith-arabic.js` | 1008 | `9108b3a91d9cadfae046b17217125f27363b4f1fffb4566057aa8cbecf8fbd31` |
| `monolith-bengali.js` | 1456 | `89012e2333fafb2fe16ff83d4718fce8d2b4da5de8eaaa3a8b1dcc06ad9a7def` |
| `monolith-catalan.js` | 937 | `b7d93ff4f777b16c603029572d817da3bc49985717fb392f5b0bee9fd34b5598` |
| `monolith-danish-norwegian.js` | 966 | `09dee28de23bfdd3a1da90d9244796d3e289096a1d3188438c78e17614d19cce` |
| `monolith-default-email.js` | 1113 | `2bfebf31cee2e686c50dd9f7f0ba960acbf013fc6f0595d50950d930748eed6d` |
| `monolith-default.js` | 924 | `23271e9638d51a88de16839d5d28c185c21dde97b9dec69fb464846fa6196d72` |
| `monolith-german.js` | 966 | `d13cbe4d69d93360d671a26cc8601d002175143cad1e44dd3483cb4aca07f40f` |
| `monolith-greek.js` | 1203 | `56a9a3637bf57cb5c0c5cd23d5f3fbd40364efeefb750abbb62cb0049aea3fa6` |
| `monolith-gujarati.js` | 1422 | `be4225077cf4081df16b6be3d303a768355b576b66b5d1e14bb0973ad93a0e7d` |
| `monolith-hebrew.js` | 891 | `927876447b73297adb7a475b3312c475ae7580eab342ad54fd8ee6048c212cdf` |
| `monolith-hindi.js` | 1364 | `d0346bbfdcc45836a290394e48728de4076854c48ff4e90ca9ceac558ac3737c` |
| `monolith-kana.js` | 2331 | `90e02e37e92eb5e596658049144214183457a83c8c3ba4d7b36f6bca39f19aaf` |
| `monolith-kannada.js` | 1343 | `fe1cda2f4b8a25b62be6cdd982d15a1c63b8065a857cac46bdca09a1ee543b7a` |
| `monolith-korean.js` | 938 | `a8b4c132b16e9c88beb409e11340d90564fab5941e22e8a1c8c45131a4ff04fb` |
| `monolith-linear-numeric.js` | 312 | `6340f734504ca72f4ef72feb7e690e7687bb2c9cfd554d79b9b1dbb2fa48ff8b` |
| `monolith-malayalam.js` | 1480 | `6e330556059fcbf1689067db658d233178486f2b4e3d11464dda8ec1ca986d20` |
| `monolith-marathi.js` | 1366 | `5a662560f6de6871e70b4921e61d891ff0dcdcd79b59400de3ad7b94951a0b69` |
| `monolith-original-kana.js` | 1738 | `573c35980681ef88c370d5e6bc2a100523084a6f101edf044eb45d7c3d9a315a` |
| `monolith-oriya.js` | 1429 | `521b1f15143bb12cec74fd0d525a5849ff047d8d439a2f3c36361f81098ee86e` |
| `monolith-pinyin.js` | 1910 | `b5b8f56e5d7b21c2344d836c5a7f5f625ba37c78ccf1abf869a6371894c35c64` |
| `monolith-punjabi.js` | 1177 | `5e246d88fb64851fd0bced2003270f68e92eef55e4b636d626857bf98ec2ea20` |
| `monolith-romaji.js` | 1713 | `63ee8e2effd8f7a20d3ef06b45d58b589ef36cd27159a655d1be8fc1481413b4` |
| `monolith-russian.js` | 1314 | `b84af68a3fd93ed7e338b5cd9b0110b263c472d7bbd05c0c5e2cc6c337064392` |
| `monolith-spanish.js` | 937 | `8317bf96134620e7d53ed6a3487e45990cb96603a47f835a5736ce7e9a240746` |
| `monolith-stroke.js` | 1927 | `3f49347036eca010330a525767095c1f61eeceab0d2c369c57fa50fb39b0aab3` |
| `monolith-sucheng.js` | 2129 | `0c282a0e47ca5f407ee87af9a4e29067dcae32b59a11398dbd3099c4caebdf95` |
| `monolith-swedish-finnish.js` | 966 | `b1ba6b47a18bb1c47f5fe023868a9f714b517055d369c779069c06a0d000c981` |
| `monolith-tamil.js` | 1341 | `009eb0487289a421c342993dd65f39f946dd9602f43a30257de5c8629d565ca4` |
| `monolith-telugu.js` | 1343 | `b07a014b281114acc4e4ddcd938882fef7431c61a1123663373a78c67488fc52` |
| `monolith-thai.js` | 9923 | `c39cb019813e1b2fb9d3b98b9b116d32c40ddc59b64bd30c9f7750b51e587988` |
| `monolith-turkish.js` | 1024 | `69d03ca680da3ed56758e423c8992e9e33fc9d6bd4841de0437804a24234a6a2` |
| `monolith-ukrainian.js` | 1314 | `fd0ae3dbce3e67b72df8f629dc089a59bb1a4cf9198f4828243e08bb140216ef` |
| `monolith-urdu.js` | 1207 | `7a4cd29d98c08e41a85465cee370bd3979d22bff2c455fe1df91355a678add07` |
| `monolith-zhuyin.js` | 3148 | `c28fa44b80ce24f062f28a33c59cec27dbc0d876857b723effaad8cd62d45ddc` |
| `MovieBundlePage.js` | 8437 | `e7bb9e35954ad2477b3555ab1b57c0cbb2d0ad13776b9878e639134856cf5c51` |
| `MultiViewPage.js` | 51619 | `81cfbc138e399866a7feddda57c7d49938611782ec89a68edec48cf93ea5894d` |
| `PlayerPage.js` | 354712 | `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955` |
| `PlayerStore.js` | 133390 | `b957612a3e17dfa5645cc95f46c4f9eb57db79130a40f7feebaaf42353944c06` |
| `ProductPage.js` | 114152 | `3cf224603413db5d9be5324efa7f9c11102086e800f6f8972155747e0f6d1870` |
| `ProfileSwitcherWidget.js` | 10649 | `12695b8514207dd0eed3ea91ba5687588054cddbf20ff1cfca0a198fd03ee438` |
| `qos.js` | 128107 | `d3278ddd89057982d1b7e2bd59c44dad79d77df6fbdcee72818de77909bc2455` |
| `QRDialogPage.js` | 11702 | `47dcd556b0a5875a9f4e72440d1613f7a25c350c8fde3d82b09b446b5b8b8eaa` |
| `QualityOfService.js` | 10528 | `6a63b8155fd3dd4915a365fdaff23457091d1ea3d8e200870ccbcb7b47b71b3b` |
| `ratings-br.js` | 12927 | `6240af3cea5476e452b07db60b4be091f93902117ed23b5378897cf48a7ac0ef` |
| `ratings-it.js` | 11260 | `05b7a1c01c44890b0d369ba09eb2e7b035f3f676bc0a6652a1d11e4800f33bc7` |
| `ratings-kr.js` | 7910 | `8cf11ba8bdfc5f5d6c34ecce7c94dc1be8395aaa9eb6198f19ea79af08598a11` |
| `ratings-tr.js` | 5450 | `75e3a5091b40227fe233ec6975c5f79f23801de7763e616eb9f5dd9050301246` |
| `ratings-za.js` | 64532 | `fb485fe46de305f8d4aa5999a98955d64d9e02ef2788b9d6e08c8fc6820262a6` |
| `SearchPageFactory.js` | 43773 | `6248f84fa94bbb3e4ec50d047860485c82d9eee9cb460eabaac7369ae7264f04` |
| `SelectAccountPage.js` | 4392 | `915684cd3eb25aa4c1d426470c5d67bb94e0139c88ca30af9137450d46e07a34` |
| `Settings.js` | 99525 | `49d86950b2d0fc71d1d3d36ae7c94aa36108510810c692bf4e7e65aff94621cd` |
| `SettingsPage.js` | 24564 | `f93662bc564dcbcb18fcceeb86ce686b8195d27ad3444ff2ca2ca6463659de9c` |
| `SportsPostPlayPage.js` | 20174 | `5971143942077f68d15e47d429598a5c77c2fb0f32e644886d7053e1f13b1062` |
| `Subscriptions.js` | 17582 | `725e532eae47316126e0982d426a0412f07ca1ce4a68cbe714ae0da4aab40566` |
| `SubscriptionsV3.js` | 1283 | `cbbac001e6954639a0e4fa76ad09a27896f796557a118d30e46637ff1211cb42` |
| `SuperBrowsePage.js` | 148609 | `77c182bc42387e5c4c4344e9253f60f4f8ff51ce8ca1586f34fb5ba64508cc25` |
| `TextInput.js` | 124387 | `94f971f710372e3a28369333cba3cf69eab2b1ea5c62c80b2b2d6a22a317d8bf` |
| `Trampoline.js` | 24652 | `8e8abfb6667233cc90316ee080a58800aa283deae44c45aafa2f33dec208988c` |
| `troubleshooting-xhr.js` | 5327 | `04d93dab2c01b0c803acdb44ae819f74d2f964ecd8e9e6177f322cab7122e0ed` |
| `WelcomeScreenFlow.js` | 8531 | `ba43e390edaf7d4645b476e19c6a9e0f4ee15f2e843bc946314acdbb4defc5c2` |
| `WelcomeScreenGDPRPage.js` | 5697 | `ee0ddf30d96ba577da001c70f516991ddc03583748b2d008c94dce7ae93bdd3b` |
| `WelcomeScreenPage.js` | 14572 | `5ed846e64b160b08b77f884d5a36d72a52b22ae3c2d6e91359c06acdfb6b1774` |

### F.2 Method and validation

- Read the decoded bundle syntax; located webpack factories, shared colour/typography tables, style suppliers, named page objects, inline presentation calls and caption rendering rules. No reference bundle bootstrap, module factory or native API was run.
- Used a Babel AST discovery pass across 139 files. The 16196 heuristic hits include false positives such as XML entities, telemetry, schemas and layout intermediates. Only scoped style declarations and inspected presentation rules were promoted into this document.
- The 975 named page/component appendix expressions were compared byte-for-byte with the original source: zero mismatches. Additional shared declaration rows were derived from AST ranges in the hashed app.js; short source expressions are retained, with verified common-import aliases expanded for readability.
- 395 language-keyboard property rows supplement the page and shared-style inventories. Locale translation files were parsed; the discovery pass found no style-property candidates in those locale files.
- Numbered chunk 9279 was classified from its actual DOM/XML parser, entity maps, MIME/namespace tables and error-code implementation. Its 2,128 heuristic hits are not application design tokens. Media/telemetry-only numbered objects were similarly excluded; the caption renderer in chunk 6759 is explicitly included in section B.

### F.3 Fresh REA check

REA analyzed an inert 45,775-byte excerpt containing exactly factories 46491,29942,38413,44548,3038,301,41457,31398,31210 from app.js, with a parent-source provenance manifest. This was a focused syntax/relationship check, not a complete-app token extractor or runtime observation. It parsed one JavaScript file with no parse failures; the retained semantic graph remains unknown/partial, with 2,303 unknown relationships reported.

| Evidence field | Identifier / result |
| --- | --- |
| REA parent analysis | `ev_e64dd90ed1808e5a74ceff75520a6df0e10a58f70a1e6963ff3222a313733c1a` |
| REA summary view | `ev_2c2ce61957e2ff55823b9da3ee110f34b24f626897b5b763b88782ca578105b6` |
| Excerpt directory SHA-256 | `3db5a2151fcfabcec2e653fee489767bc42cfcb0f9769512d5d6f07eb79fbc5c` |
| Parent app.js SHA-256 | `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4` |
| Application coverage label | Complete only for the supplied excerpt container; not the full saved app |
| Semantic coverage | Unknown/partial; static candidates do not establish execution |

### F.4 Exclusions and unresolved values

- Three hls-* media engine bundles and worker.js were excluded from design-token parsing. Protocol, buffering and codec controls are outside this design request. Other JavaScript bundles, including locale, language keyboard and advisory files, were included in the syntax scan.
- Native rasterisation, material/blur implementation details, compositor/display colour processing, actual font availability, runtime viewport, active user caption choice, accessibility settings and remote feature flags were not measured.
- Artwork-derived colours, title imagery, logo sizes bounded by content, service-defined rich text, conditional layout and runtime spring state have no single universal fixed output. Their declaration/dependency is retained when recoverable.
- The named-object appendices do not exhaust every arbitrary function-local drawing calculation in the application. Curated sections add major inline rules, but no claim is made that every visual constant in every native/runtime path is catalogued. Long computed values retain exact locations rather than an invented resolution.
- A declared token table is evidence of available code/data. It does not prove that all branches, layouts or platform variants shipped visibly or were reachable under the current account/catalogue.

### F.5 Applying the catalogue to Bloob

Treat this as a reference for the shape of a token system: foundations → semantic roles → component defaults → state/variant overrides → runtime inputs. Keep the same distinctions when choosing Bloob values. The reference does not establish one magic focus duration, one glass material or a universal minimum type size. In particular, the named dynamic typography tiers coexist with build-specific helpers that return scale 1; the tables alone cannot establish an active dynamic-type feature.
