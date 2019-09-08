export interface ConvertOptions {
  verbose: boolean
  outputFormat: string
  videoEncoder: string
  audioEncoder: string
}

interface Vector<T> {
  get(index: number): T
  size(): number
}

export interface CodecDescription {
  id: { value: number }
  name: string
  long_name: string
  props: number
  type: { value: number }
  mime_types: Vector<string>
}

export interface FFModule extends EmscriptenModule {
  free_result(): void
  convert(videoData: Uint8ClampedArray, opts: ConvertOptions): Uint8Array
  list_codecs(): Vector<CodecDescription>
  AV_CODEC_PROP_INTRA_ONLY: number
  AV_CODEC_PROP_LOSSY: number
  AV_CODEC_PROP_LOSSLESS: number
  AV_CODEC_PROP_BITMAP_SUB: number
  AV_CODEC_PROP_TEXT_SUB: number
}

export default function(opts: Partial<EmscriptenModule>): FFModule
