# Elements of A — component and page token findings

Build: `16.4.4.J34L139`. Extraction date: 15 October 2026. Static inspection only; no TV/app execution.

This companion records actual assigned page/component style data from the saved effective JavaScript. It supplements the principal token catalogue. It is not an official token specification and does not establish which remote feature variants were live.

## Reading and coverage rules

- Source offsets are zero-based UTF-8 byte offsets in the original named bundle; factory numbers identify webpack factories.
- `scaled(n)` below is a presentation shorthand for a source call of the form `(0, alias.yA)(n)`; the original alias imports sizing factory `46491`. It is not an assertion of physical pixels.
- Constructor arguments, theme roles, arithmetic, inherited objects and conditional callbacks are retained as expressions when not resolved here. A timing constant is not observed latency.
- Each appendix table enumerates the leaves of one explicit top-level component/style object in its factory. `(spread)` means inherited fields, not replacement of the inherited object. Undefined values are deliberate source values.
- The tables omit SVG path geometry and long executable function bodies. Inline drawing/animation calls, local function-scoped objects, scalar variables and runtime-derived styles are outside this declaration appendix; important extra inline values are listed separately. The principal whole-bundle inventory covers a broader candidate set.
- There is no inferred universal spacing/radius scale. A component value remains scoped to its owner.

## High-value component groups

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

## Additional scalar, inline and conditional evidence

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

## Declaration appendix

The appendix contains **67 declaration groups / 975 leaf rows**. Some rows are inherited references or conditional expressions rather than independent literal tokens.

### LibraryPage.js — LibraryPage

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

### LibraryPage.js — LibraryShowPage

Factory `90601`; source declaration `eN`; object begins at byte **37133**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eN.titleFormat.typography` | `em.o.Emphasized.Title1` | 37167 |
| `eN.titleFormat.color` | `x.Q1.LabelSecondary` | 37196 |
| `eN.titleFormat.textAlignment` | `.5` | 37230 |
| `eN.titleFormat.pageAlignment` | `.55` | 37247 |
| `eN.verticalSpacing` | `scaled(68)` | 37269 |

### ManageSportsPage.js — component style `Z`

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

### MovieBundlePage.js — component style `b`

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

### MultiViewPage.js — component style `J`

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

### MultiViewPage.js — component style `j`

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

### MultiViewPage.js — component style `Y`

Factory `66912`; source declaration `Y`; object begins at byte **4310**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `Y.(spread)` | `H.jM.Regular.Platterless()` | 4314 |
| `Y.enabled.(spread)` | `H.jM.Regular.Platterless().enabled` | 4353 |
| `Y.enabled.Idle.(spread)` | `H.jM.Regular.Platterless().enabled.Idle` | 4397 |
| `Y.enabled.Idle.title.format.typography` | `z.o.Emphasized.Body` | 4471 |
| `Y.enabled.Idle.title.format.color` | `P.Q1.LabelSecondary` | 4497 |

### MultiViewPage.js — component style `ee`

Factory `66912`; source declaration `ee`; object begins at byte **4526**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ee.(spread)` | `Y` | 4530 |
| `ee.enabled.(spread)` | `Y.enabled` | 4544 |
| `ee.enabled.Idle.(spread)` | `Y.enabled.Idle` | 4563 |
| `ee.enabled.Idle.title.format.typography` | `z.o.Emphasized.Body` | 4612 |
| `ee.enabled.Idle.title.format.color` | `P.Q1.LabelPrimary` | 4638 |

### ProductPage.js — FooterAlertLayout

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

### ProductPage.js — GroupShelfWidget rail geometry

Factory `86794`; source declaration `eF`; object begins at byte **17083**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eF.verticalDistance` | `scaled(11)` | 17101 |
| `eF.spacingWithMultipleGroups` | `scaled(35)` | 17141 |
| `eF.spacingWithSingleGroup` | `scaled(18)` | 17178 |
| `eF.multipleGroupsRowHeight` | `scaled(54)` | 17216 |
| `eF.singleGroupRowHeight` | `scaled(71)` | 17251 |
| `eF.groupRowSpacing` | `scaled(20)` | 17281 |

### ProductPage.js — GroupShelfWidget season/group pill

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

### ProductPage.js — AboutAlertLayout

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

### ProductPage.js — RottenTomatoesAlertLayout

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

### ProductPage.js — CommonSenseMediaAlertLayout

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

### ProductPage.js — DescriptionLockup container

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

### ProductPage.js — DescriptionLockup content

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

### ProductPage.js — AboutCardLockup container

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

### ProductPage.js — AboutCardLockup content

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

### ProductPage.js — CommonSenseLockup body

Factory `86794`; source declaration `eZ`; object begins at byte **41350**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eZ.descriptionFormat.typography` | `et.o.Standard.Caption2` | 41390 |
| `eZ.descriptionFormat.color` | `D.Q1.LabelPrimary` | 41419 |
| `eZ.imageSize` | `21` | 41449 |

