#!/bin/bash

set -e

export OPTIMIZE="-O3"
export PREFIX="/src/build"
export CFLAGS="${OPTIMIZE} -I${PREFIX}/include/"
export CPPFLAGS="${OPTIMIZE} -I${PREFIX}/include/"
export LDFLAGS="${OPTIMIZE} -L${PREFIX}/lib/"

apt-get update
apt-get install -qqy pkg-config

COMMON_FILTERS=aresample,scale,crop,overlay
COMMON_DEMUXERS=matroska,ogg,avi,mov,flv,mpegps,image2,mp3,concat
COMMON_DECODERS="vp8,vp9,theora,\
mpeg2video,mpeg4,h264,hevc,\
png,mjpeg,\
vorbis,opus,\
mp3,ac3,aac,\
ass,ssa,srt,webvtt"

MP4_MUXERS=mp4,mp3,null
MP4_ENCODERS=libx264,libmp3lame,aac

echo "=========================="
echo "Compiling libx264"
echo "=========================="
(
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
(
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
echo "Compiling ffmpeg"
echo "=========================="
(
  cd node_modules/ffmpeg
  EM_PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig emconfigure ./configure \
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
    --enable-ffmpeg \
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
    $(eval echo --enable-decoder={$COMMON_DECODERS}) \
    $(eval echo --enable-demuxer={$COMMON_DEMUXERS}) \
    --enable-protocol=file \
    $(eval echo --enable-filter={$COMMON_FILTERS}) \
    --disable-bzlib \
    --disable-iconv \
    --disable-libxcb \
    --disable-lzma \
    --disable-securetransport \
    --disable-xlib \
    --disable-zlib \
    $(eval echo --enable-encoder={$MP4_ENCODERS}) \
    $(eval echo --enable-muxer={$MP4_MUXERS}) \
    --enable-libmp3lame \
    --enable-gpl \
    --enable-libx264 \
    --extra-cflags="-I ${PREFIX}/include" \
    --extra-ldflags="-L ${PREFIX}/lib"
  emmake make -j8
  cp ffmpeg ffmpeg.bc
)
echo "=========================="
echo "Compiling ffmpeg done"
echo "=========================="

echo "=========================="
echo "Generating bidings"
echo "=========================="
(
  emcc \
    ${OPTIMIZE} \
    --closure 1 \
    -s MODULARIZE=1 \
    -s INVOKE_RUN=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s 'EXPORT_NAME="ffmpeg"' \
    -o ffmpeg.js \
    ${PREFIX}/lib/libmp3lame.so \
    ${PREFIX}/lib/libx264.so \
    node_modules/ffmpeg/ffmpeg.bc
)
echo "=========================="
echo "Generating bidings done"
echo "=========================="