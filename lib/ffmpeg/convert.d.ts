interface EnumValue {
  value: number
}

export interface ConvertOptions {
  verbose: boolean
  outputFormat: string
  videoEncoder: string
  audioEncoder: string
  extraOptions: string[]
}

interface Vector<T> {
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
}

export default function(opts: Partial<EmscriptenModule>): FFModule
