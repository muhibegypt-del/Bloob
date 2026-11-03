# Swiftfin tvOS: behaviour specification for Bloob

**Study date:** 15 October 2026. **Swiftfin source:** commit `791fe1249e12ffd066c763f28b4291866a14ac49`. **Supplementary Jellyfin server source:** commit `aec42b345e1a7da25ee5492183f12209338df688`.

This studies the actual `Swiftfin tvOS` source and the shared code it invokes. It covers behaviour, not visual design. Bloob is assumed to be a C++ application with its own UI and local catalogue, source resolver, progress storage and settings. Nothing here requires adopting a Jellyfin server.

**Evidence convention:** linked type/function names point to pinned source files. The source index at the end gives their complete repository paths. “Server” means behaviour provided by Jellyfin, rather than implemented by Swiftfin. **Inferred** identifies my interpretation, recommendation or proposed Bloob rule. This is a source study; it does not claim that every branch has been exercised on a running tvOS device. The supplementary server revision is a reference implementation, not proof of the server version used by any particular Swiftfin installation.

One correction to the brief: the inspected Swiftfin revision declares **MPL-2.0**, rather than GPLv3. This report describes behaviour without translating source. See [Swiftfin LICENSE.md][license].

## 1. Home

### Rows, order, contents and limits

The following is the tvOS order, before empty groups are removed. It is not the iOS Home layout. [DefaultContentGroupProvider.makeGroups / _makeGroups][home]

| Order | Group | Population and ordering | Limit and condition |
|---|---|---|---|
| 1 | Top item selector | Resume videos when that list is nonempty; otherwise recently added films/series. Resume order comes from the server. Recently added requests creation date descending. | Initial 20 from each underlying list. Both lists are loaded. The recently-added setting does not disable this fallback. |
| 2 | Next Up | Server `getNextUp` result, without a client re-sort. | Initial 20. Rewatching is off by default. Home supplies a last-activity cutoff of 366 days by default; zero disables that cutoff. |
| 3 | Recently Added | Same recent list already loaded for the top selector. | Initial 20. Setting defaults on. On tvOS this separate row requires both resume items and recent items, preventing duplication when the top selector already shows recent items. |
| 4 | Recently Played | Film/series query, played filter, date played descending. | Initial 20. Setting defaults off. |
| 5 | On Now | Server-recommended programmes restricted to currently airing programmes. | Initial 20; disappears if empty. Irrelevant to Bloob unless it supports live TV. |
| 6 onward | Latest in each eligible library | Server latest-media endpoint, one group per eligible user library, in the returned library order. | Initial 20 per group. Allowed collection types: home videos, movies, music videos and TV shows. Libraries in the user's latest-items exclusions are omitted. |

Sources for the table: [CinematicSelectionContentGroup.SelectionView.body; CinematicSelectionContentGroupViewModel.init / refresh; CinematicRecentlyAddedContentGroup._shouldBeResolved][cinematic], [PosterGroup.init / _shouldBeResolved][posterGroup], [ResumeItemsLibrary.retrievePage][resume], [RecentlyAddedLibrary.retrievePage][recent], [NextUpLibrary.Environment.default / retrievePage][nextUp], [LatestInLibrary.retrievePage][latest], [RecommendedProgramsLibrary.retrievePage][onNow], and [Defaults.Keys.Customization.Home][defaults].

These are preview groups, not evidence of endless scrolling inside every Home row. A normal row header opens the corresponding full library. Ordinary cards call their library-element navigation action: playable titles open details; folder/view items open a library. The top selector also opens details, including for resume items. It does not directly launch playback. [PosterHStackLibrarySection.body / routeToLibrary][posterSection], [BaseItemDto.libraryDidSelectElement][libraryElement], [CinematicSelectionContentGroup.SelectionView.body][cinematic]

### What the server actually supplies

* **Resume:** Jellyfin selects resumable, nonvirtual items recursively and sorts by last played date descending. Swiftfin requests video items and keeps that order. Bloob needs a distinct resumability decision plus last-activity ordering, not just every item with a nonzero clock. [ItemsController.GetResumeItems][serverItems], [ResumeItemsLibrary.retrievePage][resume]
* **Next Up:** Swiftfin requests the result rather than calculating episode progression. The inspected server finds candidate series by recent activity; selects episode candidates using watched state and episode order; then orders the resulting series candidates by watched activity. The detailed episode rules are in section 3. [NextUpLibrary.retrievePage][nextUp], [NextUpService.GetNextUpSeriesKeys / GetNextUpEpisodesBatch][serverNextUp], [TVSeriesManager.GetNextUpBatched][serverTV]
* **Latest in library:** this is not identical to the global film/series Recently Added query. The server sorts new items by creation date, with name/year tie-breaks, can group new episodes into their containing title, and applies its Hide Played in Latest preference when no played filter is supplied. Bloob would need equivalent grouping and a latest-unwatched preference if it wants this behaviour. [UserViewManager.GetLatestItems / GetItemsForLatestItems][serverViews], [UserLibraryController.GetLatestMedia][serverUserLibrary]

### Refresh and states

| Trigger | Observed behaviour |
|---|---|
| First appearance | Start initial refresh. This is the view's first appearance, not a separate promise to fetch on every process launch. |
| Return after disappearance | Refresh if the view has been away for more than 60 seconds, or a rebuild is pending. This checks time since disappearance, not a repeating 60-second poll. |
| App foreground | Refresh pending changes. |
| Progress/library changes | Shared-store events refresh relevant libraries; group rebuild requests are coalesced with a 350 ms delay. Store-driven paging refreshes use a 350 ms minimum delay and a five-second spacing rule. |
| Manual refresh | Refresh explicitly. |
| Select the current root tab again | Scroll to the top. |

Sources: [ContentGroupView.body][contentView], [ContentGroupViewModel.refreshIfNeeded / refreshIfPendingChanges / refresh][contentVM], [PagingLibraryViewModel.scheduleRefreshForStoreChange / _refresh / loadPage][pagingVM], [UserSession.observeItemChanges][itemChanges]. Returning from playback therefore has both view-lifecycle and progress-event routes; there is no single unconditional “player closed, reload everything” handler in these Home files.

Initial loading shows a progress state. A root/provider failure has an error state. Empty successful groups are removed; a wholly empty content view shows No Results. A child row that fails without usable contents can disappear through this empty-group filtering. Background refresh can preserve previously loaded content. This is not a guaranteed visible error card for every failed Home row. [ContentGroupView.body][contentView], [ContentGroupViewModel._refresh / fullRefresh / resolveGroups][contentVM], [PosterGroup._shouldBeResolved][posterGroup]

**Inferred Bloob adaptation:** keep a stable, server-ordered Home snapshot; update shared progress immediately on player exit; refresh only affected rows. Avoid putting a resolver/source search on the Home rendering path.

## 2. Title page: films and series

### Information and actions

The page adds a header to its content groups. Its metadata includes title, overview, available genre/year/runtime/episode locator information, and configured attributes. Quality/audio/subtitle attributes use the selected media source, not an assumed capability of the title as a whole. Available attributes include ratings, HD/4K, DV/HDR, 5.1/7.1 and subtitle availability; missing values are omitted. [ItemView.contentGroups][itemView], [ItemView.Description.body][description], [ItemView.MetadataHStack.body][metadata], [ItemView.AttributesHStack.body / VideoQuality / AudioChannels / Subtitles][attributes]

Below the header, the provider assembles: episodes for a series/season; available genres; studios; additional movie parts when part count exceeds one, or seasons for a series; parent season for an episode; cast/crew; special features; similar titles if recommendations are enabled; About. Empty poster groups are filtered by the group system. Genre/studio selections open filtered content collections. [ItemContentGroupProvider._makeGroups][itemProvider], [PosterGroup._shouldBeResolved][posterGroup]

The intended primary focus is Play. If a play action is unavailable, the action bar resolves another available action/menu target. Default configured actions include played/unplayed, favourite, recording, trailers and playback settings, filtered for the item's capabilities and user permissions. [ItemView.init][itemView], [ItemView.ActionBar.body][actionBar], [ItemActionButtons.Configuration.resolvedButtons][itemActions], [ItemActionButton.defaultBarActionButtons / defaultMenuActionButtons][itemActionKinds]

### Which item the primary button plays

| Page | Playback target decision |
|---|---|
| Film or another individually playable item | The item itself. |
| Series | First returned nonmissing Next Up item; otherwise first resume result under the series; otherwise first available nonmissing episode. |
| Season | First resume result under the season; otherwise first available nonmissing episode. |
| No resolved playback provider | Play button disabled. |

[ItemContentGroupProvider.resolveMediaPlayerItemProvider / nextUpItem / resumeItem / firstAvailableItem][itemProvider], [PlayButton.body][playButton]

Important boundaries: `nextUpItem` checks only the first result, rather than scanning past a missing first result. The fallback `firstAvailableItem` requests ascending order but specifies no sort field. It is therefore not evidence that Swiftfin itself selects the first season/episode numerically. The scoped series Next Up request also does not apply the Home row's cutoff/rewatching environment. [ItemContentGroupProvider.nextUpItem / firstAvailableItem][itemProvider], [NextUpLibrary.retrievePage][nextUp]

### What the button actually says

This revision does **not** implement a general Play / Resume / Next Episode / S2 E5 label decision on this button. It reads the resolved playback item's `playButtonLabel`:

| Condition, in priority order | Label |
|---|---|
| Not yet aired | Unaired |
| An already-ended programme | Ended |
| Missing/virtual item | Missing |
| Progress label available | Formatted progress text; for an ordinary partially watched file this is the remaining duration |
| Otherwise | Play |

The button also displays the chosen source's title when the playback item has multiple sources. Season/episode labels exist separately in metadata; they are not the button's next-episode naming rule. [PlayButton.body / mediaSource][playButton], [BaseItemDto.playButtonLabel / progressLabel / seasonEpisodeLabel][baseItem], [ItemView.MetadataHStack.body][metadata]

### Resume and watched state

Resume comes from the target item's user-data position. The VLC path subtracts the configured resume offset, clamps the result at zero and applies an absolute seek once the player has established a seekable timeline. The offset defaults to zero; settings allow 0–30 seconds. Play From Beginning changes the playback copy's position to zero, rather than immediately erasing shared watched data. [BaseItemDto.startSeconds][baseItem], [VLCMediaPlayerProxy.play / applyPendingStartTimeIfPossible][vlc], [VideoPlayerSettingsView.resumeSettings][playerSettings], [PlayButton.play][playButton]

