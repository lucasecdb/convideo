#include <emscripten/bind.h>
#include <emscripten/val.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavutil/log.h"
#include "libavutil/pixdesc.h"
#include "libavutil/display.h"
#include "libavutil/eval.h"
}

using namespace emscripten;
using std::string;

extern int main(int argc, char** argv);

std::vector<AVCodec> list_encoders() {
  std::vector<AVCodec> codec_list;

  void *i = 0;
  const AVCodec* codec;

  while ((codec = av_codec_iterate(&i)) != NULL) {
    // skip decoder-only codecs
    if (avcodec_find_encoder(codec->id) == NULL)
      continue;

    codec_list.push_back(*codec);
  }

  return codec_list;
}

string get_avcodec_name(const AVCodec& c) {
  return c.name;
}

string get_avcodec_long_name(const AVCodec& c) {
  return c.long_name;
}

std::vector<AVOutputFormat> list_muxers() {
  std::vector<AVOutputFormat> muxer_list;

  void *i = 0;
  const AVOutputFormat* format;

  while ((format = av_muxer_iterate(&i)) != NULL) {
    muxer_list.push_back(*format);
  }

  return muxer_list;
}

std::vector<AVOption> list_codec_options(int codec_id) {
  std::vector<AVOption> option_list;

  const AVCodec* codec = avcodec_find_encoder((AVCodecID)codec_id);

  if (codec == NULL || !codec->priv_class) {
    return option_list;
  }

  const AVClass *codec_class = codec->priv_class;

  const AVOption *opt = NULL;

  while ((opt = av_opt_next(&codec_class, opt))) {
    if (opt->type == AV_OPT_TYPE_CONST) continue;

    option_list.push_back(*opt);
  }

  return option_list;
}

string get_avformat_name(const AVOutputFormat& format) {
  return format.name;
}

string get_avformat_long_name(const AVOutputFormat& format) {
  return format.long_name;
}

string get_avformat_mime_type(const AVOutputFormat& format) {
  return format.mime_type;
}

string get_avformat_extensions(const AVOutputFormat& format) {
  return format.extensions;
}

string get_avoption_name(const AVOption& opt) {
  return opt.name;
}

string get_avoption_help(const AVOption& opt) {
  return opt.help;
}

string get_avoption_unit(const AVOption& opt) {
  return opt.unit;
}

union AVOptionDefaultValue {
  int64_t i64;
  double dbl;
  const char* str;
  AVRational q;
};

AVOptionDefaultValue get_avoption_default_val(const AVOption& opt) {
  if (opt.default_val.i64) {
    return {.i64 = opt.default_val.i64};
  } else if (opt.default_val.dbl) {
    return {.dbl = opt.default_val.dbl};
  } else if (opt.default_val.str) {
    return {.str = opt.default_val.str};
  } else {
    return {.q = opt.default_val.q};
  }
}

string get_avoption_default_val_str(const AVOptionDefaultValue& default_val) {
  return default_val.str;
}

