import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/convert.wasm'
import memUrl from '../../../lib/ffmpeg/asm/convert.js.mem'
import { initEmscriptenModule, ModuleFactory } from '../util'

type FFModule = import('../../../lib/ffmpeg/convert').FFModule
export type ConvertOptions = import('../../../lib/ffmpeg/convert').ConvertOptions

export interface Codec {
  id: number
  name: string
  longName: string
  capabilities: {
    intraOnly: boolean
    lossless: boolean
    experimental: boolean
    hardware: boolean
    hybrid: boolean
  }
  type: number
}

class Worker {
  private _wasmModule: Promise<FFModule> | undefined
  private _asmModule: Promise<FFModule> | undefined

  get wasm() {
    if (!this._wasmModule) {
      return new Promise<ModuleFactory<FFModule>>(resolve => {
        import('../../../lib/ffmpeg/convert').then(
          ({ default: defaultModule }) => resolve(defaultModule)
        )
      }).then(module => {
        this._wasmModule = initEmscriptenModule(module, { wasmUrl })

        return this._wasmModule
      })
    }

    return this._wasmModule
  }

  get asm() {
    if (!this._asmModule) {
      return new Promise<ModuleFactory<FFModule>>(resolve => {
        import('../../../lib/ffmpeg/asm/convert').then(
          ({ default: defaultModule }) => resolve(defaultModule)
        )
      }).then(module => {
        this._asmModule = initEmscriptenModule(module, { memUrl })

        return this._asmModule
      })
    }

    return this._asmModule
  }

  private _convert = async (
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

  public convert = async (data: ArrayBuffer, opts: ConvertOptions) => {
    const module = await this.wasm

    return this._convert(module, data, opts)
  }

  public convertAsm = async (data: ArrayBuffer, opts: ConvertOptions) => {
    const module = await this.asm

    return this._convert(module, data, opts)
  }

  public listCodecs = async () => {
    const module = await this.wasm

    const codecsVector = module.list_codecs()

    const codecs: Codec[] = []

    for (let i = 0; i < codecsVector.size(); i++) {
      const rawCodec = codecsVector.get(i)

      if (!rawCodec.id) {
        continue
      }

      codecs.push({
        id: rawCodec.id.value,
        name: rawCodec.name,
        capabilities: {
          intraOnly: !!(rawCodec.capabilities & module.AV_CODEC_CAP_INTRA_ONLY),
          lossless: !!(rawCodec.capabilities & module.AV_CODEC_CAP_LOSSLESS),
          experimental: !!(
            rawCodec.capabilities & module.AV_CODEC_CAP_EXPERIMENTAL
          ),
          hardware: !!(rawCodec.capabilities & module.AV_CODEC_CAP_HARDWARE),
          hybrid: !!(rawCodec.capabilities & module.AV_CODEC_CAP_HYBRID),
        },
        longName: rawCodec.long_name,
        type: rawCodec.type.value,
      })
    }

    return codecs
  }

  public listMuxers = async () => {}
}

export type FFmpegWorkerAPI = typeof Worker

expose(Worker, self as any)