### ProductPage.js — RottenTomatoesLockup override

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

### ProductPage.js — CastLockup

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

### ProductPage.js — CanonicalLockup

Factory `86794`; source declaration `tE`; object begins at byte **61087**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `tE.anchorHeight` | `eg.nl.height-scaled(162)` | 61101 |
| `tE.buttonRowHeight` | `scaled(66)` | 61145 |
| `tE.spacings.metadata` | `scaled(18)` | 61178 |
| `tE.spacings.buttonRow` | `scaled(32)` | 61202 |
| `tE.spacings.buttons` | `scaled(24)` | 61224 |
| `tE.spacings.features` | `scaled(10)` | 61247 |

### ProductPage.js — CanonicalSportsLockup

Factory `86794`; source declaration `t_`; object begins at byte **76478**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `t_.anchorHeight` | `eg.nl.height-scaled(162)` | 76492 |
| `t_.spacings.metadata` | `scaled(18)` | 76539 |
| `t_.spacings.button` | `scaled(32)` | 76560 |

### SearchPageFactory.js — SearchHintBarWidget

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

### SearchPageFactory.js — Search input style

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

### SearchPageFactory.js — Search keyboard spring

Factory `58454`; source declaration `j`; object begins at byte **5651**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `j.type` | `y.Y5.Spring` | 5657 |
| `j.mass` | `1` | 5674 |
| `j.damping` | `22` | 5684 |
| `j.stiffness` | `150` | 5697 |

### SearchPageFactory.js — SearchPage geometry and transitions

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

### SearchPageFactory.js — AISafetyUILockup

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

### SearchPageFactory.js — SearchContentWidget segment platter and empty state

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

### Settings.js — component style `td`

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

### SettingsPage.js — component style `D`

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

### SportsPostPlayPage.js — component style `m`

Factory `71412`; source declaration `m`; object begins at byte **306**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `m.subtitlesBottomPadding` | `scaled(300)` | 330 |
| `m.gradient.height` | `scaled(556)` | 361 |
| `m.gradient.bottomMargin` | `scaled(60)` | 388 |
| `m.animation.duration` | `400` | 422 |

### SuperBrowsePage.js — SuperBrowseFilterBar

Factory `17380`; source declaration `O`; object begins at byte **760**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `O.paddingTop` | `scaled(30)` | 772 |
| `O.paddingBottom` | `scaled(26)` | 799 |
| `O.buttonSpacing` | `scaled(16)` | 826 |

### SuperBrowsePage.js — LiquidList shared menu

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

### SuperBrowsePage.js — ExpandableMenuBase

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

### SuperBrowsePage.js — LiquidMenu initial geometry

Factory `17380`; source declaration `et`; object begins at byte **11544**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `et.width` | `scaled(100)` | 11551 |
| `et.height` | `scaled(44)` | 11572 |
| `et.paddingHorizontal` | `scaled(16)` | 11603 |
| `et.paddingVertical` | `scaled(8)` | 11632 |

### SuperBrowsePage.js — ChoiceMenuItem

Factory `17380`; source declaration `ea`; object begins at byte **27162**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `ea.item.(spread)` | `J.item` | 27172 |
| `ea.item.iconWidth` | `scaled(66)` | 27189 |
| `ea.item.glyphSize` | `22` | 27212 |
| `ea.item.iconOffset` | `scaled(44)` | 27226 |

### SuperBrowsePage.js — SingleChoiceMenu

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

### SuperBrowsePage.js — MultiChoiceMenu

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

### SuperBrowsePage.js — component style `eQ`

Factory `17380`; source declaration `eQ`; object begins at byte **73670**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eQ.item.(spread)` | `J.item` | 73680 |
| `eQ.item.avatarSize` | `scaled(50)` | 73698 |
| `eQ.item.avatarX` | `scaled(52)` | 73719 |
| `eQ.item.checkmarkX` | `scaled(4)` | 73743 |
| `eQ.item.textGap` | `scaled(12)` | 73763 |

### SuperBrowsePage.js — ProfileMenu

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

### SuperBrowsePage.js — CustomizeIndicatorWidget

Factory `17380`; source declaration `eU`; object begins at byte **97444**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `eU.pillHeight` | `scaled(50)` | 97456 |
| `eU.leadingPadding` | `scaled(34)` | 97484 |
| `eU.trailingPadding` | `scaled(32)` | 97513 |
| `eU.glyphTextGap` | `scaled(11)` | 97539 |

### SuperBrowsePage.js — SuperBrowseShelf / SuperBrowseRelatedShelf animation and layout

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

### TextInput.js — component style `l`

Factory `10197`; source declaration `l`; object begins at byte **13733**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `l.type` | `s.grid` | 13739 |
| `l.RowOffsets.keys` | `scaled(76)` | 13763 |
| `l.RowOffsets.controlKeys` | `scaled(76)` | 13788 |
| `l.Origins.keys` | `[scaled(31),scaled(105),scaled(179),scaled(253),scaled(327),scaled(401)]` | 13816 |
| `l.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 13913 |

