# Elements of A: UI and UX principles visible in shipped code

This is a study of the extracted version 16.2.4 application, aimed at informing an original Bloob design. It describes behavior and structure; it does not reproduce source, assets, typography, or screen layouts. The local `data/diff.jetpack` is mounted before UI bootstrap when the updater succeeds, so the `res/` bundle may not exactly match the UI that ran on the TV. We have not decoded the diff or captured a complete screen-by-screen visual record. Exact on-screen appearance and measured latency remain open.

REA analyzed the `res/` directory as a JavaScript application (summary Evidence `ev_5c3d28d282632943188296896a45c648a35f8f9dea083a6a5162649d819e134f`; retained parent `ev_96fb06668f86ec3f08564838cc15c82fe980f27133e8111cee19fe8988325c89`). It parsed 134 JavaScript files, but semantic coverage is partial on large bundles. The specific findings below were checked directly in the named shipped modules. Native event/render findings are from the separate focused Ghidra analysis in `Luna-runtime-architecture.md`, not from the JavaScript graph.

## The interaction spine

```text
remote input -> focused widget -> semantic action -> root page deck
                                            |          -> loading page
                                            |          -> product/search/library/player page
                                            -> shared data or playback service
```

`res/app.js` defines a root `Application` with a `mainDeck`, tabbed page, menu, notifications, and shared services. Its `push` and `remove` paths distinguish modal overlays from navigated pages and select transitions for each. `ProductPageInner` in `res/ProductPage.js` sends a `Play` intent upward rather than creating a player directly. The page stack waits for a returning product page to be ready in several pop paths. This gives one owner for navigation and a place to preserve the view beneath an overlay.

### 1. Respond now; finish the work behind a visible state

`LoadingPage` in `res/app.js` can draw the selected artwork, a solid color, or a prepared drawing context. It presents a title and a spinner whose hold time varies by path; an ordinary path uses a long hold, and a faster path uses a short one. The loading page can hide its spinner and fade its title independently. `res/main.js` draws a native splash before it attempts the JetPack update and imports the UI bootstrap. The separate UI responsiveness note records lazy page chunks and first-frame prefetch.

**Principle:** A remote press should cause an immediate, coherent visual response. Loading UI should preserve the title's context, and a spinner should communicate genuine waiting instead of flashing during a quick transition. The delays in this build are implementation choices, not proposed Bloob timing targets.

### 2. Focus carries location and intent

`GroupShelfWidget` in `res/ProductPage.js` places season groups above a horizontal row of items. Up moves from an episode to its group row; Down returns to the item row. Selecting an episode updates the selected group, and focusing a season can move the item row to that group's first item. The widget retains horizontal scroll when replacing related group content. Season tabs have separate idle, focus-candidate, focused, and disabled styles; the shown style changes contrast and width, with distinct focus-gain and focus-loss animation durations.

`res/app.js` also contains selection-mimic helpers that match a prior selected widget by ID, then fall back when content changes. In `LibraryPage`, browsing is delayed briefly when changing categories so rapid navigation does not trigger every request.

**Principle:** Focus is both a visual highlight and the user's remembered position. Keep it visible, move it predictably between rails, and restore it after data refresh or Back. A loaded page that forgets the selected item feels broken even if it renders quickly.

### 3. Artwork transitions are gated by readiness

`BackgroundWidget` in `res/ProductPage.js` prefetches an incoming image. It waits through idle/loading states and begins the transition once the image is ready or has failed; the fallback can be a solid color. It subscribes to frame animation only while layers are pending or transitioning, then stops. The drawing path uses cover-style cropping and a protection gradient. `ProductPageInner.preloadContentImages` has an explicit image-loading step before some entry transitions.

**Principle:** Hold a stable current image until the next visual state can be presented. Failure needs a designed fallback. Motion should have a start and end, so an unchanged page can rest.

### 4. A title page changes density as the user moves down

`ProductPageInner.animateFold` in `res/ProductPage.js` fades and moves the above-the-fold title information as shelves rise. It adjusts shelf transforms, logo/header opacity, and background protection in one fold value. The page's vertical navigation targets depend on the visible state. This is a coordinated change of information hierarchy, not just a scroll effect.

**Principle:** The first view of a title can give artwork and the primary action room. As the user heads toward episodes and supporting information, the hero yields space and the next task becomes dominant. Bloob should define both states and the transition between them.

### 5. Large shows remain navigable before all data arrives

