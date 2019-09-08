#include <emscripten/bind.h>
#include <emscripten/val.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
}

using namespace emscripten;
using std::string;

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;

typedef struct FilteringContext {
  AVFilterContext *buffersink_ctx;
  AVFilterContext *buffersrc_ctx;
  AVFilterGraph *filter_graph;
} FilteringContext;
static FilteringContext *filter_ctx;

typedef struct StreamContext {
  AVCodecContext *dec_ctx;
  AVCodecContext *enc_ctx;
} StreamContext;
static StreamContext *stream_ctx;

struct Options {
  bool verbose;
  string outputFormat;
  string videoEncoder;
  string audioEncoder;
};

static int open_input_file(const char *filename)
{
  int ret;
  unsigned int i;
  ifmt_ctx = NULL;

  if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  stream_ctx = (StreamContext*)av_mallocz_array(ifmt_ctx->nb_streams, sizeof(*stream_ctx));

  if (!stream_ctx)
    return AVERROR(ENOMEM);

  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    AVStream *stream = ifmt_ctx->streams[i];
    AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext *codec_ctx;

    if (!dec) {
      av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
      return AVERROR_DECODER_NOT_FOUND;
    }

    codec_ctx = avcodec_alloc_context3(dec);

    if (!codec_ctx) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
      return AVERROR(ENOMEM);
    }

    ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
           "for stream #%u\n", i);
      return ret;
    }

    /* Reencode video & audio and remux subtitles etc. */
    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
        || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
      /* Open decoder */
      ret = avcodec_open2(codec_ctx, dec, NULL);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
        return ret;
      }
    }
    stream_ctx[i].dec_ctx = codec_ctx;
  }

  av_dump_format(ifmt_ctx, 0, filename, 0);
  return 0;
}

static int open_output_file(string filename,
    string format_name,
    string video_encoder_name,
    string audio_encoder_name)
{
  AVStream *out_stream;
  AVStream *in_stream;
  AVCodecContext *dec_ctx, *enc_ctx;
  AVCodec *encoder;
  int ret;
  unsigned int i;
  ofmt_ctx = NULL;

  avformat_alloc_output_context2(&ofmt_ctx, NULL, format_name.c_str(), filename.c_str());

  if (!ofmt_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
    return AVERROR_UNKNOWN;
  }

  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    out_stream = avformat_new_stream(ofmt_ctx, NULL);

    if (!out_stream) {
      av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
      return AVERROR_UNKNOWN;
    }

    in_stream = ifmt_ctx->streams[i];
    dec_ctx = stream_ctx[i].dec_ctx;

    int codec_type = dec_ctx->codec_type;

    if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
      encoder = avcodec_find_encoder_by_name(codec_type == AVMEDIA_TYPE_VIDEO
          ? video_encoder_name.c_str()
          : audio_encoder_name.c_str()
      );

      if (!encoder) {
        av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
        return AVERROR_INVALIDDATA;
      }

      enc_ctx = avcodec_alloc_context3(encoder);

      if (!enc_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
      }
      /* In this example, we transcode to same properties (picture size,
       * sample rate etc.). These properties can be changed for output
       * streams easily using filters */
      if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        enc_ctx->height = dec_ctx->height;
        enc_ctx->width = dec_ctx->width;
        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;

        // take first available format from list of supported formats
        if (encoder->pix_fmts) {
          enc_ctx->pix_fmt = encoder->pix_fmts[0];
        } else {
          enc_ctx->pix_fmt = dec_ctx->pix_fmt;
        }

        if (encoder->supported_framerates) {
          enc_ctx->framerate = encoder->supported_framerates[0];
        } else {
          enc_ctx->framerate = dec_ctx->framerate;
        }

        enc_ctx->time_base = av_inv_q(enc_ctx->framerate);
      } else {
        enc_ctx->sample_rate = dec_ctx->sample_rate;
        enc_ctx->channel_layout = dec_ctx->channel_layout;
        enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
        /* take first format from list of supported formats */
        enc_ctx->sample_fmt = encoder->sample_fmts[0];
        enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
      }

      if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

      /* Third parameter can be used to pass settings to encoder */
      ret = avcodec_open2(enc_ctx, encoder, NULL);

      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
        return ret;
      }

      ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
        return ret;
      }

      out_stream->time_base = enc_ctx->time_base;
      out_stream->r_frame_rate = in_stream->r_frame_rate;

      stream_ctx[i].enc_ctx = enc_ctx;
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
      av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
      return AVERROR_INVALIDDATA;
    } else {
      /* if this stream must be remuxed */
      ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);

      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
        return ret;
      }

      out_stream->time_base = in_stream->time_base;
    }
  }

  av_dump_format(ofmt_ctx, 0, filename.c_str(), 1);

  if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&ofmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename.c_str());
      return ret;
    }
  }

  /* init muxer, write output file header */
  ret = avformat_write_header(ofmt_ctx, NULL);

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
    return ret;
  }

  return 0;
}

static int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
    AVCodecContext *enc_ctx, const char *filter_spec)
{
  char args[512];
  int ret = 0;
  const AVFilter *buffersrc = NULL;
  const AVFilter *buffersink = NULL;
  AVFilterContext *buffersrc_ctx = NULL;
  AVFilterContext *buffersink_ctx = NULL;
  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();
  AVFilterGraph *filter_graph = avfilter_graph_alloc();

  if (!outputs || !inputs || !filter_graph) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    buffersrc = avfilter_get_by_name("buffer");
    buffersink = avfilter_get_by_name("buffersink");

    if (!buffersrc || !buffersink) {
      av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
      ret = AVERROR_UNKNOWN;
      goto end;
    }

    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
        dec_ctx->time_base.num, dec_ctx->time_base.den,
        dec_ctx->sample_aspect_ratio.num,
        dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
        args, NULL, filter_graph);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
      goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
        NULL, NULL, filter_graph);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
      goto end;
    }

    ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
        (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
        AV_OPT_SEARCH_CHILDREN);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
      goto end;
    }
  } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
    buffersrc = avfilter_get_by_name("abuffer");
    buffersink = avfilter_get_by_name("abuffersink");

    if (!buffersrc || !buffersink) {
      av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
      ret = AVERROR_UNKNOWN;
      goto end;
    }

    if (!dec_ctx->channel_layout)
      dec_ctx->channel_layout =
        av_get_default_channel_layout(dec_ctx->channels);

    snprintf(args, sizeof(args),
        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
        dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
        av_get_sample_fmt_name(dec_ctx->sample_fmt),
        dec_ctx->channel_layout);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
        args, NULL, filter_graph);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
      goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
        NULL, NULL, filter_graph);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
      goto end;
    }

    ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
        (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
        AV_OPT_SEARCH_CHILDREN);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
      goto end;
    }

    ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
        (uint8_t*)&enc_ctx->channel_layout,
        sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
      goto end;
    }

    ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
        (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
        AV_OPT_SEARCH_CHILDREN);

    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
      goto end;
    }
  } else {
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  /* Endpoints for the filter graph. */
  outputs->name     = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx;
  outputs->pad_idx  = 0;
  outputs->next     = NULL;

  inputs->name     = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx  = 0;
  inputs->next     = NULL;

  if (!outputs->name || !inputs->name) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
          &inputs, &outputs, NULL)) < 0)
    goto end;

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
    goto end;

  /* Fill FilteringContext */
  fctx->buffersrc_ctx = buffersrc_ctx;
  fctx->buffersink_ctx = buffersink_ctx;
  fctx->filter_graph = filter_graph;
end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);
  return ret;
}

