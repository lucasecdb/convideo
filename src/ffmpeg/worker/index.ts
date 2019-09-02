import { expose } from 'comlink'

import ffmpeg, { FFModule } from '../../../lib/ffmpeg/ffmpeg'
import wasmUrl from '../../../lib/ffmpeg/ffmpeg.wasm'
import { initEmscriptenModule } from '../util'

let emscriptenModule: Promise<FFModule>

const convert = async (data: ArrayBuffer) => {
  if (!emscriptenModule) {
    emscriptenModule = initEmscriptenModule(ffmpeg, wasmUrl, {
      // @ts-ignore
      stdin: () => 0,
    })
  }

  const module = await emscriptenModule

  try {
    const resultView = module.convert(new Uint8ClampedArray(data))
    const result = new Uint8ClampedArray(resultView)

    return result.buffer as ArrayBuffer
  } finally {
    module.free_result()
  }
}

const exports = {
  convert,
}

export type FFmpegWorkerAPI = typeof exports

expose(exports, self as any)
