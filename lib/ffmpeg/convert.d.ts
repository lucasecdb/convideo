interface EnumValue {
  value: number
}

export interface ConvertOptions {
  verbose: boolean
  outputFormat: string
  videoEncoder: string
  audioEncoder: string
  extraOptions?: string[]
}

export interface Vector<T> {
  get(index: number): T
  size(): number
}

export interface Codec {
  id: EnumValue
  name: string
  long_name: string
  capabilities: number
  type: EnumValue
}

export interface CodecOption {
  name: string
  help: string
  unit: string
  offset: number
  type: EnumValue
  min: number
  max: number
  flags: number
  default_val: any
}

export interface Muxer {
  name: string
  long_name: string
  mime_type: string
  extensions: string
  video_codec?: EnumValue
  audio_codec?: EnumValue
}

export interface FFModule extends EmscriptenModule {
  free_result(): void
  convert(videoData: Uint8ClampedArray, opts: ConvertOptions): Uint8Array
  list_encoders(): Vector<Codec>
  list_codec_options(id: number): Vector<CodecOption>
  list_muxers(): Vector<Muxer>

  AV_CODEC_CAP_DRAW_HORIZ_BAND: number
  AV_CODEC_CAP_DR1: number
  AV_CODEC_CAP_TRUNCATED: number
  AV_CODEC_CAP_DELAY: number
  AV_CODEC_CAP_SMALL_LAST_FRAME: number
  AV_CODEC_CAP_SUBFRAMES: number
  AV_CODEC_CAP_EXPERIMENTAL: number
  AV_CODEC_CAP_CHANNEL_CONF: number
  AV_CODEC_CAP_FRAME_THREADS: number
  AV_CODEC_CAP_SLICE_THREADS: number
  AV_CODEC_CAP_PARAM_CHANGE: number
  AV_CODEC_CAP_AUTO_THREADS: number
  AV_CODEC_CAP_VARIABLE_FRAME_SIZE: number
  AV_CODEC_CAP_AVOID_PROBING: number
  AV_CODEC_CAP_INTRA_ONLY: number
  AV_CODEC_CAP_LOSSLESS: number
  AV_CODEC_CAP_HARDWARE: number
  AV_CODEC_CAP_HYBRID: number
  AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE: number

  AV_OPT_FLAG_ENCODING_PARAM: number
  AV_OPT_FLAG_DECODING_PARAM: number
  AV_OPT_FLAG_AUDIO_PARAM: number
  AV_OPT_FLAG_VIDEO_PARAM: number
  AV_OPT_FLAG_SUBTITLE_PARAM: number
  AV_OPT_FLAG_EXPORT: number
  AV_OPT_FLAG_READONLY: number
  AV_OPT_FLAG_BSF_PARAM: number
  AV_OPT_FLAG_FILTERING_PARAM: number
  AV_OPT_FLAG_DEPRECATED: number

  AVOptionType: {
    AV_OPT_TYPE_FLAGS: EnumValue
    AV_OPT_TYPE_INT: EnumValue
    AV_OPT_TYPE_INT64: EnumValue
    AV_OPT_TYPE_DOUBLE: EnumValue
    AV_OPT_TYPE_FLOAT: EnumValue
    AV_OPT_TYPE_STRING: EnumValue
    AV_OPT_TYPE_RATIONAL: EnumValue
    AV_OPT_TYPE_BINARY: EnumValue
    AV_OPT_TYPE_DICT: EnumValue
    AV_OPT_TYPE_UINT64: EnumValue
    AV_OPT_TYPE_CONST: EnumValue
    AV_OPT_TYPE_IMAGE_SIZE: EnumValue
    AV_OPT_TYPE_PIXEL_FMT: EnumValue
    AV_OPT_TYPE_SAMPLE_FMT: EnumValue
    AV_OPT_TYPE_VIDEO_RATE: EnumValue
    AV_OPT_TYPE_DURATION: EnumValue
    AV_OPT_TYPE_COLOR: EnumValue
    AV_OPT_TYPE_CHANNEL_LAYOUT: EnumValue
    AV_OPT_TYPE_BOOL: EnumValue
  }
}

export default function(opts: Partial<EmscriptenModule>): FFModule
