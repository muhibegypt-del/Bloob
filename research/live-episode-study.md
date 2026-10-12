# Elements of A: paid-episode runtime observation

## Scope and method

On 12 October 2026 the user started an episode in Elements of A on a rooted LG webOS TV. The investigation read process metadata and `/proc` counters over SSH. The user then paused for roughly 30 seconds and resumed while a local sampler recorded 30 observations at two-second intervals. No TV files were changed or deleted. Account data, media URLs, and protected-content traffic were not captured. The raw process-stat sample remains local and is excluded from Git because it contains the executable's original process name.

## Direct observations

| Signal | Observation |
| --- | --- |
| TV | Linux 5.4.268-320, aarch64 |
| App process | PID `4525`, launched before the sample; 74 threads during playback |
| Resident memory | 368,440–374,148 kB across five samples from 19:28:03 to 19:28:11 BST |
| Process CPU shown by `ps` | 41.0% falling to 39.9% over those samples; this is a process-lifetime average, not instantaneous playback cost |
| Active thread names | `SourceBuffer: v`, `SourceBuffer: a`, `video-queue:src`, `video-app-es:sr`, `video-sink-Queu`, `audio-queue:src`, `audio-app-es:sr`, `audio-sink-Queu`, `lxvideodec3:src`, `PlayerControlle`, plus UI, networking, image, and GPU-related workers |
| Separate system process | `starfish-media-pipeline` was PID `3201` at roughly 10 MB RSS and a `ps` lifetime CPU display of 0.0%; association with this episode was not established |

The app had separate video and audio source-buffer and queue threads throughout the inspected playback period. Earlier playback/exit comparison in the [system map](Elements-of-A-system-map.md) showed that these threads and selected media descriptors disappeared after leaving the player. Their presence alone does not identify the rendition, container, audio format, output mode, or actual first-frame event.

## Pause and resume comparison

The local sampler recorded `/proc/4525/stat` plus the video source-buffer and sink thread counters every two seconds from 19:30:37 to 19:31:35 BST. Process CPU ticks per two-second interval were mostly 7–10 from 19:30:45 through 19:30:59, then 27–84 from 19:31:03 onward. This rise aligns with the user's reported pause/resume action and is consistent with substantially more process work after resuming. A few transition intervals were higher or lower. The sampled source-buffer thread stayed near zero CPU ticks in both windows; the video-sink thread showed occasional 1–2 tick increments after the rise. Thread names remained allocated during the pause window.

This is a **correlation**, not a verified timestamped playback-state transition: the TV did not expose an internal pause callback or the exact remote-key timestamp in this capture. A broad CPU percentage cannot establish decode cost or dropped frames. The observations support a retained player session that becomes less active while paused, pending a direct state trace.

## Capture and control gaps

- SSH process inspection works. The TV host identity was checked against an existing trusted SSH fingerprint before connecting.
- TV shell exposes a capture service (`executeOneShot`) and a network-input service (`getPointerInputSocket`, media play/pause/seek controls). Their interfaces were read; a working client session was not established.
- A read-only foreground-app Luna query returned no response through the TV shell. One media pause service call also returned no response, so its effect is unknown. Do not count it as a successful remote-control test.
- The TV's second-screen WebSocket port is reachable, but pairing, pointer input, and screenshots have not been validated. These may require an on-screen prompt and must not be assumed to work.
- No screen recording, internal player state callback, seek/track UI interaction, selected rendition, buffer depth, output refresh rate, or first-frame timing was obtained from this run.

## Next controlled capture

Establish a paired, least-privilege remote-control client and a capture route without storing a TV control key in Git. Drive one documented sequence: play, show controls, pause, resume, single seek, repeated scrub, open audio/subtitle menus, Back, and exit. Record each input timestamp, observable UI state, `/proc` media-thread and RSS deltas, and any public output-mode or player metrics. Keep account values and full stream URLs out of logs. Distinguish visible behavior from static code paths in the result.
