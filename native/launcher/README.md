# Bloob as the TV's home screen

Two small scripts, adapted from [Lumen](https://github.com/samrenfrew/lumen-webos) (MIT, `LICENSE-lumen`), make Bloob what you see at power-on and on the Home button. It is the same mechanism other launchers for rooted LG TVs use (a Homebrew Channel boot hook plus a watcher), installed once from your computer:

- `60-bloob-home`: a Homebrew Channel boot hook that starts the watcher at every boot.
- `watch.sh`: once a second, if LG's own Home comes to the front, opens Bloob instead. After three failed launches it leaves LG Home alone, so the TV is never stuck. About a minute after boot it starts Bloob hidden, so the first Home press is quick.
- `install.sh`: puts the two in place (only what changed) and starts the watcher. Adds files; deletes nothing.

When the hook exists, Back on Bloob's Home no longer offers to close the app.

## Setup (once, after installing the Bloob IPK)

Mac, Linux, or Windows with Git Bash or WSL, from the repository folder (`TV` = the TV's address):

```
sh native/launcher/setup-tv.sh TV
```

Windows PowerShell, from `native/launcher`:

```
ssh root@TV "mkdir -p /tmp/bloob-launcher"
scp watch.sh 60-bloob-home install.sh root@TV:/tmp/bloob-launcher/
ssh root@TV "sh /tmp/bloob-launcher/install.sh"
```

Then restart Bloob once. What the watcher did is in `/tmp/bloob-home.log` on the TV. Keep LG's automatic firmware updates off: an update that removes root stops the hook, and the TV simply goes back to LG's launcher.

## Options

- No hidden start at boot: `touch /var/lib/bloob-home/no-boot-preload`.

## Undo (your call; it removes only these files)

```
ssh root@TV "rm /var/lib/webosbrew/init.d/60-bloob-home; killall watch.sh"
```

## Becoming LG Home (4.0, optional)

The watcher replaces LG Home after it appears, so LG Home can flash up first. With the
takeover, Bloob **is** LG Home: the Home button and power-on open Bloob directly, and
opening the Bloob tile brings that same Bloob forward. The method is Luma's (ecklf/lg-luma,
MIT, `LICENSE-luma`): `takeover.sh` stages Bloob's program and files under LG Home's id in
`/var/lib/bloob-home/app` and bind-mounts that over `/usr/palm/applications/com.webos.app.home`,
then has webOS read Home's manifest again. LG's own files stay untouched underneath.

Turn it on from your computer (after installing the Bloob IPK and the setup above):

```
sh native/launcher/setup-tv.sh TV --takeover
```

Turn it off: `sh native/launcher/setup-tv.sh TV --no-takeover`, or on the TV
`sh /var/lib/bloob-home/takeover.sh disable`. Status: `sh /var/lib/bloob-home/takeover.sh status`.

Safety:
- If Bloob is not on screen as Home within 15 seconds of switching on, everything is undone
  and LG Home is opened.
- A restart always drops the mount. The boot hook (`62-bloob-takeover`) applies it again only
  while the takeover is on, and stops after two restarts in a row in which Bloob never started
  as Home (Bloob marks each good start). So switching the TV off and on twice always brings
  LG Home back.
- Nothing is deleted; turning it off leaves the files, marked off.
- The watcher stays: it still replaces an HDMI input or Live TV at power-on, and leaves Home
  alone while the takeover is on.

Not yet seen on the owner's TV (webOS 9): whether webOS keeps Bloob running between Home
presses or starts it again each time (Luma reports both, by firmware). The log is
`/var/lib/bloob-home/takeover.log`; Bloob's own log says "started as the TV's Home".
Desktop test: `sh tools/takeover-sim/test.sh` (14 checks against a stand-in TV).
