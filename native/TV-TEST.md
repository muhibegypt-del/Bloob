# Bloob 1.1 on the TV: test script

Run this script on the OLED55G26LA from the sofa, at normal viewing distance (about 2.5 m), with the LG remote only. Write down what happens. Do not write down what should have happened. Do not photograph account screens or PINs.

## Install

```bat
cd /d %USERPROFILE%\Downloads
ares-install -d rootedTV com.muhib.bloob_1.1.0_arm.ipk
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
