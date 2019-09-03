import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/ffmpeg.wasm'
import memUrl from '../../../lib/ffmpeg/asm/ffmpeg.js.mem'
import { initEmscriptenModule } from '../util'

type FFModule = import('../../../lib/ffmpeg/ffmpeg').FFModule

export type ConvertOptions = {
  verbose?: boolean
}

let ffmpegModule: Promise<FFModule>
let asmFFmpegModule: Promise<FFModule>

const _convert = async (
  module: FFModule,
  data: ArrayBuffer,
  { verbose = false }: ConvertOptions
) => {
  try {
    const resultView = module.convert(new Uint8ClampedArray(data), verbose)
    const result = new Uint8ClampedArray(resultView)

    return result.buffer as ArrayBuffer
  } finally {
    module.free_result()
  }
}

const convert = async (data: ArrayBuffer, opts?: ConvertOptions) => {
  if (!ffmpegModule) {
    const ffmpeg = (await import('../../../lib/ffmpeg/ffmpeg')).default
    // eslint-disable-next-line require-atomic-updates
    ffmpegModule = initEmscriptenModule(ffmpeg, { wasmUrl })
  }

  const module = await ffmpegModule

  return _convert(module, data, opts)
}

const convertAsm = async (data: ArrayBuffer, opts?: ConvertOptions) => {
  if (!asmFFmpegModule) {
    const ffmpegAsm = (await import('../../../lib/ffmpeg/asm/ffmpeg')).default
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
