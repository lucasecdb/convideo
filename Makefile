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

EMCC_COMMON_ARGS=\
	--closure 1 \
	-s TOTAL_MEMORY=67108864 \
	-s OUTLINING_LIMIT=20000 \
	-O3 --memory-init-file 0 \

all: mp4

libx264:
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

libmp3lame:
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

configure-mp4: libmp3lame libx264
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
	emmake make -j8

mp4: configure-mp4
	emcc ffmpeg $(EMCC_COMMON_ARGS) -o ffmpeg-mp4.wasm

