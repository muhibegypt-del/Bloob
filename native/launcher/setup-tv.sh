#!/bin/sh
# One-time setup, run from your computer: makes Bloob the TV's home screen.
#   native/launcher/setup-tv.sh <TV address>
# Copies watch.sh, 60-bloob-home and install.sh to the TV over SSH (root, as the Homebrew
# Channel set up) and runs install.sh there. Adds files on the TV; deletes nothing.
# Safe to run again (after a Bloob update, say): it only replaces what changed.
set -e
TV=${1:?usage: setup-tv.sh <TV address>}
cd "$(dirname "$0")"
ssh "root@$TV" 'mkdir -p /tmp/bloob-launcher'
scp watch.sh 60-bloob-home install.sh "root@$TV:/tmp/bloob-launcher/"
ssh "root@$TV" 'sh /tmp/bloob-launcher/install.sh'
echo "Done. Restart Bloob once; from now on the TV starts in Bloob and Home opens it."