EMSCRIPTEN_BINDINGS(ffmpeg) {
  register_vector<AVCodec>("VectorAVCodec");
  register_vector<AVOutputFormat>("VectorAVOutputFormat");
  register_vector<AVOption>("VectorAVOption");

  class_<AVOptionDefaultValue>("AVOptionDefaultValue")
    .property("i64", &AVOptionDefaultValue::i64)
    .property("dbl", &AVOptionDefaultValue::dbl)
    .property("str", &get_avoption_default_val_str)
    .property("q", &AVOptionDefaultValue::q);

  enum_<AVMediaType>("AVMediaType")
    .value("AVMEDIA_TYPE_UNKNOWN", AVMEDIA_TYPE_UNKNOWN)
    .value("AVMEDIA_TYPE_VIDEO", AVMEDIA_TYPE_VIDEO)
    .value("AVMEDIA_TYPE_AUDIO", AVMEDIA_TYPE_AUDIO)
    .value("AVMEDIA_TYPE_DATA", AVMEDIA_TYPE_DATA)
    .value("AVMEDIA_TYPE_SUBTITLE", AVMEDIA_TYPE_SUBTITLE)
    .value("AVMEDIA_TYPE_ATTACHMENT", AVMEDIA_TYPE_ATTACHMENT)
    .value("AVMEDIA_TYPE_NB", AVMEDIA_TYPE_NB);

  value_object<AVRational>("AVRational")
    .field("num", &AVRational::num)
    .field("den", &AVRational::den);

  class_<AVCodec>("AVCodec")
    .property("id", &AVCodec::id)
    .property("type", &AVCodec::type)
    .property("name", &get_avcodec_name)
    .property("long_name", &get_avcodec_long_name)
    .property("capabilities", &AVCodec::capabilities);

  class_<AVOutputFormat>("AVOutputFormat")
    .property("name", &get_avformat_name)
    .property("long_name", &get_avformat_long_name)
    .property("mime_type", &get_avformat_mime_type)
    .property("extensions", &get_avformat_extensions)
    .property("audio_codec", &AVOutputFormat::audio_codec)
    .property("video_codec", &AVOutputFormat::video_codec);

  class_<AVOption>("AVOption")
    .property("name", &get_avoption_name)
    .property("help", &get_avoption_help)
    .property("unit", &get_avoption_unit)
    .property("offset", &AVOption::offset)
    .property("type", &AVOption::type)
    /*union {
        int64_t i64;
        double dbl;
        const char *str;
        AVRational q;
    } default_val;*/
    .property("default_val", &get_avoption_default_val)
    .property("min", &AVOption::min)
    .property("max", &AVOption::max)
    .property("flags", &AVOption::flags);

  constant("AV_OPT_FLAG_ENCODING_PARAM", AV_OPT_FLAG_ENCODING_PARAM);
  constant("AV_OPT_FLAG_DECODING_PARAM", AV_OPT_FLAG_DECODING_PARAM);
  constant("AV_OPT_FLAG_AUDIO_PARAM", AV_OPT_FLAG_AUDIO_PARAM);
  constant("AV_OPT_FLAG_VIDEO_PARAM", AV_OPT_FLAG_VIDEO_PARAM);
  constant("AV_OPT_FLAG_SUBTITLE_PARAM", AV_OPT_FLAG_SUBTITLE_PARAM);
  constant("AV_OPT_FLAG_EXPORT", AV_OPT_FLAG_EXPORT);
  constant("AV_OPT_FLAG_READONLY", AV_OPT_FLAG_READONLY);
  constant("AV_OPT_FLAG_BSF_PARAM", AV_OPT_FLAG_BSF_PARAM);
  constant("AV_OPT_FLAG_FILTERING_PARAM", AV_OPT_FLAG_FILTERING_PARAM);
  constant("AV_OPT_FLAG_DEPRECATED", AV_OPT_FLAG_DEPRECATED);

  constant("AV_CODEC_CAP_DRAW_HORIZ_BAND", AV_CODEC_CAP_DRAW_HORIZ_BAND);
  constant("AV_CODEC_CAP_DR1", AV_CODEC_CAP_DR1);
  constant("AV_CODEC_CAP_TRUNCATED", AV_CODEC_CAP_TRUNCATED);
  constant("AV_CODEC_CAP_DELAY", AV_CODEC_CAP_DELAY);
  constant("AV_CODEC_CAP_SMALL_LAST_FRAME", AV_CODEC_CAP_SMALL_LAST_FRAME);
  constant("AV_CODEC_CAP_SUBFRAMES", AV_CODEC_CAP_SUBFRAMES);
  constant("AV_CODEC_CAP_EXPERIMENTAL", AV_CODEC_CAP_EXPERIMENTAL);
  constant("AV_CODEC_CAP_CHANNEL_CONF", AV_CODEC_CAP_CHANNEL_CONF);
  constant("AV_CODEC_CAP_FRAME_THREADS", AV_CODEC_CAP_FRAME_THREADS);
  constant("AV_CODEC_CAP_SLICE_THREADS", AV_CODEC_CAP_SLICE_THREADS);
  constant("AV_CODEC_CAP_PARAM_CHANGE", AV_CODEC_CAP_PARAM_CHANGE);
  constant("AV_CODEC_CAP_AUTO_THREADS", AV_CODEC_CAP_AUTO_THREADS);
  constant("AV_CODEC_CAP_VARIABLE_FRAME_SIZE", AV_CODEC_CAP_VARIABLE_FRAME_SIZE);
  constant("AV_CODEC_CAP_AVOID_PROBING", AV_CODEC_CAP_AVOID_PROBING);
  constant("AV_CODEC_CAP_INTRA_ONLY", AV_CODEC_CAP_INTRA_ONLY);
  constant("AV_CODEC_CAP_LOSSLESS", AV_CODEC_CAP_LOSSLESS);
  constant("AV_CODEC_CAP_HARDWARE", AV_CODEC_CAP_HARDWARE);
  constant("AV_CODEC_CAP_HYBRID", AV_CODEC_CAP_HYBRID);
  constant("AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE", AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE);

  function("list_encoders", &list_encoders);
  function("list_muxers", &list_muxers);
  function("list_codec_options", &list_codec_options);

  enum_<AVOptionType>("AVOptionType")
    .value("AV_OPT_TYPE_FLAGS", AV_OPT_TYPE_FLAGS)
    .value("AV_OPT_TYPE_INT", AV_OPT_TYPE_INT)
    .value("AV_OPT_TYPE_INT64", AV_OPT_TYPE_INT64)
    .value("AV_OPT_TYPE_DOUBLE", AV_OPT_TYPE_DOUBLE)
    .value("AV_OPT_TYPE_FLOAT", AV_OPT_TYPE_FLOAT)
    .value("AV_OPT_TYPE_STRING", AV_OPT_TYPE_STRING)
    .value("AV_OPT_TYPE_RATIONAL", AV_OPT_TYPE_RATIONAL)
    .value("AV_OPT_TYPE_BINARY", AV_OPT_TYPE_BINARY)
    .value("AV_OPT_TYPE_DICT", AV_OPT_TYPE_DICT)
    .value("AV_OPT_TYPE_UINT64", AV_OPT_TYPE_UINT64)
    .value("AV_OPT_TYPE_CONST", AV_OPT_TYPE_CONST)
    .value("AV_OPT_TYPE_IMAGE_SIZE", AV_OPT_TYPE_IMAGE_SIZE)
    .value("AV_OPT_TYPE_PIXEL_FMT", AV_OPT_TYPE_PIXEL_FMT)
    .value("AV_OPT_TYPE_SAMPLE_FMT", AV_OPT_TYPE_SAMPLE_FMT)
    .value("AV_OPT_TYPE_VIDEO_RATE", AV_OPT_TYPE_VIDEO_RATE)
    .value("AV_OPT_TYPE_DURATION", AV_OPT_TYPE_DURATION)
    .value("AV_OPT_TYPE_COLOR", AV_OPT_TYPE_COLOR)
    .value("AV_OPT_TYPE_CHANNEL_LAYOUT", AV_OPT_TYPE_CHANNEL_LAYOUT)
    .value("AV_OPT_TYPE_BOOL", AV_OPT_TYPE_BOOL);

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
