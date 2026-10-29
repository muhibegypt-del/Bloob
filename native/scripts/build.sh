#!/usr/bin/env bash
# Build, test and package the native Bloob app.
#   native/scripts/build.sh            → artifacts/com.muhib.bloob_<version>_arm.ipk
# Needs the webOS NDK in /opt (openlgtv buildroot-nc4) and ares-package
# (installed with the web app: tv-app/node_modules/.bin/ares-package).
set -euo pipefail
cd "$(dirname "$0")/.."
SDK=${WEBOS_SDK:-/opt/arm-webos-linux-gnueabi_sdk-buildroot}

cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build build-host -j"$(nproc)" >/dev/null
./build-host/native_tests
./build-host/cal_tests | tail -1

cmake -S . -B build-webos -DCMAKE_TOOLCHAIN_FILE="$SDK/share/buildroot/toolchainfile.cmake" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-webos -j"$(nproc)"

# The TV's own libraries must provide everything the binary imports.
if readelf -V build-webos/pkg/bin/bloob | grep -q GLIBCXX; then
  echo "error: binary depends on the NDK's libstdc++" >&2; exit 1
fi
# And no glibc symbol newer than the TV is known to provide (validation A3:
# this was claimed, never enforced). Raise BLOOB_MAX_GLIBC only with TV evidence.
# 2.7 since 3.0, by deduction rather than a TV reading: FFmpeg (the engine's
# demuxer) uses __isoc99_sscanf (2.7) and __sched_cpucount (2.6), symbols from
# 2006-07. The TV's own SDL2 and GLib (which Bloob already links) need 2.9 and 2.12.
MAX_GLIBC=${BLOOB_MAX_GLIBC:-2.7}
NEWEST=$(readelf -V build-webos/pkg/bin/bloob | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -1)
if [ -n "$NEWEST" ] && [ "$(printf '%s\n%s\n' "$NEWEST" "$MAX_GLIBC" | sort -V | tail -1)" != "$MAX_GLIBC" ]; then
  echo "error: binary needs GLIBC_$NEWEST, newer than GLIBC_$MAX_GLIBC:" >&2
  readelf -V build-webos/pkg/bin/bloob | grep -o 'GLIBC_[0-9.]*' | sort -uV >&2
  exit 1
fi
echo "glibc: newest symbol version GLIBC_$NEWEST (limit $MAX_GLIBC)"

# The app's own copy of the Bloob service (catalogue, sources, AllDebrid, artwork).
VERSION=$(node -p "require('./package/appinfo.json').version")
node scripts/stage-service.mjs build-webos/service "$VERSION"
# The audio bridge's ffmpeg and ffprobe (3.0; scripts/build-ffmpeg.sh makes them).
if [ -x third_party/ffmpeg/bin/ffmpeg ] && [ -x third_party/ffmpeg/bin/ffprobe ]; then
  mkdir -p build-webos/service/bin
  install -m 755 third_party/ffmpeg/bin/ffmpeg third_party/ffmpeg/bin/ffprobe build-webos/service/bin/
else
  echo "warning: no ffmpeg in third_party/ffmpeg/bin; DTS-only versions will not play (run scripts/build-ffmpeg.sh)" >&2
fi

ARES=${ARES_PACKAGE:-../tv-app/node_modules/.bin/ares-package}
mkdir -p artifacts
"$ARES" build-webos/pkg build-webos/service -o artifacts

# ares-package stores everything as 0777/0666 and the TV installs it that way.
# Nothing in the IPK may be writable by other users (the service holds the AllDebrid key).
IPK="artifacts/com.muhib.bloob_${VERSION}_arm.ipk"
python3 scripts/fix-ipk-modes.py "$IPK"

# Check what was actually packaged.
CHECK=$(mktemp -d)
(cd "$CHECK" && ar x "$OLDPWD/$IPK" && tar tvzf data.tar.gz) > "$CHECK/list"
if grep -E '^.{5}w|^.{8}w' "$CHECK/list" | grep -q .; then
  echo "error: group- or world-writable entries in $IPK:" >&2
  grep -E '^.{5}w|^.{8}w' "$CHECK/list" >&2
  exit 1
fi
ls -la artifacts