Played/unplayed and favourite changes go through the shared user session. The item store makes optimistic changes, serializes mutations for an item, merges the successful server reply and removes the temporary optimistic overlay on success or failure. **Inferred benefit:** cards and details remain consistent, and a failed action does not permanently leave a false checkmark. Parent/child watched aggregation is a server responsibility. [ItemContentGroupProvider.toggleIsPlayed / toggleIsFavorite][itemProvider], [UserSession.setPlayed / setFavorite][sessionItems], [ItemStore.mutateUserData][itemStore]

**Inferred Bloob adaptation:** return one explicit `play target` object from the local server, with episode ID, position and reason. Bloob can label it more clearly as Resume or Play S2 E5 without copying Swiftfin's remaining-time-only button label.

## 3. Seasons and episodes

### Entry selection and focus

The selected season is the main play target's season when present in the season list, otherwise the first returned season. An existing valid season selection is retained while the view remains alive; an invalid selection falls back. Inside that season, the preferred episode is the main play target when present, otherwise the first episode. That episode is also the initial scroll target. This is not a disk-persisted “last browsed episode in every season” map. [SeriesEpisodeContentGroup.Body.preferredSeasonSelection / selectPreferredSeasonIfNeeded][seriesGroup], [SeriesEpisodeContentGroup.SeasonEpisodesView.preferredElementID / body][episodeCollection]

On tvOS, moving focus to a season selects it after 350 ms if focus still rests there. Explicit selection changes it immediately. **Inferred benefit:** passing across several seasons does not load each intermediate season. Seasons use a page size of 100; the episode endpoint returns the selected season's list without a client paging limit. [SeriesEpisodeContentGroup.SeasonSelector.tvOSView / selectSeasonAfterFocusDebounce][seasonSelector], [SeriesEpisodeContentGroup.init][seriesGroup], [SeasonViewModelLibrary.retrievePage][seasonLibrary], [EpisodeLibrary.retrievePage][episodeLibrary]

Selecting an episode's image starts that episode with an episode queue. Selecting its textual metadata opens details. Cards expose title, episode locator, overview, progress/played indicators and an air-date label for unaired content. Missing titles/overview have fallback text. [SeriesEpisodeContentGroup.EpisodeCard.body][episodeCard]

### Three different meanings of “next”

| Context | Owner and rule |
|---|---|
| Series-page Play target / Home Next Up | Server Next Up policy. |
| Player Next/Previous and natural autoplay | Adjacent episodes returned by the episode endpoint, not a Next Up query. |
| Focused episode on season entry | Client aligns to the main Play target if present in that season. |

[ItemContentGroupProvider.resolveMediaPlayerItemProvider][itemProvider], [NextUpLibrary.retrievePage][nextUp], [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue], [SeriesEpisodeContentGroup.SeasonEpisodesView.preferredElementID][episodeCollection]

### Server Next Up policy, read from the supplementary server source

1. Identify recently active series for the Home query and apply its activity cutoff. A scoped series request can bypass that Home-series discovery. [NextUpService.GetNextUpSeriesKeys][serverNextUp], [TVSeriesManager.GetNextUp][serverTV]
2. For ordinary progression, use the highest season/episode marked played, excluding season zero as the normal anchor. Choose the first available unplayed regular episode after that position, by season then episode number. With no played anchor, choose the earliest available unplayed regular episode. This can skip earlier unwatched gaps after a later episode has been marked played. [NextUpService.GetNextUpEpisodesBatch][serverNextUp]
3. With specials enabled, consider available specials carrying before/after-season placement metadata, alongside the anchor and candidate. Apply aired-episode ordering; skip already-played specials for ordinary progression. Unplaced specials are not automatically inserted by this decision. [TVSeriesManager.DetermineNextEpisode][serverTV]
4. A partially watched candidate is allowed when resumable results are enabled. The inspected endpoint defaults that flag to true; Swiftfin does not explicitly change it. When disabled, progress on any version of that candidate excludes it. [TvShowsController.GetNextUp][serverShows], [TVSeriesManager.DetermineNextEpisode][serverTV], [NextUpLibrary.retrievePage][nextUp]
5. After a season ends, the same ordered candidate search can cross into a later season. If no candidate remains, no ordinary Next Up item exists. Rewatching optionally creates a separate next-played candidate based on the most recently played episode; Swiftfin Home defaults that option off. [NextUpService.GetNextUpEpisodesBatch][serverNextUp], [TVSeriesManager.GetNextUpBatched / DetermineNextEpisodeForRewatching][serverTV], [Defaults.Keys.Customization.Home.resumeNextUp][defaults]
6. The server can prefer an alternate version of the next episode whose version name matches the actually played alternate version of the previous watched episode. This is server-side Next Up behaviour, not a Swiftfin source-selection memory mechanism. [TVSeriesManager.GetMostRecentlyPlayedVersion / GetPreferredVersion][serverTV]

**Bloob server rule:** to reproduce this, the server needs watched state, playback activity, season/episode numbers, special-placement metadata, availability and version relationships. It must produce both a recommendation and a separate adjacent-playback sequence. **Inferred:** decide deliberately whether Bloob should use this “furthest watched anchor” policy or resume the earliest unfinished gap; do not let the two policies emerge accidentally.

### Player adjacency and indicators

The queue requests at most three adjacent episodes. With two results it decides whether the first is current to identify a predecessor or successor; with three it uses the first as previous and third as next. It does not itself skip watched episodes or apply a specials policy. Server episode ordering/filtering supplies that sequence. The supplementary server assembles season episode order, inserts specials according to its configuration, removes duplicate special occurrences across the series listing and then filters for adjacency. [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue], [TvShowsController.GetEpisodes][serverShows], [Series.GetEpisodes / GetSeasonEpisodes / FilterEpisodesBySeason][serverSeries]

Next/Previous queue providers force the target playback position to zero. Manually selecting an arbitrary episode in the player's episode supplement uses that episode's ordinary stored position instead. Queue advancement therefore does not automatically resume a partially watched adjacent episode. [EpisodeMediaPlayerQueue.getAdjacentEpisodes; EpisodeOverlay.select][queue]

Progress bars use server-reported played percentage. A progress label requires a resumable position/runtime combination for files. Played checkmarks, unplayed dots and unplayed counts depend on user data, item eligibility, display configuration and context; checkmarks/unplayed dots require zero resume position, and the resume context suppresses the played check. There is no client 90% threshold inside the card. [BaseItemDto.progressLabel / progressPercentage][baseItem], [PosterIndicatorsOverlay.body][indicators]

## 4. Versions / media sources

### Default, prompt and memory

Swiftfin uses an explicitly selected source, otherwise the first source returned with the item. It does not implement a client 4K/HDR/Atmos ranking algorithm. A version picker is offered when there is more than one source, but pressing Play does not force a choice dialog. The source's name supplies its display title. [BaseItemDto.getPlaybackItemProvider][baseItem], [ItemActionButtons.Playback.versionPicker][playbackPicker], [MediaSourceInfo.displayTitle][sourceInfo], [MediaPlayerItem.build][build]

In the supplementary server, source sorting puts the queried item's own source first, then prefers video files, non-3D sources and greater video width, and removes placeholder sources. Thus some ordering comes ready-made from the server, even though Swiftfin itself just takes the first. This is not an HDR/Atmos compatibility score. [MediaSourceManager.SortMediaSources][serverSources]

The details-page source selection lives in its playback provider. Changing source clears explicit audio/subtitle indices while preserving requested bitrate. The inspected flow does not write a per-title, per-series or global source-selection preference. Rebuilding the details provider resolves the default again. [ItemContentGroupProvider.select / makeGroups][itemProvider]

The adjacent-episode queue carries requested bitrate, not the current media source or audio/subtitle override. A new episode is built using its own first source and server track defaults. Server Next Up's alternate-version-name matching, described above, is a separate path; it should not be mistaken for guaranteed version carry-over during player autoplay. [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue], [MediaPlayerItem.build][build], [TVSeriesManager.GetPreferredVersion][serverTV]

### Playback resolution

Playback preparation refreshes the full item and requests server playback information for the source, device capabilities, bitrate and explicit tracks. It then resolves the returned source by tag match, an open-token heuristic, ID match, or first-source fallback. Source runtime can replace item runtime. URL selection prefers a returned transcoding URL, otherwise a static video stream endpoint for video, otherwise the source path. Missing source, session ID or usable URL fails preparation. [MediaPlayerItem.build / streamURL][build]

**Inferred Bloob adaptation:** the local server should rank only compatible available versions, return a stable version ID, and resolve the selected version to an expiring AllDebrid playback URL plus authoritative stream metadata. Keep preferences as semantic choices—resolution ceiling, HDR policy, audio capability—rather than remembering a raw source-array position. A per-series version preference is a Bloob improvement, not a feature demonstrated in Swiftfin's client.

## 5. Audio and subtitles

### Client precedence

| Track | Initial selection |
|---|---|
| Audio | Explicit playback override, otherwise server default audio index, otherwise first audio stream index, otherwise index zero fallback. |
| Subtitle | Explicit override, otherwise server default subtitle index, otherwise Off (`-1`). |

[MediaPlayerItem.init][playerItem]. The pre-play picker uses the same general override/server-default approach, with an available first-audio fallback. [ItemActionButtons.Playback.audioStreamSelection / subtitleStreamSelection][playbackPicker]

Language preference, Prefer Default Audio, subtitle mode and remember-track switches are saved to the server user's configuration. Swiftfin does not locally run the language/forced-subtitle selection policy at initialization. [VideoPlayerSettingsView.audioSettings / subtitleSettings / updateConfiguration][playerSettings]

### The delegated server track rules

The supplementary server first attempts a valid remembered per-user/item audio choice when remembering is enabled. Otherwise it applies original-language handling when configured, or the preferred-language/default-track selector. The audio selector scores language matches and stream flags; Prefer Default Audio chooses a marked-default track ahead of the normal first-scored result. If the preferred language is absent, it falls back to the sorted available audio tracks, rather than failing playback. [MediaSourceManager.SetDefaultAudioStreamIndex / SetDefaultAudioAndSubtitleStreamIndices][serverSources], [MediaStreamSelector.GetDefaultAudioStreamIndex / GetSortedStreams / GetStreamScore][serverTracks]

For subtitles, the server can reuse a valid remembered item index or remembered Off, subject to the mode: None prevents automatic remembered selection, and Only Forced prevents restoring a remembered nonforced track. Otherwise it applies this policy:

