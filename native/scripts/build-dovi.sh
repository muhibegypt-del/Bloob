#!/usr/bin/env bash
# Bloob's Dolby Vision helper (4.0): dovi/ (a few lines of Rust around
# quietvoid's dolby_vision crate, MIT) as a static library the engine links,
# so a profile 7 disc remux plays as profile 8.1 Dolby Vision.
#   native/scripts/build-dovi.sh arm|host
# Output: native/build-dovi/<target>/libbloob_dovi.a (not in Git; build.sh
# calls this when missing). Needs rustup with armv7-unknown-linux-gnueabi.
set -euo pipefail
cd "$(dirname "$0")/.."
TARGET=${1:?arm or host}
OUT=build-dovi/$TARGET
mkdir -p "$OUT"
if [ "$TARGET" = arm ]; then
  SDK=${WEBOS_SDK:-/opt/arm-webos-linux-gnueabi_sdk-buildroot}
  rustup target add armv7-unknown-linux-gnueabi >/dev/null 2>&1 || true
  CARGO_TARGET_ARMV7_UNKNOWN_LINUX_GNUEABI_LINKER="$SDK/bin/arm-webos-linux-gnueabi-gcc" \
    cargo build --quiet --release --locked --manifest-path dovi/Cargo.toml \
      --target armv7-unknown-linux-gnueabi --target-dir build-dovi/target
  cp build-dovi/target/armv7-unknown-linux-gnueabi/release/libbloob_dovi.a "$OUT/"
else
  cargo build --quiet --release --locked --manifest-path dovi/Cargo.toml --target-dir build-dovi/target
  cp build-dovi/target/release/libbloob_dovi.a "$OUT/"
fi
echo "dovi: $OUT/libbloob_dovi.a"
