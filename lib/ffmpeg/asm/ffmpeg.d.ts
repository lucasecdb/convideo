export interface FFModule extends EmscriptenModule {
  free_result(): void
  convert(videoData: Uint8ClampedArray): Uint8Array
}

export default function(opts: Partial<EmscriptenModule>): FFModule
