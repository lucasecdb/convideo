import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/convert.wasm'
import memUrl from '../../../lib/ffmpeg/asm/convert.js.mem'
import { initEmscriptenModule } from '../util'

type FFModule = import('../../../lib/ffmpeg/convert').FFModule
export type ConvertOptions = import('../../../lib/ffmpeg/convert').ConvertOptions

let ffmpegModule: Promise<FFModule>
let asmFFmpegModule: Promise<FFModule>

const _convert = async (
  module: FFModule,
  data: ArrayBuffer,
  opts: ConvertOptions
) => {
  try {
    const resultView = module.convert(new Uint8ClampedArray(data), opts)
    const result = new Uint8ClampedArray(resultView)

    return result.buffer as ArrayBuffer
  } finally {
    module.free_result()
  }
}

const convert = async (data: ArrayBuffer, opts: ConvertOptions) => {
  if (!ffmpegModule) {
    const ffmpeg = (await import('../../../lib/ffmpeg/convert')).default
    // eslint-disable-next-line require-atomic-updates
    ffmpegModule = initEmscriptenModule(ffmpeg, { wasmUrl })
  }

  const module = await ffmpegModule

  return _convert(module, data, opts)
}

const convertAsm = async (data: ArrayBuffer, opts: ConvertOptions) => {
  if (!asmFFmpegModule) {
    const ffmpegAsm = (await import('../../../lib/ffmpeg/asm/convert')).default
    // eslint-disable-next-line require-atomic-updates
    asmFFmpegModule = initEmscriptenModule(ffmpegAsm, { memUrl })
  }

  const module = await asmFFmpegModule

  return _convert(module, data, opts)
}

const exports = {
  convert,
  convertAsm,
}

export type FFmpegWorkerAPI = typeof exports

expose(exports, self as any)
