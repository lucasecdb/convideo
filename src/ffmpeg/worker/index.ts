import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/convert.wasm'
import memUrl from '../../../lib/ffmpeg/asm/convert.js.mem'
import { initEmscriptenModule } from '../util'

type FFModule = import('../../../lib/ffmpeg/convert').FFModule
export type ConvertOptions = import('../../../lib/ffmpeg/convert').ConvertOptions

export interface CodecDescription {
  id: number
  name: string
  longName: string
  props: number
  type: number
}

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

const listCodecs = async () => {
  if (!ffmpegModule) {
    const ffmpeg = (await import('../../../lib/ffmpeg/convert')).default
    // eslint-disable-next-line require-atomic-updates
    ffmpegModule = initEmscriptenModule(ffmpeg, { wasmUrl })
  }

  const module = await ffmpegModule

  const codecsVector = module.list_codecs()

  const codecs: CodecDescription[] = []

  for (let i = 0; i < codecsVector.size(); i++) {
    const rawCodec = codecsVector.get(i)

    if (!rawCodec.id) {
      continue
    }

    codecs.push({
      id: rawCodec.id.value,
      name: rawCodec.name,
      props: rawCodec.props,
      longName: rawCodec.long_name,
      type: rawCodec.type.value,
    })
  }

  return codecs
}

const exports = {
  convert,
  convertAsm,
  listCodecs,
}

export type FFmpegWorkerAPI = typeof exports

expose(exports, self as any)