| Mode | Actual server selection rule |
|---|---|
| None | No automatically selected subtitle. |
| Default | First sorted external, default-marked or forced track. This is broader than the settings description's simplified “default track.” |
| Smart | If selected audio is outside the preferred subtitle languages, choose a subtitle matching a preferred language; if absent, none. If audio matches, use Only Forced behaviour. |
| Always | Prefer a full/nonforced track matching a preferred language; otherwise use Only Forced behaviour. It does not blindly choose a full track in any language. |
| Only Forced | Forced subtitle in a preferred language, or forced subtitle with undefined language; prefer an actual language match. |

Among subtitle candidates the default selector sorts external first, then default-marked, preferred-language full tracks, preferred-language forced tracks, undefined-language forced tracks and other forced tracks. An empty preferred-language list acts as a wildcard. **Do not treat a forced subtitle as equivalent to a full translation.** [MediaSourceManager.SetDefaultSubtitleStreamIndex][serverSources], [MediaStreamSelector.GetDefaultSubtitleStreamIndex / BehaviorOnlyForced / MatchesPreferredLanguage][serverTracks]

### What is remembered, and what happens on the next episode

Swiftfin reports selected track indices with start/progress updates. The inspected server stores remembered indices in the user's item data; an episode's user-data keys include its season/episode identity. These functions are not a global “last selected language on any video” carry-over algorithm. Swiftfin's adjacent queue passes no current track overrides to the new episode; it asks for the new episode's defaults. Thus same-language continuation can occur through preferences/defaults, but exact current track continuation is not guaranteed by this queue. [MediaProgressObserver.sendStartReport / sendProgressReport][progress], [SessionManager.UpdatePlaybackSettings][serverSessions], [Episode.GetUserDataKeys][serverEpisode], [MediaSourceManager.SetDefaultAudioAndSubtitleStreamIndices][serverSources], [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue]

The menus list supplied stream display titles, with Unknown fallback; subtitles prepend an Off choice. The client excludes external audio streams, subtitle streams whose delivery method is Drop, and external nontext subtitles in direct-play compatibility mode. A track may therefore exist in metadata without being available in the menu. [VideoPlayer.PlaybackControls.Toolbar.ActionButtons.Audio.content][audioMenu], [VideoPlayer.PlaybackControls.Toolbar.ActionButtons.Subtitles.content][subtitleMenu], [MediaPlayerItem.init][playerItem]

Logical server indices are mapped to actual player track indices. Missing audio mapping leaves the active audio unchanged; missing subtitle mapping disables subtitles until mapping becomes available. Actual player track discovery replaces estimates and reapplies selections; sidecar discovery updates its mapping as well. The manager rejects invalid requested subtitle indices. [MediaPlayerItem.switchTrack / setTrackIndexes / updateSubtitleTrackMapping][playerItem], [MediaPlayerManager._setTrack][manager]

A compatible track change happens locally. A transcoded audio change or incompatible track delivery can require preparing a new stream while preserving the current position and selections. This depends on Jellyfin's transcoder; Bloob cannot reproduce that option simply by returning another AllDebrid URL. Audio/subtitle timing offsets reset for a new playback item. [MediaPlayerItem.isRebuildRequired][playerItem], [MediaPlayerManager._setTrack / updateMediaPlayerItem / resetPlaybackOffsets][manager]

**Inferred Bloob rule:** persist explicit Off separately from “use defaults”; remember track intent by language, role, commentary/accessibility flags and codec/channel preference, then match against each new file. Validate every remembered choice. Have the local server return resolved defaults and the player expose discovered tracks without assuming metadata index equals decoder index.

## 6. Player

### Overlay states and timers

| State | Behaviour |
|---|---|
| Hidden | Ordinary controls absent; eligible overlay actions can still exist. Initial presentation is hidden. |
| Progress | Timeline/seek feedback without the full toolbar. |
| Controls | Timeline, toolbar, playback buttons and available supplement choices. |
| Supplement | Selected extra panel, such as chapters, episodes or information. On tvOS this is its own presentation. |
| Scrubbing | Progress presentation using a separate preview position, not the live playback clock. |

Auto-hide is **10 seconds** for ordinary tvOS controls and **2 seconds** for progress-only presentation. It is suspended while paused, scrubbing, showing a supplement, interacting, displaying close confirmation or while focus is outside the player in a system menu. Pausing while hidden reveals controls. Focus/interaction updates restart the eligible timer. [VideoPlayer.ViewState.Presentation / connect / canAutoDismiss / autoDismissInterval / refreshAutoDismiss][viewState]

### Remote decision table

“Select” means the remote's central OK/select action. “Back” means tvOS Menu/Back. Directional focus movement inside visible controls is generally delegated to the platform focus system, not hardcoded card-by-card in these handlers.

| Key/state | Observed result |
|---|---|
| Select while hidden | Reveal controls. |
| Select while preview-scrubbing | Commit the preview seek and resume playback. |
| Select with timeline focused, not scrubbing | Toggle play/pause. |
| Select on another control/action | Activate the focused control normally. |
| Play/Pause while hidden and playing | Reveal controls; this branch does **not** pause on that first press. |
| Play/Pause while hidden and paused | Resume and reveal controls. |
| Play/Pause with UI visible | Toggle playback, unless scrubbing. |
| Play/Pause while scrubbing | Cancel preview; it does not commit it. |
| Left/Right on hidden/progress UI or focused timeline, for nonlive video and no supplement | Own the seek press and show progress. A short released press jumps. Right hold can speed-boost. |
| Left/Right elsewhere in visible controls/supplements | Normal focus navigation. |
| Directional press focused on overlay action | Normal action focus/navigation rather than stealing the seek press. |
| Up/Down with ordinary hidden UI | Reveal controls on press-began; visible UI then uses normal focus navigation. |
| Back while a system menu owns focus and player UI is visible | Give that menu/alert first opportunity; do not dismiss the player's underlying UI. |
| Back while scrubbing | Cancel preview. |
| Back in supplement | Close supplement. |
| Back with ordinary player UI visible | Hide UI. |
| Back with UI hidden | Confirm exit if configured, otherwise stop playback and exit. Confirmation defaults off. |

Sources: [VideoPlayer.UIContainerViewController.handleSelectEnded / handlePlayPauseEnded / handleMenuEnded][container], [VideoPlayer.PlaybackControls.handlePressEvent][pressHandling], [VideoPlayer.ViewState.commitScrub / cancelScrub][viewState], [Defaults.Keys.confirmClose][defaults]. This table describes Swiftfin's handlers; an LG D-pad without a touch surface requires an intentional Bloob mapping.

### Seeking, hold and preview

Short forward/backward jumps default to 15 seconds independently. Closely spaced taps accumulate a direction/count; pending seeks are coalesced for 250 ms and committed as one jump, then the accumulator resets. Reversing direction resets the count. The jump-feedback observer also resets after two seconds. This is accumulated tap distance, not a documented increasing-per-tap acceleration ladder. [VideoPlayer.PlaybackControls.jumpForward / jumpBackward / scheduleJump][scrubbing], [JumpProgressObserver.jumpForward / jumpBackward][jumpObserver], [Defaults.Keys.VideoPlayer.jumpForwardInterval / jumpBackwardInterval][defaults]

Holding Right for 500 ms starts a temporary playback-rate boost; the default target is 2× normal playback speed. Despite the preference being named a multiplier, this handler assigns its value as the absolute rate; it does not multiply the current rate. Releasing restores the previous rate. A short Right release performs the normal forward jump; an activated boost does not also produce that release jump. There is no corresponding left-hold reverse-speed ladder in this handler. [VideoPlayer.PlaybackControls.startSpeedBoost / stopSpeedBoost][scrubbing], [VideoPlayer.PlaybackControls.handlePressEvent][pressHandling], [Defaults.Keys.VideoPlayer.Gesture.longPressSpeedMultiplier][defaults]

The tvOS timeline permits touch/pan scrubbing when paused and not loading. Preview position is bounded, and finite usable runtime is required. Pan updates preview rather than immediately seeking the decoder; Select commits and resumes, while Back/Play-Pause/focus loss cancels. Cancel does not itself resume playback. Touch-slider fast-flick motion decays over time; this is a touch gesture mechanism, not LG arrow-repeat acceleration. [VideoPlayer.PlaybackControls.PlaybackProgress.body][playbackProgress], [UISliderContainer][slider], [VideoPlayer.ViewState.commitScrub / cancelScrub][viewState]

Preview images are optional: default preference uses server trickplay metadata for the selected source without automatic chapter fallback; the setting can instead use chapter images or enable chapter fallback. Absence of images does not prevent playback. VLC uses an absolute resume seek once seekable and keeps the UI clock tied to the media timeline. [MediaPlayerItem.build][build], [StoredValues.Keys.User.previewImageScrubbing][storedUser], [VLCMediaPlayerProxy.applyPendingStartTimeIfPossible / VideoPlayerBody][vlc]

### Chapters, intro and completion

Chapters form a supplement. With a chronological supplied chapter list, the current chapter is the latest start at/before the clock, with the first chapter as fallback; the supplement does not sort that list itself. Selecting a chapter seeks to its start and sets playback to playing. [MediaChaptersSupplement][chapters]

**No implemented skip-intro decision found in the inspected tvOS/shared playback paths.** The player has a generic overlay-action facility, but that alone does not prove an intro detector or media-segment skip feature. The chapter and episode supplements are explicit implementations. This is a bounded source finding, not a claim about every Swiftfin release or third-party plugin. [VideoPlayer.ViewState.performOverlayAction][viewState], [MediaChaptersSupplement][chapters], [MediaPlayerManager.setSupplements][manager]

Natural end handling requires a known runtime and an end event within one second of it; early end events are ignored. Unknown runtime causes stop. If a next queue item is already available and the **server user configuration** enables next-episode autoplay, the manager immediately prepares the next episode; otherwise it stops. There is no countdown/cancel-next presentation in this end path. Disable autoplay before ending or stop/exit to prevent advancement; there is no countdown-specific cancellation handler here. [MediaPlayerManager._ended / _playNewItem][manager], [VideoPlayer.PlaybackControls.Toolbar.ActionButtons.AutoPlay.body][autoplayButton]

The local `autoPlayEnabled` key is not the switch consulted by `_ended`; its effective condition is `enableNextEpisodeAutoPlay` in server user configuration. The queue needs its asynchronous adjacency request to have completed before end for a next item to exist. [Defaults.Keys.VideoPlayer.autoPlayEnabled][defaults], [MediaPlayerManager._ended][manager], [EpisodeMediaPlayerQueue.didReceive / getAdjacentEpisodes][queue]

### Progress reports and “played” thresholds

