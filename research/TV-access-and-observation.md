# Rooted webOS TV: access, control, and observation guide

This guide lets a later assistant continue the TV study. The owner has already authorized read-only root inspection and explicitly required **no deletion on the TV**. Never delete files, apps, logs, caches, accounts, or packages during reference-app analysis. Keep the TV address, password, keys, client key, account data, and media URLs out of Git and reports. Use the owner's current connection details privately; do not copy them into scripts.

## Connect and identify the host

In PowerShell, assign the address in your local session and connect interactively:

```powershell
$tvHostName = '<TV address or trusted SSH alias>'
ssh-keygen -F $tvHostName -l
ssh "root@$tvHostName"
```

Check the presented host fingerprint against the trusted entry or the TV owner's known fingerprint before accepting a new host key. Use a password prompt or an existing SSH key; do not put a password on a command line, in a repo file, or in shell history. A webOS CLI device alias `rootedTV` was previously configured on this PC; if the CLI is installed, `ares-setup-device --list` can confirm it. The CLI is not currently on this shell's PATH, so locate its installation before relying on it.

Read-only orientation once connected:

```sh
date
uname -a
id
ps -eo pid,ppid,comm,args
```

`ps args` may contain sensitive launch arguments. Inspect on screen, then record only process IDs and generic roles in notes. PID values change between launches. Do not publish the app's original package/process identifier.

## Capture the process and media footprint

Find the foreground app PID from `ps`, then use a temporary shell variable on the TV:

```sh
app_pid='<PID>'
cat "/proc/$app_pid/status" | grep -E '^(Name|State|Threads|VmRSS|VmSize):'
for item in /proc/$app_pid/task/*/comm; do cat "$item"; done
ls -l "/proc/$app_pid/fd"
cat "/proc/$app_pid/stat"
```

Filter thread names and device descriptors before saving or sharing. A descriptor can point to a media URL or private file; copy only known device paths and counts. `/proc/PID/stat` gives CPU ticks and must be sampled at known intervals to compare pause, play, and exit. `ps` CPU percentages are often lifetime averages. Sample the same PID before playback, while playing, while paused, and after Back; note the exact user action time and whether playback was visibly running. The previous study's process RSS and thread changes are documented in [the episode study](live-episode-study.md).

Never use broad log dumps or network captures by default: they can include session tokens, account identifiers, or full stream URLs. If a narrow log read is needed, filter locally and redact before adding a result to research notes. No protected-content data is needed for the requested UI/runtime/media architecture study.

## TV control paths

The reliable control method is the physical remote plus a synchronized observation note: record the exact sequence and timestamps for opening content, showing controls, pause, resume, single seek, repeated scrub, audio/subtitle menu, Back, and exit. Ask the owner to perform an action when a paired remote channel is unavailable. Do not claim an input was sent merely because a service call returned no output.

The TV exposes a webOS network-input service with a `getPointerInputSocket` method, and its second-screen WebSocket port was reachable in an earlier check. A paired second-screen client may be able to send remote input after an on-TV approval prompt. **Pairing and actual pointer/key control have not yet been verified on this TV.** The service's play/pause/seek methods were visible, but one shell pause call returned no response and its effect is unknown. Do not automate playback or claim a successful control channel until an on-screen action is observed and timestamped. Store any pairing key in a local user-protected location outside Git. If testing the second-screen route, begin with a harmless navigation key, verify the visible response, and then run the documented input sequence.

The capture service interface exposed `executeOneShot`, but a screenshot route was not established. Use an owner-provided photo or an approved paired capture client for visual proof. Avoid capturing account pages, PINs, stream URLs, or personal recommendations in shareable artifacts.

## Controlled trace worksheet

For each run, record a compact table with columns: local timestamp, exact remote action, visible UI state, app PID, RSS, thread count, selected media-thread names, selected media device descriptors, and any public player metrics. Take a baseline on a details page, then samples during startup, stable playback, pause, resume, seek, and after exit. Keep a separate column for **static code path expected** versus **TV event observed**. A single run should not be generalized to every format or TV model.

Next high-value measurements: first-frame latency from remote OK to visible image, frame pacing during still UI and animation, app CPU deltas during play/pause, RSS and descriptors after exit, seek target versus actual media clock, audio/subtitle exposure, and output mode where the TV offers a public status signal. This provides concrete requirements for Buna without reusing the reference runtime's private code.
