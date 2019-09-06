export interface ConvertOptions {
  verbose: boolean
  outputFormat: string
  videoEncoder: string
  audioEncoder: string
}

export interface FFModule extends EmscriptenModule {
  free_result(): void
  convert(videoData: Uint8ClampedArray, opts: ConvertOptions): Uint8Array
}

export default function(opts: Partial<EmscriptenModule>): FFModule
