import ffmpeg, { FFModule } from '../../lib/ffmpeg/ffmpeg'
import wasmUrl from '../../lib/ffmpeg/ffmpeg.wasm'
import { initEmscriptenModule } from './util'

let emscriptenModule: Promise<FFModule>

export default async function convert(data: ArrayBuffer) {
  if (!emscriptenModule) {
    emscriptenModule = initEmscriptenModule(ffmpeg, wasmUrl)
  }

  const module = await emscriptenModule

  const resultView = module.convert(new Uint8ClampedArray(data))
  const result = new Uint8ClampedArray(resultView)
  module.free_result()

  return result.buffer as ArrayBuffer
}