The progress observer uses a timer whose default delay is five seconds. It sends a start report on its first playing report, then periodic progress reports; paused reports include paused state. Playback-status changes reset the timer rather than proving an immediate HTTP report. It reports stop on explicit stop, item replacement and application termination. Start/progress include item, source/session, clock and selected tracks; stop includes item, source/session and clock. The timer is not a crash-safe journal, and the observer has no explicit durable retry queue. Its action handler explicitly handles stop, not an error-specific final flush; error/end handling is also marked incomplete in comments. [MediaProgressObserver.setup / sendReport / playbackItemDidChange / playbackRequestStatusDidChange / didReceive / sendStartReport / sendProgressReport / sendStopReport][progress], [PokeIntervalTimer.init / poke][timer]

Swiftfin does not calculate the watched threshold in this observer. The supplementary server applies this policy to ordinary video progress and stop reports:

| Condition | Server decision |
|---|---|
| Below configured minimum resume percentage | Store zero resume position. Default minimum is 5%. |
| Above configured maximum resume percentage, or within one second of runtime | Mark played and clear resume. Default maximum is 90%; the percentage comparison is strictly greater than the configured maximum. |
| Otherwise, file is shorter than minimum resumable duration | Mark played and clear resume. Default duration is 300 seconds. This is checked after the initial/near-end checks. |
| Otherwise | Retain reported position as resume. |
| Runtime unknown | Assume played and clear resume. |
| Stop reported as failed | The stop-processing method does not newly apply completion. |

[UserDataManager.UpdatePlayState][serverUserData], [ServerConfiguration.MinResumePct / MaxResumePct / MinResumeDurationSeconds][serverConfig], [SessionManager.OnPlaybackProgress / OnPlaybackStopped][serverSessions]. Previously stored played state is not automatically undone merely by replaying the start of a film. The supplementary server propagates a completed video's played state and cleared resume to alternate versions. [UserDataManager.UpdatePlayState][serverUserData], [SessionManager.OnPlaybackProgress / OnPlaybackStopped][serverSessions]

**Inferred Bloob adaptation:** use authoritative decoder clock/runtime, reliable final progress flush and explicit failed-versus-ended status. Provide a cancellable next-episode prompt if desired. Choose completion rules deliberately; do not mark an unknown-duration unresolved file watched merely because its metadata is incomplete.

## 7. Search

Query changes update filters, normalize blank query to no query and cancel the current search. Filter changes debounce for **500 ms** before searching. There is no explicit two/three-character minimum: a nonblank query or other queryable filters can search. Filter-only search is allowed; sort settings alone are not among the meaningful search predicates. [SearchViewModel._search / observeFilters / __actuallySearch / canSearch][searchVM], [ItemFilterCollection.hasQueryableFilters][filters]

Result group order is films, series, collections/box sets, episodes, music videos, other videos, live TV programmes, TV channels, music artists, then people. Empty groups disappear. Standard item groups preview 20. Item filters default to sort name ascending, rather than a single global relevance-ranked mixed list; people use their separate endpoint. [SearchContentGroupProvider.makeGroups][searchProvider], [ItemTypeContentGroupProvider.makeGroups][itemTypeProvider], [PosterGroup.init][posterGroup], [ItemFilterCollection.sortBy / sortOrder][filters], [ItemLibrary.retrievePage][itemLibrary], [PeopleLibrary.retrievePage][peopleLibrary]

Before searching, suggestions are a server query for ten random films/series. Selecting a suggestion fills the search term. They are not recent searches. No recent-query persistence is implemented in the inspected `SearchViewModel`, `SearchContentGroupProvider` and `SearchView` path. [SearchViewModel._getSuggestions][searchVM], [SearchView.body][searchView]

The screen has searching, error, no-results and suggestion states. Re-selecting the Search tab targets the search field. Ordinary result cards navigate through the same library-element action used elsewhere. [SearchView.body][searchView], [PosterHStackLibrarySection.body][posterSection]

**Inferred Bloob adaptation:** retain debounce and cancellation, but store recent searches and consider relevance ordering for the first results. The local server needs queryable title/person metadata and grouped results; it should not start resolving media sources for every search match.

## 8. Navigation, focus, return and deep links

The default tvOS tab sequence is Home, TV Shows, Movies, Search, Media and Settings. Each tab has its own navigation coordinator. Re-select events are published separately from switching tabs; Home/content views use them to return to the top, and Search uses them to regain query focus. [MainTabView][mainTabs], [TabCoordinator.select][tabs], [ContentGroupView.body][contentView], [SearchView.body][searchView]

On tvOS, navigation routes labelled push/sheet are presented through a sheet with a new coordinator, and fullscreen routes through fullscreen presentation. If a child is already presented, new routes are forwarded into it. This differs from the iOS branch's appended navigation path. **Inferred:** surviving underlying view objects can retain their local state while a child is presented; this is not proof of complete durable focus/scroll restoration. [NavigationCoordinator.push][navigation]

Focus rules read from source:

* Title pages initially request Play. The action bar supplies fallback focus when that action is unavailable. [ItemView.init][itemView], [ItemView.ActionBar.body][actionBar]
* A Home/library row prefers its content rather than the header, while exposing header and content as separate focus sections. The exact arrow-to-nearest-card movement is left to tvOS, not a cross-platform deterministic row algorithm. [PosterHStackLibrarySection.body][posterSection]
* Coordinated scopes remember the last selected control while alive, if it still exists; otherwise use the first. Initial-focus placeholders delay secondary controls, and fallback resolves loading that ends without a usable target. [FocusCoordinator.resolveInitialFocus / update; CoordinatedFocusScopeModifier.preferredSelection; CoordinatedInitialFocusModifier][focus]
* The top selector follows the focused poster only while that section has focus, otherwise retains its selection; its initial fallback is the first item. It does not continuously cycle through titles on a playback-like timer. [CinematicItemSelector.resolvedSelectedPoster / updateSelectedPoster / body][cinematicSelector]
* Season/episode default focus follows the main playback target, as described in section 3. No universal per-row focus/scroll persistence store was found in these navigation/focus paths. [SeriesEpisodeContentGroup][seriesGroup], [SeriesEpisodeContentGroup.SeasonEpisodesView][episodeCollection], [FocusCoordinator][focus]

Deep links accept a Swiftfin/Jellyfin scheme, known server ID, user ID and item/library destination ID. The parser currently constructs an item destination even when the URL says library; true library-destination routing remains unimplemented there. The session manager looks up the known server/user, switches/authenticates as needed and stores a pending link; the main tab consumes it and opens item details. It does not automatically play the linked title. Unknown server/user produces a link error rather than inventing a connection. [DeepLink.init / route][deepLink], [UserSessionManager.handleOpenURL / consumePendingDeepLink][sessions], [MainTabView.routePendingDeepLink][mainTabs]

**Inferred Bloob adaptation:** implement explicit return-state records containing screen ID, row ID, item ID and scroll anchor. Validate that the item still exists on return. Do not assume a native C++ focus engine will automatically reproduce tvOS's focus restoration.

## 9. Settings that change behaviour

Pure styling settings are intentionally excluded. The defaults below are client defaults only where declared; server user configuration may already contain different values.

| Setting | Behaviour changed | Ownership / source |
|---|---|---|
| Show Recently Added / Recently Played | Enable corresponding Home groups; defaults on/off. Recently Added off does not remove the top-selector fallback. | Local per user. [Defaults.Keys.Customization.Home][defaults]; [DefaultContentGroupProvider._makeGroups][home] |
| Resume/rewatch Next Up | Passes `enableRewatching` to Home Next Up; defaults off. It does not mean “always include resumable episodes.” | Local setting, server query. [NextUpLibrary.Environment.default / retrievePage][nextUp] |
| Maximum Next Up age | Last-activity cutoff for Home; default 366 days, zero unbounded. | Local setting, server query. [NextUpLibrary.retrievePage][nextUp]; [Defaults.Keys.Customization.Home.maxNextUp][defaults] |
| Recommendations | Include similar-title groups; default on. | Local. [ItemContentGroupProvider._makeGroups][itemProvider] |
| Show missing seasons/episodes | Controls whether the requests filter missing metadata placeholders out; defaults on. Server/user visibility still matters. | Local plus server. [SeasonLibrary.retrievePage][seasonLibrary]; [EpisodeLibrary.retrievePage][episodeLibrary]; [TvShowsController.GetEpisodes][serverShows] |
| Remember library sort | Reuse stored sort fields/order for a library, not all filters; default off. This is not universal focus restoration. | Local per user/library. [Defaults.Keys.Customization.Library.rememberSort][defaults]; [StoredValues.Keys.User.libraryFilters][storedUser]; [ItemLibrary.init; ItemLibraryBody.rememberSort][itemLibrary] |
| Enabled search filters | Controls filter operations available from Search. | Local. [Defaults.Keys.Customization.Search.enabledDrawerFilters][defaults]; [SearchView][searchView] |
| Player engine | Changes backend; default VLC in this revision. Engine supported cases gate available choices. | Local. [Defaults.Keys.VideoPlayer.videoPlayerType][defaults]; [VideoPlayerSettingsView.engineSettings][playerSettings] |
| Jump backward/forward | Separate short-jump durations; 15 s defaults. | Local. [VideoPlayer.PlaybackControls.scheduleJump][scrubbing]; [Defaults.Keys.VideoPlayer][defaults] |
| Right-hold multiplier | Temporary absolute boost rate; default 2× normal speed. | Local. [VideoPlayer.PlaybackControls.startSpeedBoost][scrubbing] |
| Playback speed | Applied playback rate, default 1×; stored when changed. | Local. [MediaPlayerManager.set (playback rate)][manager]; [Defaults.Keys.VideoPlayer.Playback.playbackRate][defaults] |
| Resume offset | Rewind from stored position by 0–30 s; default zero. | Local. [VideoPlayerSettingsView.resumeSettings][playerSettings]; [VLCMediaPlayerProxy.play][vlc] |
| Autoplay | Allow immediate next episode at natural end. | Server user configuration. [VideoPlayerSettingsView.resumeSettings][playerSettings]; [MediaPlayerManager._ended][manager] |
| Player action buttons | Changes available controls. In this source, editing bar/menu button lists also sets autoplay based on whether an autoplay button remains in either list. | Local controls plus server side effect. [VideoPlayerSettingsView.buttonSettings][playerSettings] |
| Chapter timeline / preview-image mode | Chapter cues and choice of trickplay/chapter previews. Missing preview metadata can yield no preview. | Local choice, server assets. [VideoPlayerSettingsView.sliderSettings][playerSettings]; [MediaPlayerItem.build][build] |
| Enabled supplements | Which useful player panels are offered; supplements requiring unavailable chapters, people or queue data are omitted. | Local. [MediaPlayerManager.setSupplements][manager]; [VideoPlayerSettingsView.supplementSettings][playerSettings] |
| Confirm player close | Require exit confirmation after Back with hidden UI; default off. | Local. [VideoPlayer.UIContainerViewController.handleMenuEnded][container]; [Defaults.Keys.confirmClose][defaults] |
| Pause on background | Pause when app backgrounds, except the active picture-in-picture case. | Local. [VideoPlayer.body][videoPlayer]; [Defaults.Keys.VideoPlayer.Transition.pauseOnBackground][defaults] |
| Audio language / prefer default / remember audio | Server default-track policy and saved item selections. | Server. [VideoPlayerSettingsView.audioSettings][playerSettings]; [MediaSourceManager.SetDefaultAudioStreamIndex][serverSources] |
| Subtitle language / mode / remember subtitles | Server automatic subtitle policy and saved item selections. | Server. [VideoPlayerSettingsView.subtitleSettings][playerSettings]; [MediaSourceManager.SetDefaultSubtitleStreamIndex][serverSources] |
| Bitrate / resolution / compatibility | Changes device profile and playback negotiation. Fixed bitrate uses the chosen cap; Auto measures a server test download and bounds its bitrate estimate. It is not client adaptive streaming or version ranking. | Local negotiation input. [PlaybackQualitySettingsView][qualitySettings]; [MediaPlayerManager.getMaxBitrate][manager]; [MediaPlayerItem.build][build] |
| Force HDR/DV transcoding / subtitle burn-in | Requests transformations from the server according to the profile. | Local preference, server transcoder required. [StoredValues.Keys.User][storedUser]; [PlaybackQualitySettingsView][qualitySettings]; [VideoPlayerSettingsView.subtitleSettings][playerSettings] |
| Hide Played in Latest; specials placement; watched/resume thresholds | Affect latest rows, episode ordering and canonical user data without a Swiftfin-side calculation. | Server policies. [UserLibraryController.GetLatestMedia][serverUserLibrary]; [ServerConfiguration][serverConfig]; [TVSeriesManager.DetermineNextEpisode][serverTV] |

