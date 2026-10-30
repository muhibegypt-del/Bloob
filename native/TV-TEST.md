# Bloob 2.0 on the TV: test script

Run this script on the OLED55G26LA from the sofa, at normal viewing distance (about 2.5 m), with the LG remote only. Write down what happens. Do not write down what should have happened. Do not photograph account screens or PINs.

## Install

```bat
cd /d %USERPROFILE%\Downloads
ares-install -d rootedTV com.muhib.bloob_1.2.0_arm.ipk
ares-launch -d rootedTV com.muhib.bloob
```

The log is at `ssh root@192.168.1.26 "cat /tmp/bloob-native.log"`. It records hosts only, never links or keys.

## 1. The acceptance task (hand the remote to someone and explain nothing)

Ask them to: **"Pick an episode of a series, start the version Bloob recommends, switch the audio to another track, then go back to browsing."**

| Step | Pass if | Note time / hesitation |
| --- | --- | --- |
| Find a series on Home or Series | They reach a series without help | |
| Open it and choose an episode | Down reveals seasons and episodes; they pick one | |
| Understand the source screen | They can say in their own words what *Best match* will give them | |
| Start it | Picture within a few seconds; the startup screen names what is starting | |
| Change audio | They find *Audio* in the controls, pick a track, and hear the change | |
| Return to browsing | Back takes them out of the player, then to the details page, then Home, with the same focus as before | |

Write down every place where they paused, asked something or pressed a wrong key. Those are the design bugs.

## 2. Viewing distance and the look

- [ ] All text readable from the sofa. The smallest text is the episode numbers, badges and hints.
- [ ] Text edges are smooth, with no jaggies on rounded corners, rings, the play disc or icons.
- [ ] Focus is obvious within a glance on every screen. Exactly one thing is white or lifted.
- [ ] Backdrops follow focus without flicker when Right is held on a shelf.
- [ ] Panels (Release details, Audio, pairing) are solid enough that the artwork does not show through the text.
- [ ] Missing artwork shows a tone and the title, never an empty box.

## 3. Playback lifecycle (the 0.4 bug)

- [ ] Play something, open the LG quick menu (gear key), change the brightness, and close the menu. **Playback must carry on without pausing.** The log shows `focus lost` / `focus gained` and no `-> pause`.
- [ ] Press Home and wait 10 s, then come back to Bloob. Playback is paused at the same point, or shows *Reconnecting* and continues there. The log shows `reloading at <ms> (pipeline released while away)` if the TV released it.
- [ ] Seek: hold Right, then release. The bubble shows the target. The log line `seek to X landed at Y` has Y close to X.
- [ ] Subtitles: turn a track on and off.
- [ ] At the end of an episode, *Up next* counts down. Back stays.
- [ ] Exit the player and look at the log: there is one `unload`, and no errors after it.

## 4. Persistence

- [ ] Pair AllDebrid once. Quit Bloob from the Back double-press, launch it again, and play. You are not asked to pair again. Settings says *saved on this TV*.
- [ ] Settings → *Disconnect AllDebrid* (OK twice). Quit and relaunch: Bloob is still disconnected, and the web app's pairing was not touched. Pair again afterwards.

## 5. Capturing screens for the design review (unverified procedure)

`com.webos.service.capture` may be able to save what is on the panel. This has not been tried on this TV, so treat it as unverified. Capture **UI screens only**. Never capture account screens, PINs, or protected content: the TV blanks protected video, and we do not try to work around that.

```sh
ssh root@192.168.1.26 "luna-send -n 1 -f luna://com.webos.service.capture/executeOneShot '{\"path\":\"/tmp/bloob-home.png\",\"method\":\"DISPLAY\",\"width\":1920,\"height\":1080,\"format\":\"PNG\"}'"
scp root@192.168.1.26:/tmp/bloob-home.png .
```

If the call is refused, photograph the screen from the sofa instead. A photo at viewing distance is the better test of legibility anyway. Files saved in `/tmp` disappear on reboot, so nothing needs to be deleted on the TV.

## 6. Elements of A reference pass (UI organisation only)

The goal is to compare how it *feels*. It is not to copy anything. For the same moments in Elements of A, record:

1. **Home:**
   - How long until artwork appears.
   - What changes when focus moves (backdrop, text, card size).
   - How long it waits before the backdrop changes.
