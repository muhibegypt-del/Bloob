# Bloob as the TV's home screen

Two small scripts, adapted from [Lumen](https://github.com/samrenfrew/lumen-webos) (MIT, `LICENSE-lumen`), make Bloob what you see at power-on and on the Home button:

- `60-bloob-home`: a Homebrew Channel boot hook that starts the watcher.
- `watch.sh`: once a second, if LG's own Home comes to the front, opens Bloob instead. After three failed launches it leaves LG Home alone, so the TV is never stuck. About a minute after boot it starts Bloob hidden, so the first Home press is quick.

When the hook file exists, Back on Bloob's Home no longer offers to close the app.

## Install (run from your computer; adds files, deletes nothing)

Bloob 2.6 must already be installed. Replace `TV` with the TV's address.

```
scp native/launcher/watch.sh root@TV:/tmp/ && scp native/launcher/60-bloob-home root@TV:/tmp/
ssh root@TV 'mkdir -p /var/lib/bloob-home && mv /tmp/watch.sh /var/lib/bloob-home/ && chmod 755 /var/lib/bloob-home/watch.sh \
  && mv /tmp/60-bloob-home /var/lib/webosbrew/init.d/ && chmod 755 /var/lib/webosbrew/init.d/60-bloob-home \
  && /var/lib/webosbrew/init.d/60-bloob-home'
```

The last command starts the watcher now; after that it starts at every boot. Restart Bloob once so it sees the hook. What the watcher did is in `/tmp/bloob-home.log`.

## Options

- No hidden start at boot: `touch /var/lib/bloob-home/no-boot-preload`.

## Undo (your call; it removes only these files)

```
ssh root@TV 'rm /var/lib/webosbrew/init.d/60-bloob-home; killall watch.sh'
```