**Inferred Bloob applicability:** keep controls, language/track, completion and queue preferences. Compatibility filtering can use LG capabilities and your resolver. Transcode/burn-in options require an actual transcoder or an explicitly compatible alternate source; they cannot be promised merely because Swiftfin has settings for them.

## 10. Errors and recovery

| Failure or edge case | Observed handling |
|---|---|
| Home/content provider failure | Error state; generic ErrorView exposes Retry only when a refresh action exists in its environment. Child rows can disappear when empty, rather than each becoming an error row. |
| Details fetch failure | Error state, with refresh support; deleting/invalidating the shared item dismisses the details screen. Action failures show separate error messages. |
| Optional trailers/backdrop lookup fails | Provider tolerates failure and uses no trailers/backdrop result. It does not fail the core item solely for that optional request. |
| Episode list loading/empty/error | Disabled loading placeholders, disabled No Episodes state, or a season retry action. |
| Rapid refresh/search overlap | Cancellation and paging generations reject superseded results; results are deduplicated by stable IDs. |
| Failed played/favourite write | Optimistic overlay removed, leaving confirmed state; action error surfaced. |
| No sources, no playback session, unusable URL | Preparation throws. Player presents an Unable to Load-style error and a close/dismiss path. |
| Player/backend failure | Manager stops the proxy and clears its published instance. No explicit AllDebrid-link renewal, automatic next-source fallback or durable playback recovery loop exists in these paths. Decoder/network behaviour inside the backend libraries is outside this application-source finding. |
| Network unavailable for progress reports | Observer launches asynchronous report requests; no durable retry queue is implemented there. Do not assume reports survive shutdown or offline periods. |
| Missing title/overview | Unknown / No Overview fallback; optional metadata is omitted. |
| Missing/zero runtime | Runtime is unavailable for ordinary finite seeking; manager cannot use its normal known-runtime end condition. Supplementary server's unknown-runtime completion assumption is a separate policy. |
| Missing preferred track | Server falls back according to selection mode; invalid remembered indices are ignored. Runtime missing-map fallback differs for audio and subtitles, as in section 5. |

Sources: [ErrorView.body][errorView], [ContentGroupView.body][contentView], [ContentGroupViewModel._refresh / fullRefresh / resolveGroups][contentVM], [ItemView.body][itemView], [ItemContentGroupProvider.makeGroups][itemProvider], [SeriesEpisodeContentGroup.SeasonEpisodesView][episodeCollection], [PagingLibraryViewModel.loadPage][pagingVM], [ItemStore.mutateUserData][itemStore], [MediaPlayerItem.build / streamURL][build], [VideoPlayer.body][videoPlayer], [MediaPlayerManager.onError / _ended][manager], [MediaProgressObserver][progress], [BaseItemDto.displayTitle / runtime][baseItem], [MediaSourceManager.SetDefaultAudioStreamIndex / SetDefaultSubtitleStreamIndex][serverSources].

**Inferred Bloob recovery additions:** distinguish catalogue failure, resolver failure, expired URL, unsupported file and decoder failure. Preserve the selected item, clock and user track intent. Allow a bounded URL refresh or compatible-source retry, with clear user feedback, without erasing progress or silently switching to an unsuitable version.

## Smart-default inventory

This inventories the automatic decisions established in the ten areas above. It is not a claim to list unrelated defaults throughout the entire repository. Rows marked Server are delegated rules, from the supplementary pinned server source where available.