2. **Title page:** the order of text, and where the eye lands first.
3. **Season selection:**
   - Does the page continue or change?
   - What stays visible?
4. **Episode row:**
   - Card size and spacing.
   - What the focused card shows that the others do not.
5. **Loading:** what is on screen between OK and the first frame.
6. **Player:**
   - What appears on the first key press.
   - How seeking previews and commits.
   - How it goes away again.

Note timings by eye or from a phone's slow-motion video. Label each finding as a **live observation**, with the date. Do not capture the reference app's account or playback screens.

## 7. New in 1.2 (record what the log and the screen say)

The log lines below are written for exactly this pass. None of these behaviours has run on the TV yet.

**Measurement lines.** After one ordinary viewing, copy these lines from `/tmp/bloob-native.log`:
- `[launch] window / fonts / service / home-data / home-frame at … ms`: cold start, stage by stage.
- `[perf] keys n=… key->present p50/p95/max; frames … render …`: every 5 s while the UI moves. If it says `effects -> lite`, the TV measured itself slow and turned shadows and blur off for the session; note which screen you were on.
- `[qoe] first frame …; stalls …; media watched …; seeks …; reloads …`: one line per viewing, on leaving the player.

**Playback.**
- [ ] **Start position (P5).** Resume something with saved progress. The log says either `started at … ms from the load request` (the TV honoured the start option) or `the load start was not honoured … seeking` (fallback). If it says `load with a start position refused`, playback still starts from the beginning and then the next line shows the reason; record it.
- [ ] **Tracks after a reconnect (F7).** Choose a second audio track, press Home, wait 10 s, return. After *Reconnecting*, the same track plays (`audio track N (…)` in the log).
- [ ] **Preferred languages.** Set Settings → Subtitles to English. Start a film with English subtitles: they come on by themselves. With a preferred audio language that the file has as a second track, that track starts.
- [ ] **Subtitle position (U7).** With subtitles on, press Down: the subtitle line should rise above the controls, and drop back when they hide. If the log says `subtitle position not supported here`, record that.
- [ ] **Reduced seek (U6).** With the controls hidden, press Right: only the timeline appears and it hides about 2 s after the seek lands. In 2.0, resting on a control keeps the bar up for about 8 s (see §8).
- [ ] **Up next prepared (U10).** Near the end of an episode, the log says `preparing S… · E… during the credits` and then `next episode is being prepared`. *Play now* reaches the picture faster than an ordinary start; time it. With Settings → *Play the next episode automatically* off, there is no countdown.
- [ ] **Clock.** The wall clock (top right) and *Ends at* match the TV's own clock.
- [ ] **ACG (P1).** Run `native/scripts/acg-lookup.sh` from the build machine and keep its output: it lists which permission groups this webOS build defines for the media, settings, connection-manager and TTS services. `requiredACG` is not yet set in `appinfo.json`; it will be filled from this output.

**Questions only the TV can answer (record yes/no and the log line):**
- [ ] **Frame rate.** On 23.976 fps content, does the panel switch to 24p (TV info overlay, or *Diagnostics → Panel*)? Bloob does not request it; the TV may do it on its own.
- [ ] **TrueHD passthrough.** On a file whose first audio track is Dolby TrueHD Atmos, what does the soundbar or receiver show: Atmos, TrueHD, Dolby Digital Plus or PCM? The source screen warns this may play as standard sound.
- [ ] **DTS.** On a version the source screen marks *No sound on this TV*, confirm there is indeed no sound (do not leave it playing).
- [ ] **Text-to-speech.** Replaced by §8 *Spoken interface* in 2.0.
- [ ] **Unknown media events.** Copy every `[media] unknown event key …` line and the `audio track fields: …` line. They show what this TV's pipeline reports that Bloob does not yet read.

**Look and comfort.**
- [ ] **First run.** On a TV that has never paired, Home shows the AllDebrid note once. *Later* dismisses it for good.
- [ ] **Season row.** On a series with many seasons, every season is reachable with Right, and chevrons show more on either side.
- [ ] **Search.** Type, then press Right into the results: the keyboard slides away, the query stays as a chip, five columns show (2.0). Left from the first column brings the keyboard back.
- [ ] **Reduce motion.** With Settings → *Reduce motion* on, nothing slides, zooms or advances by itself.
- [ ] **Small text.** The smallest text (episode numbers, badges, captions) is 23 px in 2.0; check it is readable from the sofa.
- [ ] **Non-Latin titles.** A title with Polish, Czech, Greek or Cyrillic letters shows them correctly. Chinese and Japanese still show as `?` (no font for them yet).

