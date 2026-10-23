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