### TextInput.js — component style `d`

Factory `10197`; source declaration `d`; object begins at byte **13960**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `d.type` | `s.grid` | 13966 |
| `d.RowOffsets.keys` | `scaled(76)` | 13990 |
| `d.RowOffsets.controlKeys` | `scaled(76)` | 14015 |
| `d.Origins.keys` | `[scaled(30),scaled(92),scaled(154),scaled(216),scaled(278),scaled(340),scaled(402)]` | 14043 |
| `d.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14153 |

### TextInput.js — component style `h`

Factory `10197`; source declaration `h`; object begins at byte **14200**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `h.type` | `s.grid` | 14206 |
| `h.RowOffsets.keys` | `scaled(93)` | 14230 |
| `h.RowOffsets.controlKeys` | `scaled(84)` | 14255 |
| `h.Origins.keys` | `[scaled(31),scaled(124),scaled(216),scaled(308),scaled(401)]` | 14283 |
| `h.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14366 |

### TextInput.js — component style `u`

Factory `10197`; source declaration `u`; object begins at byte **14413**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `u.type` | `s.grid` | 14419 |
| `u.RowOffsets.keys` | `scaled(64)` | 14443 |
| `u.RowOffsets.controlKeys` | `scaled(70)` | 14468 |
| `u.Origins.keys` | `[scaled(31),scaled(124),scaled(216),scaled(308),scaled(401)]` | 14496 |
| `u.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14579 |

### TextInput.js — component style `c`

Factory `10197`; source declaration `c`; object begins at byte **14626**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `c.type` | `s.grid` | 14632 |
| `c.RowOffsets.keys` | `scaled(93)` | 14656 |
| `c.RowOffsets.controlKeys` | `scaled(93)` | 14681 |
| `c.Origins.keys` | `[scaled(71),scaled(216),scaled(361)]` | 14709 |
| `c.Origins.controlKeys` | `[scaled(68),scaled(216),scaled(364)]` | 14764 |

### TextInput.js — component style `g`

Factory `10197`; source declaration `g`; object begins at byte **14811**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `g.type` | `s.linear` | 14817 |
| `g.rowHeight` | `scaled(76)` | 14836 |
| `g.keySpacing` | `scaled(0)` | 14860 |

### TextInput.js — component style `u`

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

### TextInput.js — component style `b`

Factory `23828`; source declaration `b`; object begins at byte **36802**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `b.width` | `scaled(54)` | 36809 |
| `b.height` | `scaled(64)` | 36829 |

### TextInput.js — component style `D`

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

### TextInput.js — component style `k`

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

### TextInput.js — component style `I`

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

### TextInput.js — component style `E`

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

### TextInput.js — component style `F`

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

### TextInput.js — component style `L`

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

### TextInput.js — component style `_`

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

### TextInput.js — component style `l`

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

### TextInput.js — component style `d`

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

### TextInput.js — component style `h`

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

### TextInput.js — component style `u`

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

### TextInput.js — component style `c`

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

### TextInput.js — component style `g`

Factory `40083`; source declaration `g`; object begins at byte **119284**.

| Original property path | Exact stored value / expression | UTF-8 byte |
| --- | --- | ---: |
| `g.character.width` | `scaled(64)` | 119302 |
| `g.character.height` | `scaled(64)` | 119322 |
| `g.backgroundExpand` | `scaled(1)` | 119353 |
| `g.cursorWidth` | `scaled(76)` | 119377 |
| `g.cursorHeight` | `scaled(76)` | 119403 |

### TextInput.js — component style `y`

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

### WelcomeScreenGDPRPage.js — component style `z`

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

### WelcomeScreenPage.js — component style `N`

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

## File coverage and identities

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

## Remaining boundary

Shared app typography, colours and common component tokens are analysed separately. PlayerPage is analysed separately. Native runtime rasterisation, dynamic service-provided artwork palettes, live remote flags, GPU effects and actual screen-scale output cannot be recovered as universal fixed tokens from these component declaration tables.

## Follow-up: WebVTT caption drawing tokens in `6759.js`

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