## 8. New in 2.0: Glass (record what the screen and the log say)

Nothing in this section has run on the TV yet. Install `artifacts/com.muhib.bloob_2.1.0_arm.ipk`. Take photos at viewing distance where a check is about looks.

**Play goes straight to the picture (G4).**
- [ ] On a film's title page, *Play* shows only the *Starting* sheet and then the picture, without the version list. Time it from the press to the first frame (`[player] first frame after … ms`).
- [ ] *How to watch* shows *Finding versions…*, then the best version ticked and two alternatives. Down, Right, OK plays the alternative; the log's `[sources]` line names that version.
- [ ] *Cancel* (or Back) on the *Starting* sheet returns to the title page. If the version fails, the failure sheet appears, and Back from it shows the other versions.
- [ ] Unpaired: *Play* opens pairing at once; *Cancel* returns to the title page.

**Glass (G1–G3).**
- [ ] The tab bar, How to watch and the pairing sheet look **frosted** (blurred colour of the artwork), not flat grey. If they stay grey for more than about a second after the picture appears, note the screen: the frost bake is slow on this TV.
- [ ] Changing the hero (Left/Right on Home) crossfades the frost with the picture, without a flash of grey.
- [ ] On a bright picture (sky, snow), the white text on How to watch is still easy to read.
- [ ] `[perf]` lines while browsing: note render p95. If the log says `effects -> lite`, glass turned solid for the session; note where.
- [ ] Settings → *Reduce transparency*: every panel turns solid grey, including the player bar. *Increase contrast*: grey text becomes brighter. Both survive an app restart.

**Player (G5).**
- [ ] The bar floats above the bottom edge; the video under it stays sharp (smoked, not blurred).
- [ ] Timers: the bar hides about 5 s after the last key while playing, about 8 s when paused or resting on a control. Paused, the bar fades and the paused frame can be looked at; any key brings it back.
- [ ] Seek commit: with the timeline focused, three quick Rights land as one seek about 0.4 s after the last press (`[player] seek to … landed at …`).
- [ ] Up from the timeline reaches the controls; Down returns.
- [ ] Subtitle lift (U7): with subtitles on, the line sits above the bar while it shows. If it is hidden behind the bar, note the subtitle position level in use (4 for the full bar, 2 for the timeline alone).

**Spoken interface (G7).** Turn on LG Audio Guidance (Settings → Accessibility).
- [ ] The log says `[speech] audio guidance on`.
- [ ] Moving focus speaks: tabs ("Home, tab, 2 of 5"), hero buttons, cards with position, title page buttons, versions, episodes, keyboard letters, settings switches, player controls and track choices.
- [ ] If nothing speaks and the log says `the TV refused speech (…)`, record the reason: the TTS service or its permission group differs on this TV (compare with `acg-lookup.sh`).
- [ ] The TV's own reader does not talk over Bloob (the app declares `supportsAudioGuidance`).
- [ ] The Home slideshow does not advance while Audio Guidance is on.


## 9. New in 2.1: continuity (UX-PLAN X1–X3)

Install `artifacts/com.muhib.bloob_2.1.0_arm.ipk`. These checks answer the 2.0 viewing report: each episode picked its own version (often HD) and there was no way to choose a version per episode.

**The same version, episode after episode (X3).**
- [ ] Play E1 of a show in 4K. Let Up next play E2, then E3. Each *Starting* sheet should say *same version as your last episode* or *like your last episode*. The log's `[sources]` line should name a 4K version each time.
- [ ] On the title page after an episode, the line under *Play* names the version. If the earlier version isn't offered, it says so.
- [ ] *All versions* for an episode shows three sections: the matches first, then *Other versions*, then *May not play well on this TV*. Season packs and single episodes are tagged.
- [ ] Choose a 1080p version by hand for one episode. The next episode should then keep to 1080p.
- [ ] On the *Starting* sheet, Right then OK on *Other versions* stops the start and shows the list.

