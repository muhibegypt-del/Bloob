# Elements of A — Shared Design Token Findings

Target: saved build **16.4.4.J34L139**, inspected 15 October 2026. This is a static token inventory from shipped JavaScript, not an official design specification. No application or native API was executed. Dimensions are unscaled reference units unless noted; durations are milliseconds. Source coordinates are zero-based UTF-8 bytes in `app.js`, SHA-256 `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4`.

## 1. Raw colors and materials

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

## 2. Semantic palette: light, dark and contrast roles

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

## 3. Typography: every named table entry

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

## 4. Dynamic text and tracking tiers

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

## 5. Font resolution and accessibility transformations

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

## 6. Canvas, safe area and grid primitives

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

## 7. Shared focus, pressed and drawing defaults

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

## 8. Standard button token matrix

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

## 9. List/settings button tokens

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

## 10. Animation primitives and default parameters

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

## 11. Coverage and unresolved values

- Complete declaration coverage within factory 44548 static Color assignments and light/dark semantic maps, factory 3038 named typography tables, factory 38413 dynamic tiers, factory 301 grid/radius presets, factory 31210 selectable defaults, and constructor-level button branches in 41457/31398.
- This is not the complete app inventory: page-specific overrides, shared card/shelf variants elsewhere in app.js, icon paths and runtime/server-provided artwork palettes need their separate inventories.
- Native text shaping, actual loaded fallback font, display viewport, feature flags and active accessibility settings were not executed or measured.
- Generic easing names are resolved where formulas are directly implemented; component-supplied spring/Bezier curves must retain their own values.
- No global spacing grid, universal corner radius, fixed card scale percentage or frame-rate guarantee is inferred from these tokens.

## 12. Factory evidence coordinates

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

## 13. Floating menus: complete shared presentation defaults

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

## 14. Post-play/promotional shared composition

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

## 15. Advisory/ratings overlay tokens

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

## 16. Caption rendering and user-style tokens

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

### Caption font resolution

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

### Caption preset records

Source: factory **61202**, records at bytes **913625**, **914625**, **915582**, **916533**, **917502**. All five records set white opaque text, black background/highlight colors, userCreated false, and video-override flags true except background-opacity override false. These are preset declarations; the active user-selected preset is runtime state.

| Preset record | Main font | Size | Background alpha | Highlight alpha | Edge | Radius | ID |
| --- | --- | --- | --- | --- | --- | --- | --- |
| TransparentBackground | SanFranciscoMedium | Medium | 0.5 | 0 | None | scaled 12 | 1 |
| LargeText | Helvetica | Large | 0.5 | 0 | None | scaled 12 | 2 |
| Classic | Courier | Medium | 0 | 1 | None | 0 | 3 |
| OutlineText large record | SanFranciscoMedium | Large | 0 | 0 | Uniform | 0 | 4 |
| OutlineText medium record | SanFranciscoMedium | Medium | 0 | 0 | Uniform | 0 | 5 |

All preset records declare monospaced-serif/serif Courier, monospaced-sans Menlo, casual Avenir, script SanFranciscoMedium and small-caps Copperplate. The sans-serif slot is SanFranciscoMedium except LargeText uses Helvetica. Preset titles alone do not describe every literal field: TransparentBackground still declares 0.5 background alpha while permitting video to override background opacity.

## 17. Additional primitive evidence

- Factory **21456**, `Animation.SpringAnimationIntegrationInterval`: **18 ms**. This is integration granularity, not an observed display frame rate.
- Factory **29942**, bytes **1936065–1936394**: text/glyph scale helpers are constant 1.
- Factory **84184**, bytes **2719133–2720930**, `CrossfadeTransition`: default duration **400 ms**, separation **0**, curve **easeOut**, fadeBottom false; default top/bottom fractional positions [0,0]. Animation capability may force duration to zero unless explicitly requested.

## 18. Shared hero, canonical-lockup, footer and shelf tokens

The following tables exhaust the named data declarations in the selected style objects. A dependency expression is retained when screen state, content type, accessibility, another token or an imported factory determines its value. `scale(x)` denotes the shared dimension helper. Typography/color import prefixes are local aliases, so follow factory imports when resolving them. No widget constructors or application factories were executed.

### Factory 394: bytes 1597470–1598959

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

### Factory 58119: bytes 1529088–1537517

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

### Factory 65581: bytes 745323–747720

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

### Factory 81935: bytes 744467–745322

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

### Shelf headers and spacing

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