| # | Automatic decision | Rule / owner | Source symbol |
|---|---|---|---|
| 1 | Top Home contents | Resume if nonempty, else recently added. Client. | [CinematicSelectionContentGroup.SelectionView.body][cinematic] |
| 2 | Avoid duplicate recent row | Separate recent row only while top selector uses resume. Client. | [CinematicRecentlyAddedContentGroup._shouldBeResolved][cinematic] |
| 3 | Home preview size | 20 initial items per ordinary group/resume list. Client. | [PosterGroup.init][posterGroup]; [CinematicSelectionContentGroupViewModel.init][cinematic] |
| 4 | Omit irrelevant libraries | Allowed collection types minus latest exclusions. Client/server config. | [DefaultContentGroupProvider.makeGroups][home] |
| 5 | Remove empty groups | Only resolved nonempty groups survive. Client. | [PosterGroup._shouldBeResolved][posterGroup]; [ContentGroupViewModel._refresh / fullRefresh / resolveGroups][contentVM] |
| 6 | Home content visibility | Recent added on; recent played off. Client. | [Defaults.Keys.Customization.Home][defaults] |
| 7 | Rewatch suggestions | Off for Home by default. Client. | [NextUpLibrary.Environment.default][nextUp]; [Defaults.Keys.Customization.Home.resumeNextUp][defaults] |
| 8 | Stale-series cutoff | 366 days for Home; zero means no cutoff. Client/server query. | [NextUpLibrary.retrievePage][nextUp]; [Defaults.Keys.Customization.Home.maxNextUp][defaults] |
| 9 | Resume order | Last-played descending, resumable nonvirtual items. Server. | [ItemsController.GetResumeItems][serverItems] |
| 10 | Latest grouping/order | Newest creation first; group child releases; optional hide-played. Server. | [UserViewManager.GetLatestItems / GetItemsForLatestItems][serverViews]; [UserLibraryController.GetLatestMedia][serverUserLibrary] |
| 11 | Refresh on return | Away for more than 60 s or pending rebuild, not fixed polling. Client. | [ContentGroupViewModel.refreshIfNeeded][contentVM] |
| 12 | Coalesce metadata/progress changes | Rebuild delay and spaced store refresh; preserve usable background content. Client. | [ContentGroupViewModel][contentVM]; [PagingLibraryViewModel.scheduleRefreshForStoreChange][pagingVM] |
| 13 | Prevent stale/duplicate contents | Generation/cancellation checks and item-ID deduplication. Client. | [PagingLibraryViewModel.loadPage][pagingVM] |
| 14 | Series Play target | Next Up, else resume, else available episode. Client over server results. | [ItemContentGroupProvider.resolveMediaPlayerItemProvider][itemProvider] |
| 15 | Season Play target | Resume, else available episode. Client. | [ItemContentGroupProvider.resolveMediaPlayerItemProvider][itemProvider] |
| 16 | Play label | Unaired, ended, missing, progress label, then Play. Client. | [BaseItemDto.playButtonLabel][baseItem] |
| 17 | Default title-page focus | Play when available, otherwise available action/menu. Client. | [ItemView.init][itemView]; [ItemView.ActionBar.body][actionBar] |
| 18 | Resume rewind | Stored position minus offset, clamped to zero; wait for seekable timeline. Client. | [VLCMediaPlayerProxy.play / applyPendingStartTimeIfPossible][vlc] |
| 19 | Start-over semantics | Zero the playback copy's position, not shared progress immediately. Client. | [PlayButton.play][playButton] |
| 20 | Select initial season | Play target's season if present, else first. Client. | [SeriesEpisodeContentGroup][seriesGroup] |
| 21 | Select initial episode | Play target within chosen season if present, else first. Client. | [SeriesEpisodeContentGroup.SeasonEpisodesView.preferredElementID][episodeCollection] |
| 22 | Avoid season-load thrash | Focus must rest 350 ms before implicit season switch. Client. | [SeriesEpisodeContentGroup.SeasonSelector.selectSeasonAfterFocusDebounce][seasonSelector] |
| 23 | Ordinary Next Up anchor | Furthest played regular episode; first later available unplayed episode. Server. | [NextUpService.GetNextUpEpisodesBatch][serverNextUp] |
| 24 | Specials in Next Up | Placement metadata and aired order when enabled; skip played specials. Server. | [TVSeriesManager.DetermineNextEpisode][serverTV] |
| 25 | Partial candidate allowed | Endpoint defaults resumable inclusion on; client does not override. Server. | [TvShowsController.GetNextUp][serverShows]; [NextUpLibrary.retrievePage][nextUp] |
| 26 | Rewatch progression | Optional next-played candidate after most recent played episode. Server. | [NextUpService.GetNextUpEpisodesBatch][serverNextUp]; [TVSeriesManager.DetermineNextEpisodeForRewatching][serverTV] |
| 27 | Next Up alternate version | Match previous actually played alternate's version name if available. Server. | [TVSeriesManager.GetPreferredVersion][serverTV] |
| 28 | Player next/previous | Adjacent server sequence, not Next Up; start at zero. Client/server. | [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue] |
| 29 | Default version | Explicit override else first returned source. Client. | [MediaPlayerItem.build][build]; [BaseItemDto.getPlaybackItemProvider][baseItem] |
| 30 | Server source order | Queried source first, then video file/non-3D/width preferences. Server. | [MediaSourceManager.SortMediaSources][serverSources] |
| 31 | Reset source-specific tracks | Changing version clears explicit audio/subtitle overrides. Client. | [ItemContentGroupProvider.select][itemProvider] |
| 32 | Resolve playback response | Source identity heuristics/fallback; source runtime preferred. Client. | [MediaPlayerItem.build][build] |
| 33 | Initial audio | Override, server default, first audio, zero fallback. Client. | [MediaPlayerItem.init][playerItem] |
| 34 | Initial subtitle | Override, server default, Off. Client. | [MediaPlayerItem.init][playerItem] |
| 35 | Saved audio validation | Reuse valid remembered item index, else language/default policy. Server. | [MediaSourceManager.SetDefaultAudioStreamIndex][serverSources] |
| 36 | Saved subtitle validation | Valid remembered track/Off within mode rules, else automatic policy. Server. | [MediaSourceManager.SetDefaultSubtitleStreamIndex][serverSources] |
| 37 | Preferred audio missing | Fall back to scored available tracks. Server. | [MediaStreamSelector.GetDefaultAudioStreamIndex][serverTracks] |
| 38 | Forced/smart/full subtitle decision | Mode, selected audio language, preferred language and flags. Server. | [MediaStreamSelector.GetDefaultSubtitleStreamIndex / BehaviorOnlyForced][serverTracks] |
| 39 | Hide unavailable track types | Filter external audio, dropped subtitles, certain incompatible external subtitles. Client. | [MediaPlayerItem.init][playerItem] |
| 40 | Reconcile actual player tracks | Replace estimated mappings and reapply selected tracks. Client. | [MediaPlayerItem.setTrackIndexes / updateSubtitleTrackMapping][playerItem] |
| 41 | Preserve position across rebuild | Negotiate new stream at current clock and track selections. Client/server. | [MediaPlayerManager.updateMediaPlayerItem][manager] |
| 42 | Clear timing offsets for new item | Audio/subtitle offsets reset on replacement. Client. | [MediaPlayerManager.resetPlaybackOffsets / _playNewItem][manager] |
| 43 | Paused UI remains useful | Reveal controls when hidden/paused; suppress auto-hide. Client. | [VideoPlayer.ViewState.connect / canAutoDismiss][viewState] |
| 44 | Overlay timer choice | 10 s controls; 2 s progress; stop during active interaction/menu. Client. | [VideoPlayer.ViewState.autoDismissInterval / canAutoDismiss][viewState] |
| 45 | Seek tap aggregation | Default 15 s per tap; coalesce 250 ms; reset count/direction. Client. | [VideoPlayer.PlaybackControls.scheduleJump][scrubbing]; [JumpProgressObserver][jumpObserver] |
| 46 | Right-hold behaviour | After 500 ms set temporary rate to 2× normal speed by default; restore prior rate on release. Client. | [VideoPlayer.PlaybackControls.startSpeedBoost / stopSpeedBoost][scrubbing] |
| 47 | Preview does not alter live clock | Select commits/resumes; cancel restores preview without resuming. Client. | [VideoPlayer.ViewState.commitScrub / cancelScrub][viewState] |
| 48 | Preview source | Trickplay by default; chapter fallback only if configured. Client/server assets. | [MediaPlayerItem.build][build]; [StoredValues.Keys.User.previewImageScrubbing][storedUser] |
| 49 | Chapter highlight | Latest starting chapter at/before playback time; first fallback. Client. | [MediaChaptersSupplement][chapters] |
| 50 | Protect against early end event | Advance only within 1 s of known runtime. Client. | [MediaPlayerManager._ended][manager] |
| 51 | Autoplay | Immediate next when available and server user setting allows it. Client/server. | [MediaPlayerManager._ended][manager] |
| 52 | Report cadence | Five-second poke timer, start/progress/paused/stop forms. Client. | [MediaProgressObserver][progress]; [PokeIntervalTimer.init][timer] |
| 53 | Resume/completion decision | Configurable 5% / 90% / 300 s defaults, plus near-end/unknown-runtime branches. Server. | [UserDataManager.UpdatePlayState][serverUserData]; [ServerConfiguration][serverConfig] |
| 54 | Consistent optimistic actions | Shared mutation overlay, serial execution, confirm/rollback. Client. | [ItemStore.mutateUserData][itemStore] |
| 55 | Search delay | 500 ms debounce; cancel superseded work; blank-query normalization. Client. | [SearchViewModel.observeFilters / _search][searchVM] |
| 56 | Search suggestions | Ten random films/series, not query history. Client/server query. | [SearchViewModel._getSuggestions][searchVM] |
| 57 | Search result arrangement | Type-group order, default alphabetical item sorting; empty groups omitted. Client. | [SearchContentGroupProvider.makeGroups][searchProvider]; [ItemFilterCollection][filters] |
| 58 | Scoped focus recovery | Last valid control, otherwise first; release initial gate on empty/error. Client. | [FocusCoordinator; CoordinatedFocusScopeModifier][focus] |
| 59 | Deep-link action | Known server/user session then item details, not immediate playback. Client. | [DeepLink][deepLink]; [UserSessionManager.handleOpenURL][sessions]; [MainTabView.routePendingDeepLink][mainTabs] |
| 60 | Optional metadata tolerance | Absent optional attributes omitted; optional trailers/backdrop failures tolerated. Client. | [ItemView.AttributesHStack][attributes]; [ItemContentGroupProvider.makeGroups][itemProvider] |

## Data each screen needs from Bloob's server

This is an **inferred Bloob contract**, derived from the cited consumer functions. It is a list of required information, not a prescription to implement Jellyfin's endpoint names or DTOs. Image references are included as content data; no appearance choices are prescribed.

| Consumer | Data needed | Source consumers |
|---|---|---|
| Home | Ordered catalogue sections/library IDs; availability and exclusions; ordered resume and Next Up lists; newest additions; recently played; title type/ID/name; film or episode image references; runtime; episode locator; played/favourite/resume/percentage/unplayed-child count; revisions/change events. Live programme data only if supported. | [DefaultContentGroupProvider][home]; [ResumeItemsLibrary][resume]; [NextUpLibrary][nextUp]; [LatestInLibrary][latest]; [UserSession.observeItemChanges][itemChanges] |
| Film details | Full metadata, runtime/release date/overview, genres/studios/cast, available versions and stream metadata, current user state, trailers/extras/parts/recommendations if supported, permitted actions and resolvable play target. | [ItemContentGroupProvider][itemProvider]; [ItemView.MetadataHStack][metadata]; [ItemView.AttributesHStack][attributes] |
| Series details | Series metadata; ordered seasons; explicit play-target episode and decision reason; resumability; Next Up; missing/unaired state; child progress counts; optional recommendations/extras. | [ItemContentGroupProvider.resolveMediaPlayerItemProvider][itemProvider]; [SeriesEpisodeContentGroup][seriesGroup] |
| Season/episode browser | Stable series/season/episode IDs, order numbers, special placement, availability/air dates, runtime, overview, images, played state, position/percentage and target episode. | [SeasonLibrary][seasonLibrary]; [EpisodeLibrary][episodeLibrary]; [SeriesEpisodeContentGroup.EpisodeCard][episodeCard] |
| Version picker | Stable version ID, clear name, container/video codec/resolution/HDR/DV/audio capability/bitrate, track list, runtime and compatibility/availability; preferred/default version. Bloob compatibility and ranking fields are proposed additions. | [ItemActionButtons.Playback][playbackPicker]; [MediaPlayerItem.build][build]; [ItemView.AttributesHStack][attributes] |
| Audio/subtitle menus | Stable logical track identity, decoder mapping, language/role/title/codec/channels/default/forced/accessibility flags, internal/external/delivery method and subtitle URL where applicable; default selections; explicit Off; remembered intent. Semantic intent memory is a proposed Bloob improvement. | [MediaPlayerItem.init / setTrackIndexes][playerItem]; [Audio.content][audioMenu]; [Subtitles.content][subtitleMenu]; [MediaSourceManager][serverSources] |
| Player session | Resolved playback URL and expiry/renewal context, selected stable version, runtime, authoritative position, session ID, seekability, streams, chapters and optional preview assets, ordered adjacent queue, autoplay preference, progress/report acknowledgements. Actual clock/buffering/decoder status must come from the local player; metadata alone cannot supply them. URL-expiry recovery is a proposed Bloob addition. | [MediaPlayerItem.build][build]; [EpisodeMediaPlayerQueue][queue]; [MediaProgressObserver][progress]; [MediaChaptersSupplement][chapters]; [VLCMediaPlayerProxy][vlc] |
| Search/full catalogue | Searchable metadata, query/filter options, grouping, sort order, paging/counts, lightweight card user data/images and ID; recent-query store if Bloob adds it. | [SearchViewModel][searchVM]; [SearchContentGroupProvider][searchProvider]; [ItemFilterCollection][filters]; [PagingLibraryViewModel][pagingVM] |
| Settings | Persisted user preferences and capability flags; language/forced/default/remember-track policy; autoplay; missing-item visibility; Home preferences; completion/resume policy. Local UI/remote settings need not involve server requests unless shared between devices. | [VideoPlayerSettingsView][playerSettings]; [Defaults.Keys][defaults]; [ServerConfiguration][serverConfig] |
| Return/navigation/deep-link handling | Stable item/parent IDs, existence/deletion/revision information and selected play-target IDs. Screen focus and scroll anchors can stay in the app; they are not server catalogue attributes. | [NavigationCoordinator][navigation]; [FocusCoordinator][focus]; [ItemStore.delete / invalidate][itemStore]; [DeepLink][deepLink] |

**Inferred minimum separation:** catalogue records, user-state records, version records and playback-session records should be separate identities. A temporary AllDebrid URL is not a title ID, version preference or watch-progress key.

## Ten behaviours most worth adopting

Ranking and expected everyday benefit are **inferred product judgments for Bloob**, not claims about measured Swiftfin performance. Each row identifies the observed mechanism it draws on.

