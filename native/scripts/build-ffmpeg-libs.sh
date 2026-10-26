#!/usr/bin/env bash
# FFmpeg libraries for Bloob's own playback engine (3.0): the demuxer and the
# audio converter link them statically. Same FFmpeg 7.1.1 source and LGPL-only
# rules as build-ffmpeg.sh (no GPL parts, no external libraries).
#   native/scripts/build-ffmpeg-libs.sh arm|host
# Audio filters (3.2): Headphones mode's binaural chain (headphone, EQ, limiter).
# Output: native/build-ffmpeg/<target>/{include,lib} (not in Git; build.sh calls this when missing).
set -euo pipefail
cd "$(dirname "$0")/.."
TARGET=${1:?arm or host}
SRC=build-ffmpeg/src
if [ ! -d "$SRC" ]; then
  git clone -q --depth 1 --branch n7.1.1 https://github.com/FFmpeg/FFmpeg.git "$SRC"
fi
PREFIX=$(realpath -m "build-ffmpeg/$TARGET")
OBJ=$(realpath -m "build-ffmpeg/obj-libs-$TARGET")
rm -rf "$OBJ" "$PREFIX" && mkdir -p "$OBJ"
CROSS=()
if [ "$TARGET" = arm ]; then
  SDK=${WEBOS_SDK:-/opt/arm-webos-linux-gnueabi_sdk-buildroot}
  CROSS=(--enable-cross-compile --arch=arm --cpu=cortex-a9 --target-os=linux --cross-prefix="$SDK/bin/arm-webos-linux-gnueabi-")
fi
cd "$OBJ"
"$(realpath "$OLDPWD/$SRC")"/configure "${CROSS[@]}" --prefix="$PREFIX" \
  --pkg-config=false --disable-autodetect --disable-iconv --enable-pic \
  --enable-static --disable-shared --disable-programs --disable-doc --disable-debug --enable-small \
  --disable-everything --disable-avdevice --disable-swscale --disable-postproc \
  --enable-network --enable-protocol=http,tcp,file \
  --enable-demuxer=matroska,mov,mpegts,avi \
  --enable-decoder=dca,ac3,eac3,truehd,mlp,aac,aac_latm,flac,mp3,opus,vorbis,pcm_s16le,pcm_s24le,pcm_s16be,pcm_s24be,pcm_bluray,pcm_dvd,subrip,ass,ssa,webvtt,mov_text,pgssub \
  --enable-encoder=eac3,ac3 \
  --enable-parser=dca,ac3,aac,aac_latm,mlp,flac,mpegaudio,opus,vorbis,h264,hevc,av1,vp9 \
  --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb,null \
  --enable-filter=abuffer,abuffersink,aformat,aresample,anull,pan,headphone,equalizer,lowshelf,highshelf,volume,alimiter,acompressor \
  --enable-swresample
make -j"$(nproc)" >/dev/null
make install >/dev/null
ls "$PREFIX/lib"
