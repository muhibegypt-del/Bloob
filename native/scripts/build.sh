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

cmake -S . -B build-webos -DCMAKE_TOOLCHAIN_FILE="$SDK/share/buildroot/toolchainfile.cmake" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-webos -j"$(nproc)"

# The TV's own libraries must provide everything the binary imports.
if readelf -V build-webos/pkg/bin/bloob | grep -q GLIBCXX; then
  echo "error: binary depends on the NDK's libstdc++" >&2; exit 1
fi

ARES=${ARES_PACKAGE:-../tv-app/node_modules/.bin/ares-package}
mkdir -p artifacts
"$ARES" build-webos/pkg -o artifacts
ls -la artifacts