**Progress (X1).**
- [ ] Stop an episode in the closing credits, with less than about 3 minutes left on a 45-minute episode. Home's Continue should offer the next episode, not a resume.
- [ ] Stop a film halfway, then quit the app with Back twice. Relaunch: it resumes within about 15 s of where it stopped. Record the gap; that is the loss window.
- [ ] *Remove from Continue* on a title page takes the show off Home. Playing it again brings it back.

**Next action (X2).**
- [ ] For a show whose next episode hasn't aired, the title page says *You're up to date* and when it airs. Continue says the day, and pressing it opens the title page instead of playing.
- [ ] An episode that aired since you finished the previous one shows *New episode* on Home.

## 10. New in 2.4: runtime resources (R5a–R5d, R6r)

Install `artifacts/com.muhib.bloob_2.4.0_arm.ipk`. These checks measure what the Elements of A resource study changed. Keep a second terminal on `native/scripts/tv-inspect.sh root@<tv> --watch`, which is read-only.

**Baseline record.** Save one inspector snapshot for each of these:
- Home idle;
- Home while scrolling a row;
- a title page;
- playback with the controls hidden;
- playback with the controls shown;
- after pressing the Home key while playing;
- after returning.

Copy the `frames`, `gpu`, `images` and `memory` blocks into `research/` with the date.

**Responsiveness.**
- [ ] Open a title from Home: the title page's text and art should arrive without the brief stall that 2.3 could show. Before 2.4, worker replies waited for an unrelated wake.
- [ ] While idle on Home, `frames.total` does not climb.
- [ ] During playback with the controls hidden, `frames.total` grows by nearly nothing. In 2.3 every 200 ms clock report repainted the screen.
- [ ] The first focus move after a pause glides; it does not jump.
- [ ] `[perf]` lines: record key→submit p95, render p95, the stage p95s and allocations per frame on Home and while scrolling.

**Memory.**
- [ ] `memory.rss_kb` on Home after scrolling every row; `gpu.used` by class. These set the budgets in `resources.cpp`.
- [ ] Press the Home key while on Bloob: the log says `policy background` and `released … textures`, and `memory.rss_kb` drops. Return: the visible art is back within a moment, without fading in again.
- [ ] Start playback: the log says `policy playback`, and `gpu.used` stays under 56 MB.
- [ ] The first `[memory] system memory state:` line says whether the TV's memory manager answers. Record it.

**Transitions.**
- [ ] Open and close a title page several times: `frames.layer_reuses` grows much faster than `layer_paints`, and the transition looks the same as in 2.3.

**Log check.** `SDL linked …`, `renderer flags`, `vsync on|off`: record them. `vsync off` means frames are gated to one per refresh.

## 11. New in 2.5: sources, picture, subtitles, versions list

Install `artifacts/com.muhib.bloob_2.5.3_arm.ipk`. Keep `/tmp/bloob-native.log` open; every check below names the line to read.

