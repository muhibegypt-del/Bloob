# Buna native UI preview

This is Bloob's first original native UI path. It uses the TV's Qt Quick/QML
runner, not the web engine. `BunaNavigation.js` owns remote focus and page state;
`main.qml` renders that state. It is installed under a separate app ID so the
working web Bloob package remains available.

The current preview covers launch, remote navigation, native artwork surfaces,
a details page, live Cinemeta catalog rows, source search and resolution through
the existing local Bloob service, and a native Qt Multimedia playback path. A
small fixed catalog remains as a fallback. The native UI keeps media URLs only
in memory; the local service retains the paired account key.

This is still a preview. Episode selection currently starts the first regular
episode, and the existing web app's local service must be running. Playback of
resolved formats, track selection, settings, and production lifecycle behavior
need further integration and TV validation.

Package with `ares-package native-app -o tv-app/artifacts` from the repository
root. Install and launch `com.muhib.bloob.native` on the rooted TV. Keep all
existing TV applications and their data intact.

For a direct test of the bundled clip, launch with
`ares-launch com.muhib.bloob.native -p bunaDemo=true -d rootedTV`.
The test clip loops and the player shows its media status, video/audio detection,
buffer progress, timing, and error code during this preview.

To inspect source search without navigating from the home screen, launch with
`ares-launch com.muhib.bloob.native -p bunaSources=true -d rootedTV`.