`EpisodePaging` in `res/ProductPage.js` creates placeholders for the episode count, retains the selected episode index, and identifies the selected season from cumulative episode counts. It delays a request by 500 ms, fetches missing details roughly ten items around selection, and replaces distant downloaded entries with placeholders beyond a wider range of about twenty. The specific window and delay belong to this build.

**Principle:** Selection and spatial navigation should not wait for full metadata. Work near the user's focus first, bound retained detail, and keep placeholders stable so the row does not jump while data arrives.

### 6. Search adapts its workspace to the current task

`SearchPage` in `res/SearchPageFactory.js` places an input above a horizontal keyboard-and-content arrangement. Left/Right can explicitly move focus between keyboard and results. `displayKeyboard` shifts the layout when focus moves; animation can become instant on lower performance tiers. Search hints, a landing view, results, and an empty placeholder are distinct deck states. The landing view can be retained while obsolete results are disposed. Search also adjusts its menu state on scroll and has screen-reader suggestion announcements.

**Principle:** A TV search screen is a navigable workspace. Keyboard, hints, results, and empty states need explicit focus routes and Back behavior. The keyboard should recede when browsing results without destroying the user's query or landing context.

### 7. Player chrome follows the user's activity

`UnifiedPlayerWidget` in `res/PlayerPage.js` has Full, Reduced, and Deck UI modes. Its `show`, `hide`, and `restartTimer` methods use different timeout classes: ordinary/automatic display, paused or focused controls, and an open tab. Dragging, full-screen trick play, and thumbnail scrubbing suppress auto-dismiss. Scrubbing can reduce the UI; player controls may hide in a short fade. The player calculates bottom space for subtitles based on visible controls and thumbnails. Back first dismisses some banners or the player UI before leaving playback. Screen-reader use can extend control visibility.

**Principle:** Player controls should be quiet during viewing and persistent while the user is deciding or scrubbing. Subtitle placement belongs in the same state model as the HUD. Back should close the nearest interaction layer before ending the session.

### 8. Continuity can be literal, not merely animated

`ProductCard` in `res/ProductPage.js` can inherit a slideshow widget's video layer when entering a product. It tracks whether the inherited trailer was playing and switches to product artwork when that widget is disposed. This is a specialized path and does not prove that every navigation transition reuses a video layer.

**Principle:** Where a preview is already on screen, consider carrying its visual context into the detail page. If continuity is too expensive or unavailable, a prepared artwork transition can serve the same user need without a discontinuous blank frame.

### 9. Visual ambition is a capability decision

`res/app.js` defines performance-level flags for focus and page animations, product-card view, real-time blur, image placeholders, slideshow advance, player-position updates, and more. `ProductPageInner` checks real-time blur support and reduced-transparency accessibility preferences. `SearchPage` can force keyboard movement instant when auto-scroll animation is disabled; the player can force a fade instant when general animations are disabled. The code does not establish which performance level this LG TV actually used.

**Principle:** Design the readable, responsive base state first. Treat blur, continuous motion, animated page transitions, and rich cards as optional enhancements. Accessibility preferences and hardware capability should influence the presentation without changing the user's ability to complete a task.

## What Bloob should take into design work

1. Specify the **focus graph** for each screen alongside the visual mockup: D-pad destinations, selected-item restoration, Back, and modal dismissal.
2. Design four states for each content surface: **ready, loading, empty, and failed**. Keep artwork and focus stable as data changes.
3. Define a **title-page density transition** from hero to episodes; keep the primary action legible throughout.
4. Let the **source chooser** use the same principles: one obvious recommendation, nearby alternatives, progressive technical detail, and a stable selection during search/resolve.
5. Give the player an explicit **HUD state model** covering play, pause, scrub, menu, subtitle-safe area, buffering, and exit. Choose Bloob's own durations after TV review.
6. Establish a **visual quality ladder** for this LG TV: plain readable surfaces first; optional blur, video preview, and longer transitions only if they remain smooth.

## Evidence limits

- These are code-derived organization and behavior findings. They are not measured input latency, frame rate, or proof of exactly how the current TV UI looked.
- The JetPack update may replace or amend base `res/` modules before bootstrap. Its contents are not decoded. Treat precise style values in base modules as version-specific examples rather than a definitive current design spec.
- Several named styles use a screen-scaling helper, so source numbers are not reliable literal TV pixel measurements.
- The reference includes multiple feature flags and content variants; a path present in code may not have been active in the observed session.
- The original Bloob design should use its own assets, copy, tokens, components, and interaction timings.