static int init_filters(void)
{
  const char *filter_spec;
  unsigned int i;
  int ret;
  filter_ctx = (FilteringContext*)av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx));
  if (!filter_ctx)
    return AVERROR(ENOMEM);
  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    filter_ctx[i].buffersrc_ctx  = NULL;
    filter_ctx[i].buffersink_ctx = NULL;
    filter_ctx[i].filter_graph   = NULL;
    if (!(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
        || ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
      continue;
    if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      filter_spec = "null"; /* passthrough (dummy) filter for video */
    else
      filter_spec = "anull"; /* passthrough (dummy) filter for audio */
    ret = init_filter(&filter_ctx[i], stream_ctx[i].dec_ctx,
        stream_ctx[i].enc_ctx, filter_spec);
    if (ret)
      return ret;
  }
  return 0;
}

static int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame) {
  int ret;
  int got_frame_local;
  AVPacket enc_pkt;
  int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
    (ifmt_ctx->streams[stream_index]->codecpar->codec_type ==
     AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;

  if (!got_frame)
    got_frame = &got_frame_local;

  /* encode filtered frame */
  enc_pkt.data = NULL;
  enc_pkt.size = 0;

  av_init_packet(&enc_pkt);

  ret = enc_func(stream_ctx[stream_index].enc_ctx, &enc_pkt,
      filt_frame, got_frame);

  av_frame_free(&filt_frame);

  if (ret < 0)
    return ret;

  if (!(*got_frame))
    return 0;

  /* prepare packet for muxing */
  enc_pkt.stream_index = stream_index;
  av_packet_rescale_ts(
    &enc_pkt,
    stream_ctx[stream_index].enc_ctx->time_base,
    ofmt_ctx->streams[stream_index]->time_base
  );

  /* mux encoded frame */
  ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);

  return ret;
}

static int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{
  int ret;
  AVFrame *filt_frame;

  /* push the decoded frame into the filtergraph */
  ret = av_buffersrc_add_frame_flags(filter_ctx[stream_index].buffersrc_ctx,
      frame, 0);

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
    return ret;
  }

  /* pull filtered frames from the filtergraph */
  while (1) {
    filt_frame = av_frame_alloc();

    if (!filt_frame) {
      ret = AVERROR(ENOMEM);
      break;
    }

    ret = av_buffersink_get_frame(filter_ctx[stream_index].buffersink_ctx,
        filt_frame);

    if (ret < 0) {
      /* if no more frames for output - returns AVERROR(EAGAIN)
       * if flushed and no more frames for output - returns AVERROR_EOF
       * rewrite retcode to 0 to show it as normal procedure completion
       */
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        ret = 0;
      av_frame_free(&filt_frame);
      break;
    }

    filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
    ret = encode_write_frame(filt_frame, stream_index, NULL);

    if (ret < 0)
      break;
  }

  return ret;
}

static int flush_encoder(unsigned int stream_index)
{
  int ret;
  int got_frame;
  if (!(stream_ctx[stream_index].enc_ctx->codec->capabilities &
        AV_CODEC_CAP_DELAY))
    return 0;
  while (1) {
    ret = encode_write_frame(NULL, stream_index, &got_frame);
    if (ret < 0)
      break;
    if (!got_frame)
      return 0;
  }
  return ret;
}

int transcode(string input_filename, string output_filename, struct Options options)
{
  int ret;
  AVPacket packet = { .data = NULL, .size = 0 };
  AVFrame *frame = NULL;
  enum AVMediaType type;
  unsigned int stream_index;
  unsigned int i;
  int got_frame;
  int (*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

  if (options.verbose) {
    av_log_set_level(AV_LOG_DEBUG);
  } else {
    av_log_set_level(AV_LOG_INFO);
  }

  if ((ret = open_input_file(input_filename.c_str())) < 0)
    goto end;
  if ((ret = open_output_file(
          output_filename,
          options.outputFormat,
          options.videoEncoder,
          options.audioEncoder)) < 0)
    goto end;
  if ((ret = init_filters()) < 0)
    goto end;

  /* read all packets */
  while (true) {
    if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
      break;
    stream_index = packet.stream_index;
    type = ifmt_ctx->streams[packet.stream_index]->codecpar->codec_type;
    if (filter_ctx[stream_index].filter_graph) {
      frame = av_frame_alloc();
      if (!frame) {
        ret = AVERROR(ENOMEM);
        break;
      }
      av_packet_rescale_ts(&packet,
                 ifmt_ctx->streams[stream_index]->time_base,
                 stream_ctx[stream_index].dec_ctx->time_base);
      dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 :
        avcodec_decode_audio4;
      ret = dec_func(stream_ctx[stream_index].dec_ctx, frame,
          &got_frame, &packet);
      if (ret < 0) {
        av_frame_free(&frame);
        av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
        break;
      }
      if (got_frame) {
        frame->pts = frame->best_effort_timestamp;
        ret = filter_encode_write_frame(frame, stream_index);
        av_frame_free(&frame);
        if (ret < 0)
          goto end;
      } else {
        av_frame_free(&frame);
      }
    } else {
      /* remux this frame without reencoding */
      av_packet_rescale_ts(&packet,
                 ifmt_ctx->streams[stream_index]->time_base,
                 ofmt_ctx->streams[stream_index]->time_base);
      ret = av_interleaved_write_frame(ofmt_ctx, &packet);
      if (ret < 0)
        goto end;
    }
    av_packet_unref(&packet);
  }
  /* flush filters and encoders */
  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    /* flush filter */
    if (!filter_ctx[i].filter_graph)
      continue;
    ret = filter_encode_write_frame(NULL, i);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
      goto end;
    }
    /* flush encoder */
    ret = flush_encoder(i);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
      goto end;
    }
  }
  av_write_trailer(ofmt_ctx);
end:
  av_packet_unref(&packet);
  av_frame_free(&frame);
  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    avcodec_free_context(&stream_ctx[i].dec_ctx);
    if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
      avcodec_free_context(&stream_ctx[i].enc_ctx);
    if (filter_ctx && filter_ctx[i].filter_graph)
      avfilter_graph_free(&filter_ctx[i].filter_graph);
  }
  av_free(filter_ctx);
  av_free(stream_ctx);
  avformat_close_input(&ifmt_ctx);
  if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    avio_closep(&ofmt_ctx->pb);
  avformat_free_context(ofmt_ctx);
  if (ret < 0)
    av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
  return ret ? 1 : 0;
}

uint8_t* result;

val convert(string video, struct Options options) {
  remove("input");
  remove("output");

  FILE* infile = fopen("input", "wb");
  fwrite(video.c_str(), video.length(), 1, infile);
  fflush(infile);
  fclose(infile);

  transcode("input", "output", options);

  FILE *outfile = fopen("output", "rb");

  fseek(outfile, 0, SEEK_END);
  long fsize = ftell(outfile);

  result = (uint8_t*) malloc(fsize);

  fseek(outfile, 0, SEEK_SET);
  fread(result, fsize, 1, outfile);

  return val(typed_memory_view(fsize, result));
}

void free_result() {
  free(result);
}

std::vector<AVCodecDescriptor> list_codecs() {
  std::vector<AVCodecDescriptor> desc_list;

  const AVCodecDescriptor* curr_desc = NULL;

  while ((curr_desc = avcodec_descriptor_next(curr_desc)) != NULL) {
    desc_list.push_back(*curr_desc);
  }

  return desc_list;
}

string get_avcodec_name(const AVCodecDescriptor& c) {
  return c.name;
}

string get_avcodec_long_name(const AVCodecDescriptor& c) {
  return c.long_name;
}

