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
}

export interface FFModule extends EmscriptenModule {
  free_result(): void
  convert(videoData: Uint8ClampedArray, opts: ConvertOptions): Uint8Array
  list_codecs(): Vector<CodecDescription>
}

export default function(opts: Partial<EmscriptenModule>): FFModule
