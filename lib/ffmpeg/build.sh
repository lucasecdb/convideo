#!/bin/bash

set -e

export OPTIMIZE="-Os"
export PREFIX="/src/build"
export CFLAGS="${OPTIMIZE} -I${PREFIX}/include/"
export CPPFLAGS="${OPTIMIZE} -I${PREFIX}/include/"
export LDFLAGS="${OPTIMIZE} -L${PREFIX}/lib/"

apt-get update
apt-get install -qqy pkg-config

FILTERS=aresample,crop,overlay,scale
DEMUXERS=avi,concat,flv,image2,matroska,mov,mp3,mpegps,ogg
MUXERS=avi,flv,image2,matroska,mov,mp4,mp3,null,ogg
ENCODERS=aac,ac3,libx264,libmp3lame
DECODERS="aac,\
ac3,\
ass,\
h264,\
hevc,\
mjpeg,\
mp3,\
mpeg2video,\
mpeg4,\
opus,\
png,\
srt,\
ssa,\
theora,\
vorbis,\
vp8,\
vp9,\
webvtt"

echo "=========================="
echo "Compiling libx264"
echo "=========================="
test -n "$SKIP_BUILD" || (
  cd node_modules/libx264
  if ! patch -R -s -f -p1 --dry-run < ../../x264-configure.patch; then
    patch -p1 < ../../x264-configure.patch
  fi
  emconfigure ./configure \
    --prefix=${PREFIX}/ \
    --extra-cflags="-Wno-unknown-warning-option" \
    --host=x86-none-linux \
    --disable-cli \
    --enable-shared \
    --disable-opencl \
    --disable-thread \
    --disable-asm \
    --disable-avs \
    --disable-swscale \
    --disable-lavf \
    --disable-ffms \
    --disable-gpac \
    --disable-lsmash
  emmake make -j8
  emmake make install
)
echo "=========================="
echo "Compiling libx264 done"
echo "=========================="

echo "=========================="
echo "Compiling lame"
echo "=========================="
test -n "$SKIP_BUILD" || (
  cd node_modules/libmp3lame
  emconfigure ./configure \
    --prefix=${PREFIX}/ \
    --host=x86-none-linux \
    --disable-static \
    \
    --disable-gtktest \
    --disable-analyzer-hooks \
    --disable-decoder \
    --disable-frontend
  emmake make -j8
  emmake make install
)
echo "=========================="
echo "Compiling lame done"
echo "=========================="

echo "=========================="
echo "Compiling zlib"
echo "=========================="
test -n "$SKIP_BUILD" || (
  cd node_modules/zlib
  emconfigure ./configure \
    --prefix=${PREFIX}/
  emmake make -j8
  emmake make install
)
echo "=========================="
echo "Compiling zlib done"
echo "=========================="

echo "=========================="
echo "Compiling ffmpeg"
echo "=========================="
test -n "$SKIP_BUILD" || (
  cd node_modules/ffmpeg
  EM_PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig emconfigure ./configure \
    --prefix=${PREFIX}/ \
    --cc=emcc \
    --enable-cross-compile \
    --target-os=none \
    --arch=x86 \
    --disable-runtime-cpudetect \
    --disable-asm \
    --disable-fast-unaligned \
    --disable-pthreads \
    --disable-w32threads \
    --disable-os2threads \
    --disable-debug \
    --disable-stripping \
    --disable-all \
    --enable-avcodec \
    --enable-avformat \
    --enable-avutil \
    --enable-swresample \
    --enable-swscale \
    --enable-avfilter \
    --disable-network \
    --disable-d3d11va \
    --disable-dxva2 \
    --disable-vaapi \
    --disable-vdpau \
    $(eval echo --enable-decoder={$DECODERS}) \
    $(eval echo --enable-encoder={$ENCODERS}) \
    $(eval echo --enable-demuxer={$DEMUXERS}) \
    $(eval echo --enable-muxer={$MUXERS}) \
    --enable-protocol=file \
    $(eval echo --enable-filter={$FILTERS}) \
    --disable-bzlib \
    --disable-iconv \
    --disable-libxcb \
    --disable-lzma \
    --disable-securetransport \
    --disable-xlib \
    --enable-libmp3lame \
    --enable-gpl \
    --enable-libx264 \
    --enable-zlib \
    --extra-cflags="-I ${PREFIX}/include" \
    --extra-ldflags="-L ${PREFIX}/lib"
  emmake make -j8
  emmake make install
)
echo "=========================="
echo "Compiling ffmpeg done"
echo "=========================="

EMSCRIPTEN_COMMON_ARGS="--bind \
  ${OPTIMIZE} \
  --closure 1 \
  -s MODULARIZE=1 \
  -s INVOKE_RUN=0 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORT_NAME=\"ffmpeg\" \
  -s ASSERTIONS=1 \
  --std=c++11 \
  -I ${PREFIX}/include \
  -L ${PREFIX}/lib \
  ffmpeg.cpp"

echo "=========================="
echo "Generating bidings"
echo "=========================="
(
  emcc $EMSCRIPTEN_COMMON_ARGS -o ffmpeg.js

  if [ $ASM ]; then
    emcc $EMSCRIPTEN_COMMON_ARGS -s WASM=0 -o ffmpeg.asm.js
  fi
)
echo "=========================="
echo "Generating bidings done"
echo "=========================="