| Rank | Behaviour to adopt | Why it helps everyday use | Reference |
|---|---|---|---|
| 1 | One reliable “continue this title” decision | Series Play, Home and episode focus agree about what the viewer should continue. | [ItemContentGroupProvider.resolveMediaPlayerItemProvider][itemProvider]; [SeriesEpisodeContentGroup][seriesGroup] |
| 2 | Canonical progress shared across screens | Returning from playback changes Resume, played markers and the next target consistently. | [ItemStore][itemStore]; [UserSession.observeItemChanges][itemChanges]; [PagingLibraryViewModel][pagingVM] |
| 3 | Validated language/forced-subtitle defaults | Most viewers never need to open track menus, while explicit choices remain available. | [MediaPlayerItem.init][playerItem]; [MediaSourceManager][serverSources]; [MediaStreamSelector][serverTracks] |
| 4 | Clear Back hierarchy | Back first cancels/closes the current interaction, then hides controls, then leaves playback. | [VideoPlayer.UIContainerViewController.handleMenuEnded][container] |
| 5 | Decoder-clock-based seek UI and separate preview | No misleading clock; preview can be cancelled without accidental seeks. | [VLCMediaPlayerProxy.VLCPlayerView][vlc]; [VideoPlayer.ViewState.commitScrub / cancelScrub][viewState] |
| 6 | Automatic compatible default version with optional override | Normal Play stays one action; advanced viewers can choose a version. Bloob must add compatibility ranking itself. | [ItemActionButtons.Playback.versionPicker][playbackPicker]; [MediaPlayerItem.build][build] |
| 7 | Preserve controls during pause/menu/scrub | Menus do not disappear while the viewer is reading or choosing. | [VideoPlayer.ViewState.canAutoDismiss][viewState] |
| 8 | Focus the meaningful season/episode | Large series open where the viewer is watching, not at Season 1 every time. | [SeriesEpisodeContentGroup][seriesGroup]; [SeriesEpisodeContentGroup.SeasonEpisodesView][episodeCollection] |
| 9 | Debounce requests and reject superseded replies | Rapid search and season movement do not flood the server or overwrite newer results. | [SearchViewModel.observeFilters][searchVM]; [SeasonSelector.selectSeasonAfterFocusDebounce][seasonSelector]; [PagingLibraryViewModel.loadPage][pagingVM] |
| 10 | Distinct loading/empty/error/retry and tolerant optional metadata | A missing overview or failed extras query does not turn the whole app into an unexplained spinner. | [ErrorView][errorView]; [ItemContentGroupProvider.makeGroups][itemProvider]; [SeriesEpisodeContentGroup.SeasonEpisodesView][episodeCollection] |

## Five behaviours I would not copy unchanged

These objections and replacements are **inferred Bloob design judgments**.

| # | Observed behaviour | Why I would change it in Bloob | Reference |
|---|---|---|---|
| 1 | First Play/Pause press while hidden reveals controls without pausing | A dedicated transport key should perform its transport action immediately. Reveal UI as feedback, not as an extra required press. | [VideoPlayer.UIContainerViewController.handlePlayPauseEnded][container] |
| 2 | Immediate autoplay, plus autoplay changing when control lists are edited | Give an optional cancellable next-episode prompt; keep the autoplay preference independent of whether its button is displayed. | [MediaPlayerManager._ended][manager]; [VideoPlayerSettingsView.buttonSettings][playerSettings] |
| 3 | First source default and no explicit semantic version/track carry in the adjacent queue | Bloob already has ranked versions. Preserve series intent by capability/language/role, validate it per file, then fall back safely. Do not propagate raw indices. | [MediaPlayerItem.build][build]; [EpisodeMediaPlayerQueue.getAdjacentEpisodes][queue] |
| 4 | Random suggestions without recent searches and default alphabetical result grouping | Recent queries and an initial relevance-ranked answer are more useful for repeated TV use. Keep grouped browsing as an option. | [SearchViewModel._getSuggestions][searchVM]; [SearchContentGroupProvider.makeGroups][searchProvider]; [ItemFilterCollection][filters] |
| 5 | Server assumption that unknown-duration playback is completed | Resolver metadata can be incomplete. Require trustworthy end evidence before marking watched, and retain last known progress during failure. | [UserDataManager.UpdatePlayState][serverUserData]; [SessionManager.OnPlaybackStopped][serverSessions] |

## Source index

The following index and reference links are generated from the pinned checked-out sources. A type-level citation refers to that type's relevant properties/body; more specific function names are given alongside behavioural claims. Jellyfin server files are supplementary and are labelled separately above.

| Reference | Complete file path | Repository |
|---|---|---|
| license | [LICENSE.md](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/LICENSE.md) | Swiftfin |
| home | [Shared/ViewModels/ContentGroupViewModel/DefaultContentGroupProvider.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/DefaultContentGroupProvider.swift) | Swiftfin |
| cinematic | [Swiftfin tvOS/Objects/CinematicSelectionContentGroup.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Objects/CinematicSelectionContentGroup.swift) | Swiftfin |
| posterGroup | [Shared/Objects/ContentGroup/PosterGroup.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/PosterGroup.swift) | Swiftfin |
| resume | [Shared/Objects/Libraries/ResumeItemsLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/ResumeItemsLibrary.swift) | Swiftfin |
| recent | [Shared/Objects/Libraries/RecentlyAddedLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/RecentlyAddedLibrary.swift) | Swiftfin |
| nextUp | [Shared/Objects/Libraries/NextUpLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/NextUpLibrary.swift) | Swiftfin |
| latest | [Shared/Objects/Libraries/LatestInLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/LatestInLibrary.swift) | Swiftfin |
| onNow | [Shared/Objects/Libraries/RecommendedProgramsLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/RecommendedProgramsLibrary.swift) | Swiftfin |
| defaults | [Shared/Services/SwiftfinDefaults.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/SwiftfinDefaults.swift) | Swiftfin |
| posterSection | [Shared/Components/PosterHStackLibrarySection.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/PosterHStackLibrarySection.swift) | Swiftfin |
| libraryElement | [Shared/Extensions/JellyfinAPI/BaseItemDto+LibraryElement.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/BaseItemDto%2BLibraryElement.swift) | Swiftfin |
| contentView | [Shared/Views/ContentGroupView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ContentGroupView.swift) | Swiftfin |
| contentVM | [Shared/ViewModels/ContentGroupViewModel/ContentGroupViewModel.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/ContentGroupViewModel.swift) | Swiftfin |
| pagingVM | [Shared/Objects/PagingLibrary/PagingLibraryViewModel.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/PagingLibrary/PagingLibraryViewModel.swift) | Swiftfin |
| itemChanges | [Shared/Services/UserSession/UserSession+ItemChanges.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSession%2BItemChanges.swift) | Swiftfin |
| itemView | [Shared/Views/ItemView/ItemView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/ItemView.swift) | Swiftfin |
| metadata | [Shared/Views/ItemView/Components/ItemView+MetadataHStack.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BMetadataHStack.swift) | Swiftfin |
| attributes | [Shared/Views/ItemView/Components/ItemView+AttributeHStack.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BAttributeHStack.swift) | Swiftfin |
| itemProvider | [Shared/Views/ItemContentGroupView/ItemContentGroupProvider.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemContentGroupView/ItemContentGroupProvider.swift) | Swiftfin |
| actionBar | [Shared/Views/ItemView/Components/ItemView+ActionBar.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BActionBar.swift) | Swiftfin |
| itemActions | [Shared/Views/ItemView/Components/ActionButtons/ItemActionButtons.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ActionButtons/ItemActionButtons.swift) | Swiftfin |
| playButton | [Shared/Views/ItemView/Components/PlayButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/PlayButton.swift) | Swiftfin |
| baseItem | [Shared/Extensions/JellyfinAPI/BaseItemDto/BaseItemDto.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/BaseItemDto/BaseItemDto.swift) | Swiftfin |
| vlc | [Shared/Objects/MediaPlayerManager/MediaPlayerProxy/MediaPlayerProxy+VLC.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerProxy/MediaPlayerProxy%2BVLC.swift) | Swiftfin |
| playerSettings | [Shared/Views/SettingsView/VideoPlayerSettingsView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SettingsView/VideoPlayerSettingsView.swift) | Swiftfin |
| sessionItems | [Shared/Services/UserSession/UserSession+Items.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSession%2BItems.swift) | Swiftfin |
| itemStore | [Shared/Objects/ItemStore/ItemStore.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemStore/ItemStore.swift) | Swiftfin |
| seriesGroup | [Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup.swift) | Swiftfin |
| episodeCollection | [Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup+EpisodeCollection.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BEpisodeCollection.swift) | Swiftfin |
| seasonSelector | [Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup+SeasonSelector.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BSeasonSelector.swift) | Swiftfin |
| seasonLibrary | [Shared/Objects/Libraries/SeasonLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/SeasonLibrary.swift) | Swiftfin |
| episodeLibrary | [Shared/Objects/Libraries/EpisodeLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/EpisodeLibrary.swift) | Swiftfin |
| episodeCard | [Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup+EpisodeCard.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BEpisodeCard.swift) | Swiftfin |
| queue | [Shared/Objects/MediaPlayerManager/Supplements/EpisodeMediaPlayerQueue.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/Supplements/EpisodeMediaPlayerQueue.swift) | Swiftfin |
| indicators | [Shared/Components/PosterIndicators/PosterIndicatorsOverlay.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/PosterIndicators/PosterIndicatorsOverlay.swift) | Swiftfin |
| playbackPicker | [Shared/Views/ItemView/Components/ActionButtons/PlaybackActionButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ActionButtons/PlaybackActionButton.swift) | Swiftfin |
| sourceInfo | [Shared/Extensions/JellyfinAPI/MediaSourceInfo.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/MediaSourceInfo.swift) | Swiftfin |
| build | [Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem+Build.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem%2BBuild.swift) | Swiftfin |
| playerItem | [Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem.swift) | Swiftfin |
| progress | [Shared/Objects/MediaPlayerManager/MediaProgressObserver.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaProgressObserver.swift) | Swiftfin |
| audioMenu | [Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AudioActionButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AudioActionButton.swift) | Swiftfin |
| subtitleMenu | [Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/SubtitleActionButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/SubtitleActionButton.swift) | Swiftfin |
| manager | [Shared/Objects/MediaPlayerManager/MediaPlayerManager.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerManager.swift) | Swiftfin |
| viewState | [Shared/Views/VideoPlayer/Container/VideoPlayer+ViewState.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Container/VideoPlayer%2BViewState.swift) | Swiftfin |
| container | [Shared/Views/VideoPlayer/Container/VideoPlayer+Container.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Container/VideoPlayer%2BContainer.swift) | Swiftfin |
| pressHandling | [Swiftfin tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls+PressHandling.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls%2BPressHandling.swift) | Swiftfin |
| scrubbing | [Swiftfin tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls+Scrubbing.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls%2BScrubbing.swift) | Swiftfin |
| jumpObserver | [Shared/Objects/JumpProgressObserver.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/JumpProgressObserver.swift) | Swiftfin |
| playbackProgress | [Swiftfin tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackProgress.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackProgress.swift) | Swiftfin |
| slider | [Swiftfin tvOS/Components/SliderContainer/UISliderContainer.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Components/SliderContainer/UISliderContainer.swift) | Swiftfin |
| storedUser | [Shared/SwiftfinStore/StoredValue/StoredValues+User.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/SwiftfinStore/StoredValue/StoredValues%2BUser.swift) | Swiftfin |
| chapters | [Shared/Objects/MediaPlayerManager/Supplements/MediaChaptersSupplement.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/Supplements/MediaChaptersSupplement.swift) | Swiftfin |
| autoplayButton | [Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AutoPlayActionButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AutoPlayActionButton.swift) | Swiftfin |
| timer | [Shared/Objects/PokeIntervalTimer.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/PokeIntervalTimer.swift) | Swiftfin |
| searchVM | [Shared/ViewModels/SearchViewModel.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/SearchViewModel.swift) | Swiftfin |
| filters | [Shared/Objects/ItemFilter/ItemFilterCollection.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemFilter/ItemFilterCollection.swift) | Swiftfin |
| searchProvider | [Shared/ViewModels/ContentGroupViewModel/SearchContentGroupProvider.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/SearchContentGroupProvider.swift) | Swiftfin |
| itemTypeProvider | [Shared/ViewModels/ContentGroupViewModel/ItemTypeContentGroupProvider.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/ItemTypeContentGroupProvider.swift) | Swiftfin |
| peopleLibrary | [Shared/Objects/Libraries/PeopleLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/PeopleLibrary.swift) | Swiftfin |
| searchView | [Shared/Views/SearchView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SearchView.swift) | Swiftfin |
| mainTabs | [Shared/Coordinators/Tabs/MainTabView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Tabs/MainTabView.swift) | Swiftfin |
| tabs | [Shared/Coordinators/Tabs/TabCoordinator.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Tabs/TabCoordinator.swift) | Swiftfin |
| navigation | [Shared/Coordinators/Navigation/NavigationCoordinator.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Navigation/NavigationCoordinator.swift) | Swiftfin |
| focus | [Shared/Objects/FocusCoordinator.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/FocusCoordinator.swift) | Swiftfin |
| cinematicSelector | [Swiftfin tvOS/Components/CinematicItemSelector.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Components/CinematicItemSelector.swift) | Swiftfin |
| deepLink | [Shared/Services/DeepLink.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/DeepLink.swift) | Swiftfin |
| sessions | [Shared/Services/UserSession/UserSessionManager.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSessionManager.swift) | Swiftfin |
| filterVM | [Shared/ViewModels/FilterViewModel.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/FilterViewModel.swift) | Swiftfin |
| qualitySettings | [Shared/Views/SettingsView/PlaybackQualitySettingsView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SettingsView/PlaybackQualitySettingsView.swift) | Swiftfin |
| videoPlayer | [Shared/Views/VideoPlayer/VideoPlayer.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/VideoPlayer.swift) | Swiftfin |
| errorView | [Shared/Components/ErrorView.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/ErrorView.swift) | Swiftfin |
| serverItems | [Jellyfin.Api/Controllers/ItemsController.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/ItemsController.cs) | jellyfin |
| serverNextUp | [Jellyfin.Server.Implementations/Item/NextUpService.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Server.Implementations/Item/NextUpService.cs) | jellyfin |
| serverTV | [Emby.Server.Implementations/TV/TVSeriesManager.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/TV/TVSeriesManager.cs) | jellyfin |
| serverViews | [Emby.Server.Implementations/Library/UserViewManager.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/UserViewManager.cs) | jellyfin |
| serverUserLibrary | [Jellyfin.Api/Controllers/UserLibraryController.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/UserLibraryController.cs) | jellyfin |
| serverShows | [Jellyfin.Api/Controllers/TvShowsController.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/TvShowsController.cs) | jellyfin |
| serverSeries | [MediaBrowser.Controller/Entities/TV/Series.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Controller/Entities/TV/Series.cs) | jellyfin |
| serverSources | [Emby.Server.Implementations/Library/MediaSourceManager.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/MediaSourceManager.cs) | jellyfin |
| serverTracks | [Emby.Server.Implementations/Library/MediaStreamSelector.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/MediaStreamSelector.cs) | jellyfin |
| serverSessions | [Emby.Server.Implementations/Session/SessionManager.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Session/SessionManager.cs) | jellyfin |
| serverEpisode | [MediaBrowser.Controller/Entities/TV/Episode.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Controller/Entities/TV/Episode.cs) | jellyfin |
| serverUserData | [Emby.Server.Implementations/Library/UserDataManager.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/UserDataManager.cs) | jellyfin |
| serverConfig | [MediaBrowser.Model/Configuration/ServerConfiguration.cs](https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Model/Configuration/ServerConfiguration.cs) | jellyfin |
| itemLibrary | [Shared/Objects/Libraries/ItemLibrary.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/ItemLibrary.swift) | Swiftfin |
| itemActionKinds | [Shared/Objects/ItemActionButton.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemActionButton.swift) | Swiftfin |
| description | [Shared/Views/ItemView/Components/ItemView+Description.swift](https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BDescription.swift) | Swiftfin |

