COMMON_FILTERS = aresample scale crop overlay
COMMON_DEMUXERS = matroska ogg avi mov flv mpegps image2 mp3 concat
COMMON_DECODERS = \
	vp8 vp9 theora \
	mpeg2video mpeg4 h264 hevc \
	png mjpeg \
	vorbis opus \
	mp3 ac3 aac \
	ass ssa srt webvtt

MP4_MUXERS = mp4 mp3 null
MP4_ENCODERS = libx264 libmp3lame aac
MP4_PC_PATH = ../x264/dist/lib/pkgconfig
MP4_SHARED_LIBS = \
	lib/lame-3.100/dist/lib/libmp3lame.so \
	lib/x264/dist/lib/libx264.so

FFMPEG_COMMON_ARGS = \
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
	\
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
	$(addprefix --enable-decoder=,$(COMMON_DECODERS)) \
	$(addprefix --enable-demuxer=,$(COMMON_DEMUXERS)) \
	--enable-protocol=file \
	$(addprefix --enable-filter=,$(COMMON_FILTERS)) \
	--disable-bzlib \
	--disable-iconv \
	--disable-libxcb \
	--disable-lzma \
	--disable-securetransport \
	--disable-xlib \
	--disable-zlib

PRE_JS = src/shell-pre.js
POST_JS = src/shell-post.js

EMCC_COMMON_ARGS=\
	--closure 1 \
	-O3 \
	-s MODULARIZE \
	-s NO_INVOKE_RUN \
	-s ALLOW_MEMORY_GROWTH \
	-s ENVIRONMENT=web \
	-s EXPORT_NAME=ffmpeg

all: mp4

docker-build:
	docker build -t ffmpeg-wasm .

build: docker-build
	docker run --rm -v `pwd`/dist:/app/dist ffmpeg-wasm /bin/sh -c "cp -R /app/out/* /app/dist/"

clean: clean-x264 clean-lame
	rm -rf dist

clean-x264:
	cd lib/x264 && \
	rm -rf dist && \
	make clean

lib/x264/dist/lib/libx264.so:
	cd lib/x264 && \
	git reset --hard && \
	patch -p1 < ../x264-configure.patch && \
	emconfigure ./configure \
	  --prefix="$$(pwd)/dist" \
	  --extra-cflags="-Wno-unknown-warning-option" \
	  --host=x86-none-linux \
	  --disable-cli \
	  --enable-shared \
	  --disable-opencl \
	  --disable-thread \
	  --disable-asm \
	  \
	  --disable-avs \
	  --disable-swscale \
	  --disable-lavf \
	  --disable-ffms \
	  --disable-gpac \
	  --disable-lsmash \
	  && \
	emmake make -j8 && \
	emmake make install

clean-lame:
	rm lib/lame.tar.gz && \
	rm -rf lib/lame-3.100

lib/lame-3.100/dist/lib/libmp3lame.so:
	curl -s -L http://downloads.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz -o lib/lame.tar.gz && \
	tar xfz lib/lame.tar.gz -C lib && \
	cd lib/lame-3.100 && \
	emconfigure ./configure \
	  --prefix="$$(pwd)/dist" \
	  --host=x86-none-linux \
	  --disable-static \
	  \
	  --disable-gtktest \
	  --disable-analyzer-hooks \
	  --disable-decoder \
	  --disable-frontend && \
	emmake make -j8 && \
	emmake make install

configure-mp4: $(MP4_SHARED_LIBS)
	cd lib/ffmpeg-mp4 && \
	git reset --hard && \
	EM_PKG_CONFIG_PATH=$(MP4_PC_PATH) emconfigure ./configure \
	  $(FFMPEG_COMMON_ARGS) \
	  $(addprefix --enable-encoder=,$(MP4_ENCODERS)) \
	  $(addprefix --enable-muxer=,$(MP4_MUXERS)) \
	  --enable-libmp3lame \
	  --enable-gpl \
	  --enable-libx264 \
	  --extra-cflags="-I../lame-3.100/dist/include -I../x264/dist/include" \
	  --extra-ldflags="-L../lame-3.100/dist/lib -L../x264/dist/lib" && \
	emmake make -j8 && \
	cp ffmpeg ffmpeg.bc

mp4: configure-mp4
	mkdir out
	emcc lib/ffmpeg-mp4/ffmpeg.bc $(MP4_SHARED_LIBS) $(EMCC_COMMON_ARGS) -o out/ffmpeg-mp4.js
	emcc lib/ffmpeg-mp4/ffmpeg.bc $(MP4_SHARED_LIBS) $(EMCC_COMMON_ARGS) -s EXPORT_ES6 -o out/ffmpeg-mp4.esm.js

