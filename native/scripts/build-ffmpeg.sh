#!/usr/bin/env bash
# The audio bridge's ffmpeg and ffprobe for the TV (3.0): a minimal, static,
# LGPL build of FFmpeg 7.1.1 with only what Bloob needs:
#   - read: Matroska, MP4, MPEG-TS over plain HTTP from the service (no TLS: the
#     service proxies AllDebrid's HTTPS, so ffmpeg never sees the account or link);
#   - decode DTS (and the formats it probes); encode Dolby Digital (Plus);
#   - write Matroska to a pipe, copying video and subtitles untouched.
# No GPL parts and no external libraries. Output: native/third_party/ffmpeg/bin/.
#   native/scripts/build-ffmpeg.sh [source dir]   (default: clones n7.1.1)
set -euo pipefail
cd "$(dirname "$0")/.."
SDK=${WEBOS_SDK:-/opt/arm-webos-linux-gnueabi_sdk-buildroot}
OUT=third_party/ffmpeg/bin
SRC=${1:-build-ffmpeg/src}
if [ ! -d "$SRC" ]; then
  git clone -q --depth 1 --branch n7.1.1 https://github.com/FFmpeg/FFmpeg.git "$SRC"
fi
BUILD=$(realpath -m build-ffmpeg/obj)
rm -rf "$BUILD" && mkdir -p "$BUILD" "$OUT"
cd "$BUILD"
"$(realpath "$OLDPWD/$SRC")"/configure \
  --enable-cross-compile --arch=arm --cpu=cortex-a9 --target-os=linux \
  --cross-prefix="$SDK/bin/arm-webos-linux-gnueabi-" \
  --pkg-config=false --disable-autodetect --disable-iconv \
  --enable-static --disable-shared --extra-ldflags=-static \
  --disable-doc --disable-debug --enable-small --disable-ffplay --enable-ffmpeg --enable-ffprobe \
  --disable-everything \
  --enable-network --enable-protocol=http,tcp,file,pipe \
  --enable-demuxer=matroska,mov,mpegts,dts,dtshd,truehd,ac3,eac3,aac \
  --enable-muxer=matroska,null \
  --enable-decoder=dca,ac3,eac3,truehd,mlp,aac,flac,pcm_s16le,pcm_s24le,pcm_s16be,pcm_s24be,pcm_bluray \
  --enable-encoder=ac3,eac3,pcm_s16le,pcm_s24le \
  --enable-parser=dca,ac3,aac,mlp,h264,hevc,av1,vc1,mpegvideo \
  --enable-bsf=null \
  --enable-filter=abuffer,abuffersink,aformat,aresample,anull,atrim,asetpts,buffer,buffersink,null,format,copy \
  --enable-swresample
make -j"$(nproc)" ffmpeg ffprobe >/dev/null
"$SDK/bin/arm-webos-linux-gnueabi-strip" ffmpeg ffprobe
cd - >/dev/null
cp "$BUILD/ffmpeg" "$BUILD/ffprobe" "$OUT/"
ls -la "$OUT"