**Subtitles (the size and background are new; the TV draws the text).**
- [ ] Settings > Subtitle size is *Small* by default (the TV's level 0; the TV's own default is level 2). Turn subtitles on in a film: the log shows `setSubtitleFontSize,"fontSize":0` after `setSubtitleEnable`. Is the text clearly smaller than before?
- [ ] Try *Medium* and *Large (the TV's usual size)*: each step should be visibly larger. If no step changes anything, record it: the TV ignores the call, and the next step is drawing subtitles in Bloob (see `research/Subtitle-styling-on-webOS.md`).
- [ ] Subtitle background on: a dark box behind the words (`setSubtitleBackgroundOpacity,"bgOpacity":191`).
- [ ] Switch subtitles off and on again from the player's track panel: the size stays (it is re-sent on every enable).

**Dolby Vision (2.5.1: MKV Dolby Vision works on this TV, per the owner).**
- [ ] A Dolby Vision MKV version is labelled *Dolby Vision* in the versions list and on the player's info line.
- [ ] Play one and record two things: whether the TV's own picture mode or Dolby Vision badge shows Dolby Vision, and the log line `[qoe] picture: version claims Dolby Vision (p8), TV reports …`. If the TV shows Dolby Vision but the line says `HDR10`, uMediaServer names only the base layer, and the player's badge is right to follow the version.
- [ ] Settings > Picture: *HDR10, skip Dolby Vision* and *Prefer SDR* change the best version (the card's reason says "as your picture setting asks" for SDR).

**Sources (several indexes).**
- [ ] Open All versions for a popular film: the bottom line says `Found by Torrentio, TorrentsDB, StremThru, The Pirate Bay · N versions`. Record N and how long the list took compared with 2.4.
- [ ] Open a rarer title: are there 4K remux or Dolby Vision versions that 2.4 did not list?
- [ ] Settings > Sources > *Torrentio only*: the list is back to Torrentio's alone.
- [ ] Play a version found only by StremThru or The Pirate Bay (Right shows *Found by*). It has no file index, so AllDebrid's file list decides; a series pack should still pick the right episode.

**Versions list.**
- [ ] Down from the best version reaches the filter pills (*All · 4K · 1080p · 720p and below · May not play well*, with counts); Left and Right filter; Down enters the list.
- [ ] Sections by kind of release: *Remux*, *Blu-ray encodes*, *Web downloads*, *Web rips*… Each row's second line says source, size and release group.
- [ ] Is this easier to choose from than 2.4? Note what still isn't.

**Playback info panel (2.5.3).**
- [ ] During playback press the ⓘ control (or the blue key from the remote's colour buttons): the panel opens top left and updates twice a second. Back or ⓘ closes it.
- [ ] Network shows a sensible rate for a remux (tens of Mbps) on `wlan0`. Record what *Buffer* says, and the log line `[media] bufferRange: …` (it shows which fields this TV sends).
- [ ] *File bitrate* shows the size and average bitrate once the running time is known.

## 12. New in 2.6: Bloob as the home screen

**Apps row (app only, before the launcher hook).**
- [ ] Home shows *Apps* under the hero: YouTube first (if installed), then other apps with their own icons, a divider, then HDMI inputs. If the row is missing, record the `[apps]` line in `/tmp/bloob-native.log` (the TV may refuse `listLaunchPoints`; the Homebrew Channel route should then answer).
- [ ] Down from the hero buttons reaches the row; Up from the first shelf comes back to it; OK opens the app; returning to Bloob keeps focus on that tile.
- [ ] Pages without artwork are pure black.

**Launcher hook (`native/launcher/README.md`, only once the above passes).**
- [ ] Power off and on: Bloob appears instead of LG's Home (within a few seconds). Wake from standby: the same.
- [ ] The Home button from YouTube or an HDMI input lands on Bloob.
- [ ] Back on Bloob's Home does nothing (no "Press Back again" toast).
- [ ] One film plays start to finish as before.
- [ ] `/tmp/bloob-home.log` shows the takeovers; record Bloob's memory while hidden (`tv-inspect.sh`).

## 13. New in 2.8: the cinephile versions list

- [ ] A film with several cuts (e.g. The Lord of the Rings) opens on an edition row: Theatrical first unless you chose another cut before. Left/Right switches cut; the best card and list follow.
- [ ] Choose an Extended version; the next film with several cuts opens on Extended.
- [ ] Sections run UHD Blu-ray Remux, Blu-ray Remux, 4K disc encodes, 4K web, Blu-ray encodes, HD web; each row is one line plus a comparison ("Same disc, no Atmos, 40% smaller").
- [ ] Repeated releases appear once ("listed 3 times" in details); "+N more" rows unfold.
- [ ] With AllDebrid paired, some rows say "Ready now", and those start without a download wait. Check AllDebrid's magnet list afterwards: nothing new left behind except what you played.
- [ ] Dolby Vision P7 remuxes read "HDR10 (Dolby Vision P7 disc)".

**DTS (2.8.1).**
- [ ] A remux that starts on DTS with a Dolby/AAC track beside it switches by itself (log: `skipping a DTS track for …`); the audio panel shows the playing track.
- [ ] A DTS-only version pauses with "This version's sound is DTS only…" and *Choose another version* returns to the list.

## 14. New in 3.0: Bloob's own engine

3.0 plays through Bloob's engine (`src/es_player.*`, `src/demux.*`, `src/starfish.cpp`): Bloob reads the file with its own FFmpeg and feeds the TV's decoders through LG's elementary-stream pipeline (`libplayerAPIs`, the route Kodi's webOS build uses). None of it has run on the TV. It was chosen by deduction from the owner's report that every remux said "audio codec not supported" (remuxes start on TrueHD or DTS, which LG's player doesn't decode); the TV pass is the first evidence.

Read `/tmp/bloob-native.log` after each step: `[engine]` and `[demux]` lines say what happened, and `[player] engine gave up (…)` means LG's player took over.

1. **Pipeline.** Start any version. Expect `[engine] TV media pipeline ready (libplayerAPIs)`, then `[demux] MKV H265 3840x2160 …`, `[engine] pipeline loading at …`, `[engine] first frame at …`. If instead `no libplayerAPIs` or `Load refused`, LG's player takes over: note the line.
2. **Remux sound.** A UHD remux with TrueHD Atmos or DTS-HD MA: no "audio codec not supported" message, and sound plays. The info panel (blue key) shows `Engine: Bloob engine` and the Audio row `TrueHD 7.1 → Dolby Digital Plus 5.1 (converted by Bloob)`. Atmos from TrueHD is not kept (E-AC-3 conversion has no Atmos); E-AC-3 Atmos versions pass through with Atmos.
3. **Picture.** A Dolby Vision profile 8 version: does the TV show Dolby Vision? A profile 7 remux: plays as HDR10 (`base layer only` in the `[demux]` line). An HDR10 version: HDR10.
4. **Seeking.** Seek forward and back. Each seek reloads the pipeline at the exact time (`[engine] pipeline loading at …` then `seek … landed …`). Note how long the picture takes to come back.
5. **Tracks.** Change the audio language: the pipeline reloads where it was. Turn on subtitles: Bloob draws them (text in the Settings size, Blu-ray PGS pictures as on the disc), and they rise above the bar when it shows.
6. **CPU.** During a TrueHD 7.1 conversion, the info panel's Bloob CPU figure. Sound that stutters or drifts means the TV's processor can't keep up.
7. **Fallback.** If the engine fails before a picture, or shows none within 20 s, LG's player takes over at the same place (with the audio bridge for a DTS- or TrueHD-only version). Note the reason in the log.

## 15. New in 3.1: anime and more searchers

1. **Anime episode.** Open a current anime (for example Frieren, season 2), choose an episode, and open the versions list. Expect releases from SeaDex, Nyaa and Torrentio (anime) under *Found by*, the SeaDex pick first and tagged *SeaDex best*, and no "Tagged Japanese audio" warning.
2. **Pack.** Play a SeaDex or batch version: the right episode starts, without a file chooser.
3. **Long runner.** One Piece or Detective Conan: releases by absolute number appear.
4. **Ordinary titles.** A film and a series episode now also list Knaben under *Found by*.
5. **First start.** The anime index downloads once (about 6 MB) a few seconds after the service starts; `[anime] index: … titles` in the service log.

## 16. New in 3.1.2: is the video 4K?

Open the info panel (blue key) during a 4K version. **Video** is what the file says; **Decoder** (Bloob's engine) is what the TV's decoder reports; **TV output** is what the TV's video output service reports for the main video. 4K playback shows `3840×2160` in all three. The first reply of the output service is logged once as `[qoe] video output status: …` in `/tmp/bloob-native.log`; if the row says "not reported", that line shows the TV's own field names.

## 17. New in 3.2: Headphones mode

Connect the WF-1000XM5 earbuds to the TV first (*Sound out* → Bluetooth). Bloob follows the TV's sound output: `[sound] the TV's sound goes to bt_soundbar` in `/tmp/bloob-native.log`.

1. **It turns itself on.** Play a remux (TrueHD or DTS) and a web version (E-AC-3 Atmos). Open the info panel (blue key): **Headphones** reads *On · Lounge · XM5 · +9 dB*, and **Audio** reads *… → Headphones (…)*. The log shows `[headphones] 7.1 render of …` or `5.1(side) render of …`.
2. **Surround.** Voices sit in front, outside the head; effects pan round behind. Compare with Headphones *Off* in Settings: the TV's own downmix sits between the ears.
3. **Rooms.** In the player, open *Audio*. Below the tracks: **Room** and **Lip-sync**, changed with Left/Right. The sound reloads about a second after the last press. Try Lounge, Studio and Cinema.
4. **Lip-sync.** Watch a close-up of someone talking. If the voice comes after the lips, raise Lip-sync 20 ms at a time until they match. Write the number down: it is this TV's Bluetooth delay.
5. **Loudness.** Loud scenes should never crackle (the limiter holds −1 dBFS). If films are still too quiet at full TV volume, raise *Headphone volume boost* in Settings (up to +15 dB). *Night listening* holds back explosions.
6. **Taking them out.** Disconnect the earbuds: within about a second the sound reloads as Dolby 5.1 to the TV speakers (`[player] headphones off`).
7. **CPU.** With the info panel open during a TrueHD 7.1 film, note *Bloob CPU*. Desktop measurements suggest the render costs well under one core; a reading over 60% means Cinema should get shorter responses.
8. **Measured responses (3.2.1).** 48 kHz WAVs (14 channels in HeSuVi's order, or 16 in Bloob's) copied to `/media/internal/com.muhib.bloob/headphones/` appear as extra Room choices named after the file (`dolby-atmos.wav` → *Dolby Atmos*), in Settings and in the player's Audio panel. A file Bloob can't use is logged with the reason (`[sound] headphone response … not usable`).

Previews to hear before installing (phone and the same earbuds): `build-host/hp_preview <dir> <film>` writes a 7.1 speaker walk and the film excerpt through each room, plus the TV's plain downmix for comparison.
9. **Bluetooth quality (SBC).** `native/scripts/bt-sbc.sh`, copied to the TV and run as root with the earbuds playing: no argument shows the SBC settings the TV and earbuds agreed; `max` raises the bitpool to the earbuds' maximum; `xq` asks for SBC XQ (dual channel, bitpool 47 per channel, about 600 kb/s). Nothing is written to the TV; reconnecting the earbuds undoes it. Compare by ear and note which you keep.

## 18. New in 3.3: Calibrate

Settings → **Calibrate** (near the bottom, above About). It needs the Spyder5 plugged into the TV and hung flat in the middle of the screen, the Spyder probe app still installed (Bloob uses its copy of Argyll's `spotread` and LG's signed test manifest), and the 55G2 correction at `/media/internal/com.muhib.bloob/calibration/argyll/lg-55g2.ccss`. Bloob starts `bin/bloob-cal` as root through the Homebrew Channel; it runs on if Bloob is closed and writes `/media/internal/com.muhib.bloob/calibration/status.json`, which the screen shows. Its full output is in `calibration/last-run.log`.

1. **Check** (about 3 minutes, changes nothing). The first time, the TV asks to allow Bloob: press OK. A white square appears in the middle of a black screen. The screen afterwards shows ten white readings: the spread (steadiness) should be under 1%, and `1019 is …x 940` says whether SDR patterns are limited range (about 1.0×) or full range (about 1.2×). If no square appears, the pattern route is wrong: note it and stop.
2. **Test on Expert 2** (about 10 minutes). The TV switches to Expert 2; red darkens on purpose; at the end Expert 2 is back to factory and the TV back in Expert 1. Expect *7 passed, 0 failed*. The order line says whether the 3D LUT acts before the 1D LUT (expected).
3. **Calibrate Expert 1** (3.3.4: while any run goes, Bloob's screen turns see-through with a small card in the bottom left, so LG's squares show; each run first checks that a white square really reaches the meter and stops, changing nothing, if not. 3.3.3: it first turns off energy saving, dynamic contrast, peak brightness, logo dimming and AI Picture/Brightness, which stay off afterwards, and starts from LG's factory calibration data; the log lists each change. OK twice; 1½ to 2 hours, about 1,900 readings, close to Calman's AutoCal; 3.3.1). *Calibrate Expert 1, thorough* is the 3.3.0 run (3 to 4 hours, about 3,900 readings) for the last bit of accuracy. Leave the room as it is when you watch (the lamp on is fine; no light on the screen itself). The screen shows the squares the whole time. Back in Bloob afterwards, Calibrate shows *Grey before → after · Colour before → after* and *As you watch it*. Note all four numbers. Watch a few familiar films.
4. **Stop** during a run: the run ends within a reading or two, Expert 1 goes back to factory and your settings, and the screen says *stopped*.
5. **Undo calibration** (OK twice): Expert 1 back to LG's factory tables and the settings saved before the first calibration (`calibration/expert1-settings.json`).
6. If anything looks wrong, copy `calibration/status.json` and `calibration/last-run.log`. They hold no keys (the TV pairing key is in `calibration/ssap-key.json`, mode 600; don't share that one).
