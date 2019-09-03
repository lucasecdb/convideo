import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/ffmpeg.wasm'
import memUrl from '../../../lib/ffmpeg/asm/ffmpeg.js.mem'
import { initEmscriptenModule } from '../util'

type FFModule = import('../../../lib/ffmpeg/ffmpeg').FFModule

let ffmpegModule: Promise<FFModule>
let asmFFmpegModule: Promise<FFModule>

const convert = async (data: ArrayBuffer) => {
  if (!ffmpegModule) {
    const ffmpeg = (await import('../../../lib/ffmpeg/ffmpeg')).default
    // eslint-disable-next-line require-atomic-updates
    ffmpegModule = initEmscriptenModule(ffmpeg, { wasmUrl })
  }

  const module = await ffmpegModule

  try {
    const resultView = module.convert(new Uint8ClampedArray(data))
    const result = new Uint8ClampedArray(resultView)

    return result.buffer as ArrayBuffer
  } finally {
    module.free_result()
  }
}

const convertAsm = async (data: ArrayBuffer) => {
  if (!asmFFmpegModule) {
    const ffmpegAsm = (await import('../../../lib/ffmpeg/asm/ffmpeg')).default
    // eslint-disable-next-line require-atomic-updates
    asmFFmpegModule = initEmscriptenModule(ffmpegAsm, { memUrl })
  }

  const module = await asmFFmpegModule

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
  convertAsm,
}

export type FFmpegWorkerAPI = typeof exports

expose(exports, self as any)