std::vector<string> get_avcodec_mime_types(const AVCodecDescriptor& c) {
  std::vector<string> types;

  if (!c.mime_types) {
    return types;
  }

  for (auto mime_type = c.mime_types; *mime_type != NULL; mime_type++) {
    types.push_back(string(*mime_type));
  }

  return types;
}

EMSCRIPTEN_BINDINGS(ffmpeg) {
  register_vector<AVCodecDescriptor>("VectorCodecDescriptor");

  register_vector<string>("VectorString");

  enum_<AVMediaType>("AVMediaType")
    .value("AVMEDIA_TYPE_UNKNOWN", AVMEDIA_TYPE_UNKNOWN)
    .value("AVMEDIA_TYPE_VIDEO", AVMEDIA_TYPE_VIDEO)
    .value("AVMEDIA_TYPE_AUDIO", AVMEDIA_TYPE_AUDIO)
    .value("AVMEDIA_TYPE_DATA", AVMEDIA_TYPE_DATA)
    .value("AVMEDIA_TYPE_SUBTITLE", AVMEDIA_TYPE_SUBTITLE)
    .value("AVMEDIA_TYPE_ATTACHMENT", AVMEDIA_TYPE_ATTACHMENT)
    .value("AVMEDIA_TYPE_NB", AVMEDIA_TYPE_NB);

  class_<AVCodecDescriptor>("AVCodecDescriptor")
    .property("id", &AVCodecDescriptor::id)
    .property("type", &AVCodecDescriptor::type)
    .property("name", &get_avcodec_name)
    .property("long_name", &get_avcodec_long_name)
    .property("props", &AVCodecDescriptor::props)
    .property("mime_types", &get_avcodec_mime_types);

  value_object<Options>("Options")
    .field("verbose", &Options::verbose)
    .field("outputFormat", &Options::outputFormat)
    .field("videoEncoder", &Options::videoEncoder)
    .field("audioEncoder", &Options::audioEncoder);

  constant("AV_CODEC_PROP_INTRA_ONLY", AV_CODEC_PROP_INTRA_ONLY);
  constant("AV_CODEC_PROP_LOSSY", AV_CODEC_PROP_LOSSY);
  constant("AV_CODEC_PROP_LOSSLESS", AV_CODEC_PROP_LOSSLESS);
  constant("AV_CODEC_PROP_REORDER", AV_CODEC_PROP_REORDER);
  constant("AV_CODEC_PROP_BITMAP_SUB", AV_CODEC_PROP_BITMAP_SUB);
  constant("AV_CODEC_PROP_TEXT_SUB", AV_CODEC_PROP_TEXT_SUB);

  function("convert", &convert);
  function("free_result", &free_result);
  function("list_codecs", &list_codecs);

#pragma region
  enum_<AVCodecID>("AVCodecID")
    .value("AV_CODEC_ID_NONE", AV_CODEC_ID_NONE)
    .value("AV_CODEC_ID_MPEG1VIDEO", AV_CODEC_ID_MPEG1VIDEO)
    .value("AV_CODEC_ID_MPEG2VIDEO", AV_CODEC_ID_MPEG2VIDEO)
    .value("AV_CODEC_ID_H261", AV_CODEC_ID_H261)
    .value("AV_CODEC_ID_H263", AV_CODEC_ID_H263)
    .value("AV_CODEC_ID_RV10", AV_CODEC_ID_RV10)
    .value("AV_CODEC_ID_RV20", AV_CODEC_ID_RV20)
    .value("AV_CODEC_ID_MJPEG", AV_CODEC_ID_MJPEG)
    .value("AV_CODEC_ID_MJPEGB", AV_CODEC_ID_MJPEGB)
    .value("AV_CODEC_ID_LJPEG", AV_CODEC_ID_LJPEG)
    .value("AV_CODEC_ID_SP5X", AV_CODEC_ID_SP5X)
    .value("AV_CODEC_ID_JPEGLS", AV_CODEC_ID_JPEGLS)
    .value("AV_CODEC_ID_MPEG4", AV_CODEC_ID_MPEG4)
    .value("AV_CODEC_ID_RAWVIDEO", AV_CODEC_ID_RAWVIDEO)
    .value("AV_CODEC_ID_MSMPEG4V1", AV_CODEC_ID_MSMPEG4V1)
    .value("AV_CODEC_ID_MSMPEG4V2", AV_CODEC_ID_MSMPEG4V2)
    .value("AV_CODEC_ID_MSMPEG4V3", AV_CODEC_ID_MSMPEG4V3)
    .value("AV_CODEC_ID_WMV1", AV_CODEC_ID_WMV1)
    .value("AV_CODEC_ID_WMV2", AV_CODEC_ID_WMV2)
    .value("AV_CODEC_ID_H263P", AV_CODEC_ID_H263P)
    .value("AV_CODEC_ID_H263I", AV_CODEC_ID_H263I)
    .value("AV_CODEC_ID_FLV1", AV_CODEC_ID_FLV1)
    .value("AV_CODEC_ID_SVQ1", AV_CODEC_ID_SVQ1)
    .value("AV_CODEC_ID_SVQ3", AV_CODEC_ID_SVQ3)
    .value("AV_CODEC_ID_DVVIDEO", AV_CODEC_ID_DVVIDEO)
    .value("AV_CODEC_ID_HUFFYUV", AV_CODEC_ID_HUFFYUV)
    .value("AV_CODEC_ID_CYUV", AV_CODEC_ID_CYUV)
    .value("AV_CODEC_ID_H264", AV_CODEC_ID_H264)
    .value("AV_CODEC_ID_INDEO3", AV_CODEC_ID_INDEO3)
    .value("AV_CODEC_ID_VP3", AV_CODEC_ID_VP3)
    .value("AV_CODEC_ID_THEORA", AV_CODEC_ID_THEORA)
    .value("AV_CODEC_ID_ASV1", AV_CODEC_ID_ASV1)
    .value("AV_CODEC_ID_ASV2", AV_CODEC_ID_ASV2)
    .value("AV_CODEC_ID_FFV1", AV_CODEC_ID_FFV1)
    .value("AV_CODEC_ID_4XM", AV_CODEC_ID_4XM)
    .value("AV_CODEC_ID_VCR1", AV_CODEC_ID_VCR1)
    .value("AV_CODEC_ID_CLJR", AV_CODEC_ID_CLJR)
    .value("AV_CODEC_ID_MDEC", AV_CODEC_ID_MDEC)
    .value("AV_CODEC_ID_ROQ", AV_CODEC_ID_ROQ)
    .value("AV_CODEC_ID_INTERPLAY_VIDEO", AV_CODEC_ID_INTERPLAY_VIDEO)
    .value("AV_CODEC_ID_XAN_WC3", AV_CODEC_ID_XAN_WC3)
    .value("AV_CODEC_ID_XAN_WC4", AV_CODEC_ID_XAN_WC4)
    .value("AV_CODEC_ID_RPZA", AV_CODEC_ID_RPZA)
    .value("AV_CODEC_ID_CINEPAK", AV_CODEC_ID_CINEPAK)
    .value("AV_CODEC_ID_WS_VQA", AV_CODEC_ID_WS_VQA)
    .value("AV_CODEC_ID_MSRLE", AV_CODEC_ID_MSRLE)
    .value("AV_CODEC_ID_MSVIDEO1", AV_CODEC_ID_MSVIDEO1)
    .value("AV_CODEC_ID_IDCIN", AV_CODEC_ID_IDCIN)
    .value("AV_CODEC_ID_8BPS", AV_CODEC_ID_8BPS)
    .value("AV_CODEC_ID_SMC", AV_CODEC_ID_SMC)
    .value("AV_CODEC_ID_FLIC", AV_CODEC_ID_FLIC)
    .value("AV_CODEC_ID_TRUEMOTION1", AV_CODEC_ID_TRUEMOTION1)
    .value("AV_CODEC_ID_VMDVIDEO", AV_CODEC_ID_VMDVIDEO)
    .value("AV_CODEC_ID_MSZH", AV_CODEC_ID_MSZH)
    .value("AV_CODEC_ID_ZLIB", AV_CODEC_ID_ZLIB)
    .value("AV_CODEC_ID_QTRLE", AV_CODEC_ID_QTRLE)
    .value("AV_CODEC_ID_TSCC", AV_CODEC_ID_TSCC)
    .value("AV_CODEC_ID_ULTI", AV_CODEC_ID_ULTI)
    .value("AV_CODEC_ID_QDRAW", AV_CODEC_ID_QDRAW)
    .value("AV_CODEC_ID_VIXL", AV_CODEC_ID_VIXL)
    .value("AV_CODEC_ID_QPEG", AV_CODEC_ID_QPEG)
    .value("AV_CODEC_ID_PNG", AV_CODEC_ID_PNG)
    .value("AV_CODEC_ID_PPM", AV_CODEC_ID_PPM)
    .value("AV_CODEC_ID_PBM", AV_CODEC_ID_PBM)
    .value("AV_CODEC_ID_PGM", AV_CODEC_ID_PGM)
    .value("AV_CODEC_ID_PGMYUV", AV_CODEC_ID_PGMYUV)
    .value("AV_CODEC_ID_PAM", AV_CODEC_ID_PAM)
    .value("AV_CODEC_ID_FFVHUFF", AV_CODEC_ID_FFVHUFF)
    .value("AV_CODEC_ID_RV30", AV_CODEC_ID_RV30)
    .value("AV_CODEC_ID_RV40", AV_CODEC_ID_RV40)
    .value("AV_CODEC_ID_VC1", AV_CODEC_ID_VC1)
    .value("AV_CODEC_ID_WMV3", AV_CODEC_ID_WMV3)
    .value("AV_CODEC_ID_LOCO", AV_CODEC_ID_LOCO)
    .value("AV_CODEC_ID_WNV1", AV_CODEC_ID_WNV1)
    .value("AV_CODEC_ID_AASC", AV_CODEC_ID_AASC)
    .value("AV_CODEC_ID_INDEO2", AV_CODEC_ID_INDEO2)
    .value("AV_CODEC_ID_FRAPS", AV_CODEC_ID_FRAPS)
    .value("AV_CODEC_ID_TRUEMOTION2", AV_CODEC_ID_TRUEMOTION2)
    .value("AV_CODEC_ID_BMP", AV_CODEC_ID_BMP)
    .value("AV_CODEC_ID_CSCD", AV_CODEC_ID_CSCD)
    .value("AV_CODEC_ID_MMVIDEO", AV_CODEC_ID_MMVIDEO)
    .value("AV_CODEC_ID_ZMBV", AV_CODEC_ID_ZMBV)
    .value("AV_CODEC_ID_AVS", AV_CODEC_ID_AVS)
    .value("AV_CODEC_ID_SMACKVIDEO", AV_CODEC_ID_SMACKVIDEO)
    .value("AV_CODEC_ID_NUV", AV_CODEC_ID_NUV)
    .value("AV_CODEC_ID_KMVC", AV_CODEC_ID_KMVC)
    .value("AV_CODEC_ID_FLASHSV", AV_CODEC_ID_FLASHSV)
    .value("AV_CODEC_ID_CAVS", AV_CODEC_ID_CAVS)
    .value("AV_CODEC_ID_JPEG2000", AV_CODEC_ID_JPEG2000)
    .value("AV_CODEC_ID_VMNC", AV_CODEC_ID_VMNC)
    .value("AV_CODEC_ID_VP5", AV_CODEC_ID_VP5)
    .value("AV_CODEC_ID_VP6", AV_CODEC_ID_VP6)
    .value("AV_CODEC_ID_VP6F", AV_CODEC_ID_VP6F)
    .value("AV_CODEC_ID_TARGA", AV_CODEC_ID_TARGA)
    .value("AV_CODEC_ID_DSICINVIDEO", AV_CODEC_ID_DSICINVIDEO)
    .value("AV_CODEC_ID_TIERTEXSEQVIDEO", AV_CODEC_ID_TIERTEXSEQVIDEO)
    .value("AV_CODEC_ID_TIFF", AV_CODEC_ID_TIFF)
    .value("AV_CODEC_ID_GIF", AV_CODEC_ID_GIF)
    .value("AV_CODEC_ID_DXA", AV_CODEC_ID_DXA)
    .value("AV_CODEC_ID_DNXHD", AV_CODEC_ID_DNXHD)
    .value("AV_CODEC_ID_THP", AV_CODEC_ID_THP)
    .value("AV_CODEC_ID_SGI", AV_CODEC_ID_SGI)
    .value("AV_CODEC_ID_C93", AV_CODEC_ID_C93)
    .value("AV_CODEC_ID_BETHSOFTVID", AV_CODEC_ID_BETHSOFTVID)
    .value("AV_CODEC_ID_PTX", AV_CODEC_ID_PTX)
    .value("AV_CODEC_ID_TXD", AV_CODEC_ID_TXD)
    .value("AV_CODEC_ID_VP6A", AV_CODEC_ID_VP6A)
    .value("AV_CODEC_ID_AMV", AV_CODEC_ID_AMV)
    .value("AV_CODEC_ID_VB", AV_CODEC_ID_VB)
    .value("AV_CODEC_ID_PCX", AV_CODEC_ID_PCX)
    .value("AV_CODEC_ID_SUNRAST", AV_CODEC_ID_SUNRAST)
    .value("AV_CODEC_ID_INDEO4", AV_CODEC_ID_INDEO4)
    .value("AV_CODEC_ID_INDEO5", AV_CODEC_ID_INDEO5)
    .value("AV_CODEC_ID_MIMIC", AV_CODEC_ID_MIMIC)
    .value("AV_CODEC_ID_RL2", AV_CODEC_ID_RL2)
    .value("AV_CODEC_ID_ESCAPE124", AV_CODEC_ID_ESCAPE124)
    .value("AV_CODEC_ID_DIRAC", AV_CODEC_ID_DIRAC)
    .value("AV_CODEC_ID_BFI", AV_CODEC_ID_BFI)
    .value("AV_CODEC_ID_CMV", AV_CODEC_ID_CMV)
    .value("AV_CODEC_ID_MOTIONPIXELS", AV_CODEC_ID_MOTIONPIXELS)
    .value("AV_CODEC_ID_TGV", AV_CODEC_ID_TGV)
    .value("AV_CODEC_ID_TGQ", AV_CODEC_ID_TGQ)
    .value("AV_CODEC_ID_TQI", AV_CODEC_ID_TQI)
    .value("AV_CODEC_ID_AURA", AV_CODEC_ID_AURA)
    .value("AV_CODEC_ID_AURA2", AV_CODEC_ID_AURA2)
    .value("AV_CODEC_ID_V210X", AV_CODEC_ID_V210X)
    .value("AV_CODEC_ID_TMV", AV_CODEC_ID_TMV)
    .value("AV_CODEC_ID_V210", AV_CODEC_ID_V210)
    .value("AV_CODEC_ID_DPX", AV_CODEC_ID_DPX)
    .value("AV_CODEC_ID_MAD", AV_CODEC_ID_MAD)
    .value("AV_CODEC_ID_FRWU", AV_CODEC_ID_FRWU)
    .value("AV_CODEC_ID_FLASHSV2", AV_CODEC_ID_FLASHSV2)
    .value("AV_CODEC_ID_CDGRAPHICS", AV_CODEC_ID_CDGRAPHICS)
    .value("AV_CODEC_ID_R210", AV_CODEC_ID_R210)
    .value("AV_CODEC_ID_ANM", AV_CODEC_ID_ANM)
    .value("AV_CODEC_ID_BINKVIDEO", AV_CODEC_ID_BINKVIDEO)
    .value("AV_CODEC_ID_IFF_ILBM", AV_CODEC_ID_IFF_ILBM)
    .value("AV_CODEC_ID_KGV1", AV_CODEC_ID_KGV1)
    .value("AV_CODEC_ID_YOP", AV_CODEC_ID_YOP)
    .value("AV_CODEC_ID_VP8", AV_CODEC_ID_VP8)
    .value("AV_CODEC_ID_PICTOR", AV_CODEC_ID_PICTOR)
    .value("AV_CODEC_ID_ANSI", AV_CODEC_ID_ANSI)
    .value("AV_CODEC_ID_A64_MULTI", AV_CODEC_ID_A64_MULTI)
    .value("AV_CODEC_ID_A64_MULTI5", AV_CODEC_ID_A64_MULTI5)
    .value("AV_CODEC_ID_R10K", AV_CODEC_ID_R10K)
    .value("AV_CODEC_ID_MXPEG", AV_CODEC_ID_MXPEG)
    .value("AV_CODEC_ID_LAGARITH", AV_CODEC_ID_LAGARITH)
    .value("AV_CODEC_ID_PRORES", AV_CODEC_ID_PRORES)
    .value("AV_CODEC_ID_JV", AV_CODEC_ID_JV)
    .value("AV_CODEC_ID_DFA", AV_CODEC_ID_DFA)
    .value("AV_CODEC_ID_WMV3IMAGE", AV_CODEC_ID_WMV3IMAGE)
    .value("AV_CODEC_ID_VC1IMAGE", AV_CODEC_ID_VC1IMAGE)
    .value("AV_CODEC_ID_UTVIDEO", AV_CODEC_ID_UTVIDEO)
    .value("AV_CODEC_ID_BMV_VIDEO", AV_CODEC_ID_BMV_VIDEO)
    .value("AV_CODEC_ID_VBLE", AV_CODEC_ID_VBLE)
    .value("AV_CODEC_ID_DXTORY", AV_CODEC_ID_DXTORY)
    .value("AV_CODEC_ID_V410", AV_CODEC_ID_V410)
    .value("AV_CODEC_ID_XWD", AV_CODEC_ID_XWD)
    .value("AV_CODEC_ID_CDXL", AV_CODEC_ID_CDXL)
    .value("AV_CODEC_ID_XBM", AV_CODEC_ID_XBM)
    .value("AV_CODEC_ID_ZEROCODEC", AV_CODEC_ID_ZEROCODEC)
    .value("AV_CODEC_ID_MSS1", AV_CODEC_ID_MSS1)
    .value("AV_CODEC_ID_MSA1", AV_CODEC_ID_MSA1)
    .value("AV_CODEC_ID_TSCC2", AV_CODEC_ID_TSCC2)
    .value("AV_CODEC_ID_MTS2", AV_CODEC_ID_MTS2)
    .value("AV_CODEC_ID_CLLC", AV_CODEC_ID_CLLC)
    .value("AV_CODEC_ID_MSS2", AV_CODEC_ID_MSS2)
    .value("AV_CODEC_ID_VP9", AV_CODEC_ID_VP9)
    .value("AV_CODEC_ID_AIC", AV_CODEC_ID_AIC)
    .value("AV_CODEC_ID_ESCAPE130", AV_CODEC_ID_ESCAPE130)
    .value("AV_CODEC_ID_G2M", AV_CODEC_ID_G2M)
    .value("AV_CODEC_ID_WEBP", AV_CODEC_ID_WEBP)
    .value("AV_CODEC_ID_HNM4_VIDEO", AV_CODEC_ID_HNM4_VIDEO)
    .value("AV_CODEC_ID_HEVC", AV_CODEC_ID_HEVC)
    .value("AV_CODEC_ID_FIC", AV_CODEC_ID_FIC)
    .value("AV_CODEC_ID_ALIAS_PIX", AV_CODEC_ID_ALIAS_PIX)
    .value("AV_CODEC_ID_BRENDER_PIX", AV_CODEC_ID_BRENDER_PIX)
    .value("AV_CODEC_ID_PAF_VIDEO", AV_CODEC_ID_PAF_VIDEO)
    .value("AV_CODEC_ID_EXR", AV_CODEC_ID_EXR)
    .value("AV_CODEC_ID_VP7", AV_CODEC_ID_VP7)
    .value("AV_CODEC_ID_SANM", AV_CODEC_ID_SANM)
    .value("AV_CODEC_ID_SGIRLE", AV_CODEC_ID_SGIRLE)
    .value("AV_CODEC_ID_MVC1", AV_CODEC_ID_MVC1)
    .value("AV_CODEC_ID_MVC2", AV_CODEC_ID_MVC2)
    .value("AV_CODEC_ID_HQX", AV_CODEC_ID_HQX)
    .value("AV_CODEC_ID_TDSC", AV_CODEC_ID_TDSC)
    .value("AV_CODEC_ID_HQ_HQA", AV_CODEC_ID_HQ_HQA)
    .value("AV_CODEC_ID_HAP", AV_CODEC_ID_HAP)
    .value("AV_CODEC_ID_DDS", AV_CODEC_ID_DDS)
    .value("AV_CODEC_ID_DXV", AV_CODEC_ID_DXV)
    .value("AV_CODEC_ID_SCREENPRESSO", AV_CODEC_ID_SCREENPRESSO)
    .value("AV_CODEC_ID_RSCC", AV_CODEC_ID_RSCC)
    .value("AV_CODEC_ID_AVS2", AV_CODEC_ID_AVS2)
    .value("AV_CODEC_ID_Y41P", AV_CODEC_ID_Y41P)
    .value("AV_CODEC_ID_AVRP", AV_CODEC_ID_AVRP)
    .value("AV_CODEC_ID_012V", AV_CODEC_ID_012V)
    .value("AV_CODEC_ID_AVUI", AV_CODEC_ID_AVUI)
    .value("AV_CODEC_ID_AYUV", AV_CODEC_ID_AYUV)
    .value("AV_CODEC_ID_TARGA_Y216", AV_CODEC_ID_TARGA_Y216)
    .value("AV_CODEC_ID_V308", AV_CODEC_ID_V308)
    .value("AV_CODEC_ID_V408", AV_CODEC_ID_V408)
    .value("AV_CODEC_ID_YUV4", AV_CODEC_ID_YUV4)
    .value("AV_CODEC_ID_AVRN", AV_CODEC_ID_AVRN)
    .value("AV_CODEC_ID_CPIA", AV_CODEC_ID_CPIA)
    .value("AV_CODEC_ID_XFACE", AV_CODEC_ID_XFACE)
    .value("AV_CODEC_ID_SNOW", AV_CODEC_ID_SNOW)
    .value("AV_CODEC_ID_SMVJPEG", AV_CODEC_ID_SMVJPEG)
    .value("AV_CODEC_ID_APNG", AV_CODEC_ID_APNG)
    .value("AV_CODEC_ID_DAALA", AV_CODEC_ID_DAALA)
    .value("AV_CODEC_ID_CFHD", AV_CODEC_ID_CFHD)
    .value("AV_CODEC_ID_TRUEMOTION2RT", AV_CODEC_ID_TRUEMOTION2RT)
    .value("AV_CODEC_ID_M101", AV_CODEC_ID_M101)
    .value("AV_CODEC_ID_MAGICYUV", AV_CODEC_ID_MAGICYUV)
    .value("AV_CODEC_ID_SHEERVIDEO", AV_CODEC_ID_SHEERVIDEO)
    .value("AV_CODEC_ID_YLC", AV_CODEC_ID_YLC)
    .value("AV_CODEC_ID_PSD", AV_CODEC_ID_PSD)
    .value("AV_CODEC_ID_PIXLET", AV_CODEC_ID_PIXLET)
    .value("AV_CODEC_ID_SPEEDHQ", AV_CODEC_ID_SPEEDHQ)
    .value("AV_CODEC_ID_FMVC", AV_CODEC_ID_FMVC)
    .value("AV_CODEC_ID_SCPR", AV_CODEC_ID_SCPR)
    .value("AV_CODEC_ID_CLEARVIDEO", AV_CODEC_ID_CLEARVIDEO)
    .value("AV_CODEC_ID_XPM", AV_CODEC_ID_XPM)
    .value("AV_CODEC_ID_AV1", AV_CODEC_ID_AV1)
    .value("AV_CODEC_ID_BITPACKED", AV_CODEC_ID_BITPACKED)
    .value("AV_CODEC_ID_MSCC", AV_CODEC_ID_MSCC)
    .value("AV_CODEC_ID_SRGC", AV_CODEC_ID_SRGC)
    .value("AV_CODEC_ID_SVG", AV_CODEC_ID_SVG)
    .value("AV_CODEC_ID_GDV", AV_CODEC_ID_GDV)
    .value("AV_CODEC_ID_FITS", AV_CODEC_ID_FITS)
    .value("AV_CODEC_ID_IMM4", AV_CODEC_ID_IMM4)
    .value("AV_CODEC_ID_PROSUMER", AV_CODEC_ID_PROSUMER)
    .value("AV_CODEC_ID_MWSC", AV_CODEC_ID_MWSC)
    .value("AV_CODEC_ID_WCMV", AV_CODEC_ID_WCMV)
    .value("AV_CODEC_ID_RASC", AV_CODEC_ID_RASC)
    .value("AV_CODEC_ID_HYMT", AV_CODEC_ID_HYMT)
    .value("AV_CODEC_ID_ARBC", AV_CODEC_ID_ARBC)
    .value("AV_CODEC_ID_AGM", AV_CODEC_ID_AGM)
    .value("AV_CODEC_ID_LSCR", AV_CODEC_ID_LSCR)
    .value("AV_CODEC_ID_VP4", AV_CODEC_ID_VP4)
    .value("AV_CODEC_ID_FIRST_AUDIO", AV_CODEC_ID_FIRST_AUDIO)
    .value("AV_CODEC_ID_PCM_S16LE", AV_CODEC_ID_PCM_S16LE)
    .value("AV_CODEC_ID_PCM_S16BE", AV_CODEC_ID_PCM_S16BE)
    .value("AV_CODEC_ID_PCM_U16LE", AV_CODEC_ID_PCM_U16LE)
    .value("AV_CODEC_ID_PCM_U16BE", AV_CODEC_ID_PCM_U16BE)
    .value("AV_CODEC_ID_PCM_S8", AV_CODEC_ID_PCM_S8)
    .value("AV_CODEC_ID_PCM_U8", AV_CODEC_ID_PCM_U8)
    .value("AV_CODEC_ID_PCM_MULAW", AV_CODEC_ID_PCM_MULAW)
    .value("AV_CODEC_ID_PCM_ALAW", AV_CODEC_ID_PCM_ALAW)
    .value("AV_CODEC_ID_PCM_S32LE", AV_CODEC_ID_PCM_S32LE)
    .value("AV_CODEC_ID_PCM_S32BE", AV_CODEC_ID_PCM_S32BE)
    .value("AV_CODEC_ID_PCM_U32LE", AV_CODEC_ID_PCM_U32LE)
    .value("AV_CODEC_ID_PCM_U32BE", AV_CODEC_ID_PCM_U32BE)
    .value("AV_CODEC_ID_PCM_S24LE", AV_CODEC_ID_PCM_S24LE)
    .value("AV_CODEC_ID_PCM_S24BE", AV_CODEC_ID_PCM_S24BE)
    .value("AV_CODEC_ID_PCM_U24LE", AV_CODEC_ID_PCM_U24LE)
    .value("AV_CODEC_ID_PCM_U24BE", AV_CODEC_ID_PCM_U24BE)
    .value("AV_CODEC_ID_PCM_S24DAUD", AV_CODEC_ID_PCM_S24DAUD)
    .value("AV_CODEC_ID_PCM_ZORK", AV_CODEC_ID_PCM_ZORK)
    .value("AV_CODEC_ID_PCM_S16LE_PLANAR", AV_CODEC_ID_PCM_S16LE_PLANAR)
    .value("AV_CODEC_ID_PCM_DVD", AV_CODEC_ID_PCM_DVD)
    .value("AV_CODEC_ID_PCM_F32BE", AV_CODEC_ID_PCM_F32BE)
    .value("AV_CODEC_ID_PCM_F32LE", AV_CODEC_ID_PCM_F32LE)
    .value("AV_CODEC_ID_PCM_F64BE", AV_CODEC_ID_PCM_F64BE)
    .value("AV_CODEC_ID_PCM_F64LE", AV_CODEC_ID_PCM_F64LE)
    .value("AV_CODEC_ID_PCM_BLURAY", AV_CODEC_ID_PCM_BLURAY)
    .value("AV_CODEC_ID_PCM_LXF", AV_CODEC_ID_PCM_LXF)
    .value("AV_CODEC_ID_S302M", AV_CODEC_ID_S302M)
    .value("AV_CODEC_ID_PCM_S8_PLANAR", AV_CODEC_ID_PCM_S8_PLANAR)
    .value("AV_CODEC_ID_PCM_S24LE_PLANAR", AV_CODEC_ID_PCM_S24LE_PLANAR)
    .value("AV_CODEC_ID_PCM_S32LE_PLANAR", AV_CODEC_ID_PCM_S32LE_PLANAR)
    .value("AV_CODEC_ID_PCM_S16BE_PLANAR", AV_CODEC_ID_PCM_S16BE_PLANAR)
    .value("AV_CODEC_ID_PCM_S64LE", AV_CODEC_ID_PCM_S64LE)
    .value("AV_CODEC_ID_PCM_S64BE", AV_CODEC_ID_PCM_S64BE)
    .value("AV_CODEC_ID_PCM_F16LE", AV_CODEC_ID_PCM_F16LE)
    .value("AV_CODEC_ID_PCM_F24LE", AV_CODEC_ID_PCM_F24LE)
    .value("AV_CODEC_ID_PCM_VIDC", AV_CODEC_ID_PCM_VIDC)
    .value("AV_CODEC_ID_ADPCM_IMA_QT", AV_CODEC_ID_ADPCM_IMA_QT)
    .value("AV_CODEC_ID_ADPCM_IMA_WAV", AV_CODEC_ID_ADPCM_IMA_WAV)
    .value("AV_CODEC_ID_ADPCM_IMA_DK3", AV_CODEC_ID_ADPCM_IMA_DK3)
    .value("AV_CODEC_ID_ADPCM_IMA_DK4", AV_CODEC_ID_ADPCM_IMA_DK4)
    .value("AV_CODEC_ID_ADPCM_IMA_WS", AV_CODEC_ID_ADPCM_IMA_WS)
    .value("AV_CODEC_ID_ADPCM_IMA_SMJPEG", AV_CODEC_ID_ADPCM_IMA_SMJPEG)
    .value("AV_CODEC_ID_ADPCM_MS", AV_CODEC_ID_ADPCM_MS)
    .value("AV_CODEC_ID_ADPCM_4XM", AV_CODEC_ID_ADPCM_4XM)
    .value("AV_CODEC_ID_ADPCM_XA", AV_CODEC_ID_ADPCM_XA)
    .value("AV_CODEC_ID_ADPCM_ADX", AV_CODEC_ID_ADPCM_ADX)
    .value("AV_CODEC_ID_ADPCM_EA", AV_CODEC_ID_ADPCM_EA)
    .value("AV_CODEC_ID_ADPCM_G726", AV_CODEC_ID_ADPCM_G726)
    .value("AV_CODEC_ID_ADPCM_CT", AV_CODEC_ID_ADPCM_CT)
    .value("AV_CODEC_ID_ADPCM_SWF", AV_CODEC_ID_ADPCM_SWF)
    .value("AV_CODEC_ID_ADPCM_YAMAHA", AV_CODEC_ID_ADPCM_YAMAHA)
    .value("AV_CODEC_ID_ADPCM_SBPRO_4", AV_CODEC_ID_ADPCM_SBPRO_4)
    .value("AV_CODEC_ID_ADPCM_SBPRO_3", AV_CODEC_ID_ADPCM_SBPRO_3)
    .value("AV_CODEC_ID_ADPCM_SBPRO_2", AV_CODEC_ID_ADPCM_SBPRO_2)
    .value("AV_CODEC_ID_ADPCM_THP", AV_CODEC_ID_ADPCM_THP)
    .value("AV_CODEC_ID_ADPCM_IMA_AMV", AV_CODEC_ID_ADPCM_IMA_AMV)
    .value("AV_CODEC_ID_ADPCM_EA_R1", AV_CODEC_ID_ADPCM_EA_R1)
    .value("AV_CODEC_ID_ADPCM_EA_R3", AV_CODEC_ID_ADPCM_EA_R3)
    .value("AV_CODEC_ID_ADPCM_EA_R2", AV_CODEC_ID_ADPCM_EA_R2)
    .value("AV_CODEC_ID_ADPCM_IMA_EA_SEAD", AV_CODEC_ID_ADPCM_IMA_EA_SEAD)
    .value("AV_CODEC_ID_ADPCM_IMA_EA_EACS", AV_CODEC_ID_ADPCM_IMA_EA_EACS)
    .value("AV_CODEC_ID_ADPCM_EA_XAS", AV_CODEC_ID_ADPCM_EA_XAS)
    .value("AV_CODEC_ID_ADPCM_EA_MAXIS_XA", AV_CODEC_ID_ADPCM_EA_MAXIS_XA)
    .value("AV_CODEC_ID_ADPCM_IMA_ISS", AV_CODEC_ID_ADPCM_IMA_ISS)
    .value("AV_CODEC_ID_ADPCM_G722", AV_CODEC_ID_ADPCM_G722)
    .value("AV_CODEC_ID_ADPCM_IMA_APC", AV_CODEC_ID_ADPCM_IMA_APC)
    .value("AV_CODEC_ID_ADPCM_VIMA", AV_CODEC_ID_ADPCM_VIMA)
    .value("AV_CODEC_ID_ADPCM_AFC", AV_CODEC_ID_ADPCM_AFC)
    .value("AV_CODEC_ID_ADPCM_IMA_OKI", AV_CODEC_ID_ADPCM_IMA_OKI)
    .value("AV_CODEC_ID_ADPCM_DTK", AV_CODEC_ID_ADPCM_DTK)
    .value("AV_CODEC_ID_ADPCM_IMA_RAD", AV_CODEC_ID_ADPCM_IMA_RAD)
    .value("AV_CODEC_ID_ADPCM_G726LE", AV_CODEC_ID_ADPCM_G726LE)
    .value("AV_CODEC_ID_ADPCM_THP_LE", AV_CODEC_ID_ADPCM_THP_LE)
    .value("AV_CODEC_ID_ADPCM_PSX", AV_CODEC_ID_ADPCM_PSX)
    .value("AV_CODEC_ID_ADPCM_AICA", AV_CODEC_ID_ADPCM_AICA)
    .value("AV_CODEC_ID_ADPCM_IMA_DAT4", AV_CODEC_ID_ADPCM_IMA_DAT4)
    .value("AV_CODEC_ID_ADPCM_MTAF", AV_CODEC_ID_ADPCM_MTAF)
    .value("AV_CODEC_ID_ADPCM_AGM", AV_CODEC_ID_ADPCM_AGM)
    .value("AV_CODEC_ID_AMR_NB", AV_CODEC_ID_AMR_NB)
    .value("AV_CODEC_ID_AMR_WB", AV_CODEC_ID_AMR_WB)
    .value("AV_CODEC_ID_RA_144", AV_CODEC_ID_RA_144)
    .value("AV_CODEC_ID_RA_288", AV_CODEC_ID_RA_288)
    .value("AV_CODEC_ID_ROQ_DPCM", AV_CODEC_ID_ROQ_DPCM)
    .value("AV_CODEC_ID_INTERPLAY_DPCM", AV_CODEC_ID_INTERPLAY_DPCM)
    .value("AV_CODEC_ID_XAN_DPCM", AV_CODEC_ID_XAN_DPCM)
    .value("AV_CODEC_ID_SOL_DPCM", AV_CODEC_ID_SOL_DPCM)
    .value("AV_CODEC_ID_SDX2_DPCM", AV_CODEC_ID_SDX2_DPCM)
    .value("AV_CODEC_ID_GREMLIN_DPCM", AV_CODEC_ID_GREMLIN_DPCM)
    .value("AV_CODEC_ID_MP2", AV_CODEC_ID_MP2)
    .value("AV_CODEC_ID_MP3", AV_CODEC_ID_MP3)
    .value("AV_CODEC_ID_AAC", AV_CODEC_ID_AAC)
    .value("AV_CODEC_ID_AC3", AV_CODEC_ID_AC3)
    .value("AV_CODEC_ID_DTS", AV_CODEC_ID_DTS)
    .value("AV_CODEC_ID_VORBIS", AV_CODEC_ID_VORBIS)
    .value("AV_CODEC_ID_DVAUDIO", AV_CODEC_ID_DVAUDIO)
    .value("AV_CODEC_ID_WMAV1", AV_CODEC_ID_WMAV1)
    .value("AV_CODEC_ID_WMAV2", AV_CODEC_ID_WMAV2)
    .value("AV_CODEC_ID_MACE3", AV_CODEC_ID_MACE3)
    .value("AV_CODEC_ID_MACE6", AV_CODEC_ID_MACE6)
    .value("AV_CODEC_ID_VMDAUDIO", AV_CODEC_ID_VMDAUDIO)
    .value("AV_CODEC_ID_FLAC", AV_CODEC_ID_FLAC)
    .value("AV_CODEC_ID_MP3ADU", AV_CODEC_ID_MP3ADU)
    .value("AV_CODEC_ID_MP3ON4", AV_CODEC_ID_MP3ON4)
    .value("AV_CODEC_ID_SHORTEN", AV_CODEC_ID_SHORTEN)
    .value("AV_CODEC_ID_ALAC", AV_CODEC_ID_ALAC)
    .value("AV_CODEC_ID_WESTWOOD_SND1", AV_CODEC_ID_WESTWOOD_SND1)
    .value("AV_CODEC_ID_GSM", AV_CODEC_ID_GSM)
    .value("AV_CODEC_ID_QDM2", AV_CODEC_ID_QDM2)
    .value("AV_CODEC_ID_COOK", AV_CODEC_ID_COOK)
    .value("AV_CODEC_ID_TRUESPEECH", AV_CODEC_ID_TRUESPEECH)
    .value("AV_CODEC_ID_TTA", AV_CODEC_ID_TTA)
    .value("AV_CODEC_ID_SMACKAUDIO", AV_CODEC_ID_SMACKAUDIO)
    .value("AV_CODEC_ID_QCELP", AV_CODEC_ID_QCELP)
    .value("AV_CODEC_ID_WAVPACK", AV_CODEC_ID_WAVPACK)
    .value("AV_CODEC_ID_DSICINAUDIO", AV_CODEC_ID_DSICINAUDIO)
    .value("AV_CODEC_ID_IMC", AV_CODEC_ID_IMC)
    .value("AV_CODEC_ID_MUSEPACK7", AV_CODEC_ID_MUSEPACK7)
    .value("AV_CODEC_ID_MLP", AV_CODEC_ID_MLP)
    .value("AV_CODEC_ID_GSM_MS", AV_CODEC_ID_GSM_MS)
    .value("AV_CODEC_ID_ATRAC3", AV_CODEC_ID_ATRAC3)
    .value("AV_CODEC_ID_APE", AV_CODEC_ID_APE)
    .value("AV_CODEC_ID_NELLYMOSER", AV_CODEC_ID_NELLYMOSER)
    .value("AV_CODEC_ID_MUSEPACK8", AV_CODEC_ID_MUSEPACK8)
    .value("AV_CODEC_ID_SPEEX", AV_CODEC_ID_SPEEX)
    .value("AV_CODEC_ID_WMAVOICE", AV_CODEC_ID_WMAVOICE)
    .value("AV_CODEC_ID_WMAPRO", AV_CODEC_ID_WMAPRO)
    .value("AV_CODEC_ID_WMALOSSLESS", AV_CODEC_ID_WMALOSSLESS)
    .value("AV_CODEC_ID_ATRAC3P", AV_CODEC_ID_ATRAC3P)
    .value("AV_CODEC_ID_EAC3", AV_CODEC_ID_EAC3)
    .value("AV_CODEC_ID_SIPR", AV_CODEC_ID_SIPR)
    .value("AV_CODEC_ID_MP1", AV_CODEC_ID_MP1)
    .value("AV_CODEC_ID_TWINVQ", AV_CODEC_ID_TWINVQ)
    .value("AV_CODEC_ID_TRUEHD", AV_CODEC_ID_TRUEHD)
    .value("AV_CODEC_ID_MP4ALS", AV_CODEC_ID_MP4ALS)
    .value("AV_CODEC_ID_ATRAC1", AV_CODEC_ID_ATRAC1)
    .value("AV_CODEC_ID_BINKAUDIO_RDFT", AV_CODEC_ID_BINKAUDIO_RDFT)
    .value("AV_CODEC_ID_BINKAUDIO_DCT", AV_CODEC_ID_BINKAUDIO_DCT)
    .value("AV_CODEC_ID_AAC_LATM", AV_CODEC_ID_AAC_LATM)
    .value("AV_CODEC_ID_QDMC", AV_CODEC_ID_QDMC)
    .value("AV_CODEC_ID_CELT", AV_CODEC_ID_CELT)
    .value("AV_CODEC_ID_G723_1", AV_CODEC_ID_G723_1)
    .value("AV_CODEC_ID_G729", AV_CODEC_ID_G729)
    .value("AV_CODEC_ID_8SVX_EXP", AV_CODEC_ID_8SVX_EXP)
    .value("AV_CODEC_ID_8SVX_FIB", AV_CODEC_ID_8SVX_FIB)
    .value("AV_CODEC_ID_BMV_AUDIO", AV_CODEC_ID_BMV_AUDIO)
    .value("AV_CODEC_ID_RALF", AV_CODEC_ID_RALF)
    .value("AV_CODEC_ID_IAC", AV_CODEC_ID_IAC)
    .value("AV_CODEC_ID_ILBC", AV_CODEC_ID_ILBC)
    .value("AV_CODEC_ID_OPUS", AV_CODEC_ID_OPUS)
    .value("AV_CODEC_ID_COMFORT_NOISE", AV_CODEC_ID_COMFORT_NOISE)
    .value("AV_CODEC_ID_TAK", AV_CODEC_ID_TAK)
    .value("AV_CODEC_ID_METASOUND", AV_CODEC_ID_METASOUND)
    .value("AV_CODEC_ID_PAF_AUDIO", AV_CODEC_ID_PAF_AUDIO)
    .value("AV_CODEC_ID_ON2AVC", AV_CODEC_ID_ON2AVC)
    .value("AV_CODEC_ID_DSS_SP", AV_CODEC_ID_DSS_SP)
    .value("AV_CODEC_ID_CODEC2", AV_CODEC_ID_CODEC2)
    .value("AV_CODEC_ID_FFWAVESYNTH", AV_CODEC_ID_FFWAVESYNTH)
    .value("AV_CODEC_ID_SONIC", AV_CODEC_ID_SONIC)
    .value("AV_CODEC_ID_SONIC_LS", AV_CODEC_ID_SONIC_LS)
    .value("AV_CODEC_ID_EVRC", AV_CODEC_ID_EVRC)
    .value("AV_CODEC_ID_SMV", AV_CODEC_ID_SMV)
    .value("AV_CODEC_ID_DSD_LSBF", AV_CODEC_ID_DSD_LSBF)
    .value("AV_CODEC_ID_DSD_MSBF", AV_CODEC_ID_DSD_MSBF)
    .value("AV_CODEC_ID_DSD_LSBF_PLANAR", AV_CODEC_ID_DSD_LSBF_PLANAR)
    .value("AV_CODEC_ID_DSD_MSBF_PLANAR", AV_CODEC_ID_DSD_MSBF_PLANAR)
    .value("AV_CODEC_ID_4GV", AV_CODEC_ID_4GV)
    .value("AV_CODEC_ID_INTERPLAY_ACM", AV_CODEC_ID_INTERPLAY_ACM)
    .value("AV_CODEC_ID_XMA1", AV_CODEC_ID_XMA1)
    .value("AV_CODEC_ID_XMA2", AV_CODEC_ID_XMA2)
    .value("AV_CODEC_ID_DST", AV_CODEC_ID_DST)
    .value("AV_CODEC_ID_ATRAC3AL", AV_CODEC_ID_ATRAC3AL)
    .value("AV_CODEC_ID_ATRAC3PAL", AV_CODEC_ID_ATRAC3PAL)
    .value("AV_CODEC_ID_DOLBY_E", AV_CODEC_ID_DOLBY_E)
    .value("AV_CODEC_ID_APTX", AV_CODEC_ID_APTX)
    .value("AV_CODEC_ID_APTX_HD", AV_CODEC_ID_APTX_HD)
    .value("AV_CODEC_ID_SBC", AV_CODEC_ID_SBC)
    .value("AV_CODEC_ID_ATRAC9", AV_CODEC_ID_ATRAC9)
    .value("AV_CODEC_ID_HCOM", AV_CODEC_ID_HCOM);
#pragma endregion AVCodecID enums
}