[license]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/LICENSE.md
[home]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/DefaultContentGroupProvider.swift
[cinematic]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Objects/CinematicSelectionContentGroup.swift
[posterGroup]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/PosterGroup.swift
[resume]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/ResumeItemsLibrary.swift
[recent]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/RecentlyAddedLibrary.swift
[nextUp]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/NextUpLibrary.swift
[latest]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/LatestInLibrary.swift
[onNow]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/RecommendedProgramsLibrary.swift
[defaults]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/SwiftfinDefaults.swift
[posterSection]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/PosterHStackLibrarySection.swift
[libraryElement]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/BaseItemDto%2BLibraryElement.swift
[contentView]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ContentGroupView.swift
[contentVM]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/ContentGroupViewModel.swift
[pagingVM]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/PagingLibrary/PagingLibraryViewModel.swift
[itemChanges]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSession%2BItemChanges.swift
[itemView]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/ItemView.swift
[metadata]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BMetadataHStack.swift
[attributes]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BAttributeHStack.swift
[itemProvider]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemContentGroupView/ItemContentGroupProvider.swift
[actionBar]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BActionBar.swift
[itemActions]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ActionButtons/ItemActionButtons.swift
[playButton]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/PlayButton.swift
[baseItem]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/BaseItemDto/BaseItemDto.swift
[vlc]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerProxy/MediaPlayerProxy%2BVLC.swift
[playerSettings]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SettingsView/VideoPlayerSettingsView.swift
[sessionItems]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSession%2BItems.swift
[itemStore]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemStore/ItemStore.swift
[seriesGroup]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup.swift
[episodeCollection]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BEpisodeCollection.swift
[seasonSelector]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BSeasonSelector.swift
[seasonLibrary]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/SeasonLibrary.swift
[episodeLibrary]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/EpisodeLibrary.swift
[episodeCard]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ContentGroup/SeriesEpisodeContentGroup/SeriesEpisodeContentGroup%2BEpisodeCard.swift
[queue]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/Supplements/EpisodeMediaPlayerQueue.swift
[indicators]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/PosterIndicators/PosterIndicatorsOverlay.swift
[playbackPicker]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ActionButtons/PlaybackActionButton.swift
[sourceInfo]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Extensions/JellyfinAPI/MediaSourceInfo.swift
[build]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem%2BBuild.swift
[playerItem]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerItem/MediaPlayerItem.swift
[progress]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaProgressObserver.swift
[audioMenu]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AudioActionButton.swift
[subtitleMenu]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/SubtitleActionButton.swift
[manager]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/MediaPlayerManager.swift
[viewState]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Container/VideoPlayer%2BViewState.swift
[container]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Container/VideoPlayer%2BContainer.swift
[pressHandling]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls%2BPressHandling.swift
[scrubbing]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackControls%2BScrubbing.swift
[jumpObserver]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/JumpProgressObserver.swift
[playbackProgress]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Views/VideoPlayer/PlaybackControls/Components/PlaybackProgress.swift
[slider]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Components/SliderContainer/UISliderContainer.swift
[storedUser]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/SwiftfinStore/StoredValue/StoredValues%2BUser.swift
[chapters]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/MediaPlayerManager/Supplements/MediaChaptersSupplement.swift
[autoplayButton]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/Components/Toolbar/ActionButtons/AutoPlayActionButton.swift
[timer]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/PokeIntervalTimer.swift
[searchVM]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/SearchViewModel.swift
[filters]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemFilter/ItemFilterCollection.swift
[searchProvider]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/SearchContentGroupProvider.swift
[itemTypeProvider]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/ContentGroupViewModel/ItemTypeContentGroupProvider.swift
[peopleLibrary]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/PeopleLibrary.swift
[searchView]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SearchView.swift
[mainTabs]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Tabs/MainTabView.swift
[tabs]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Tabs/TabCoordinator.swift
[navigation]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Coordinators/Navigation/NavigationCoordinator.swift
[focus]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/FocusCoordinator.swift
[cinematicSelector]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Swiftfin%20tvOS/Components/CinematicItemSelector.swift
[deepLink]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/DeepLink.swift
[sessions]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Services/UserSession/UserSessionManager.swift
[filterVM]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/ViewModels/FilterViewModel.swift
[qualitySettings]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/SettingsView/PlaybackQualitySettingsView.swift
[videoPlayer]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/VideoPlayer/VideoPlayer.swift
[errorView]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Components/ErrorView.swift
[serverItems]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/ItemsController.cs
[serverNextUp]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Server.Implementations/Item/NextUpService.cs
[serverTV]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/TV/TVSeriesManager.cs
[serverViews]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/UserViewManager.cs
[serverUserLibrary]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/UserLibraryController.cs
[serverShows]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Jellyfin.Api/Controllers/TvShowsController.cs
[serverSeries]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Controller/Entities/TV/Series.cs
[serverSources]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/MediaSourceManager.cs
[serverTracks]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/MediaStreamSelector.cs
[serverSessions]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Session/SessionManager.cs
[serverEpisode]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Controller/Entities/TV/Episode.cs
[serverUserData]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/Emby.Server.Implementations/Library/UserDataManager.cs
[serverConfig]: https://github.com/jellyfin/jellyfin/blob/aec42b345e1a7da25ee5492183f12209338df688/MediaBrowser.Model/Configuration/ServerConfiguration.cs
[itemLibrary]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/Libraries/ItemLibrary.swift
[itemActionKinds]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Objects/ItemActionButton.swift
[description]: https://github.com/jellyfin/Swiftfin/blob/791fe1249e12ffd066c763f28b4291866a14ac49/Shared/Views/ItemView/Components/ItemView%2BDescription.swift


