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
    drawHorizBand: boolean
    dr1: boolean
    truncated: boolean
    delay: boolean
    subFrames: boolean
    experimental: boolean
    intraOnly: boolean
    lossless: boolean
    hardware: boolean
    hybrid: boolean
  }
  type: number
}

export interface Muxer {
  name: string
  longName: string
  mimeType: string
  extensions: string[]
  audioCodec?: number
  videoCodec?: number
}

class FFmpeg {
  private _wasmModule: Promise<FFModule> | undefined
  private _asmModule: Promise<FFModule> | undefined

  private get wasm() {
    if (!this._wasmModule) {
      return new Promise<ModuleFactory<FFModule>>(resolve => {
        import('../../../lib/ffmpeg/convert').then(
          ({ default: defaultValue }) => resolve(defaultValue)
        )
      }).then(instance => {
        this._wasmModule = initEmscriptenModule(instance, { wasmUrl })

        return this._wasmModule
      })
    }

    return this._wasmModule
  }

  private get asm() {
    if (!this._asmModule) {
      return new Promise<ModuleFactory<FFModule>>(resolve => {
        import('../../../lib/ffmpeg/asm/convert').then(
          ({ default: defaultValue }) => resolve(defaultValue)
        )
      }).then(instance => {
        this._asmModule = initEmscriptenModule(instance, { memUrl })

        return this._asmModule
      })
    }

    return this._asmModule
  }

  private _convert = async (
    instance: FFModule,
    data: ArrayBuffer,
    opts: ConvertOptions
  ) => {
    try {
      const resultView = instance.convert(new Uint8ClampedArray(data), opts)
      const result = new Uint8ClampedArray(resultView)

      instance.free_result()

      return result.buffer as ArrayBuffer
    } catch (_) {
      instance.free_result()

      return null
    }
  }

  public convert = async (data: ArrayBuffer, opts: ConvertOptions) => {
    const wasm = await this.wasm

    return this._convert(wasm, data, opts)
  }

  public convertAsm = async (data: ArrayBuffer, opts: ConvertOptions) => {
    const asm = await this.asm

    return this._convert(asm, data, opts)
  }

  public listCodecs = async () => {
    const wasm = await this.wasm

    const codecsVector = wasm.list_codecs()

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
          drawHorizBand: !!(
            rawCodec.capabilities & wasm.AV_CODEC_CAP_DRAW_HORIZ_BAND
          ),
          dr1: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_DR1),
          truncated: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_TRUNCATED),
          delay: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_DELAY),
          subFrames: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_SUBFRAMES),
          experimental: !!(
            rawCodec.capabilities & wasm.AV_CODEC_CAP_EXPERIMENTAL
          ),
          intraOnly: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_INTRA_ONLY),
          lossless: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_LOSSLESS),
          hardware: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_HARDWARE),
          hybrid: !!(rawCodec.capabilities & wasm.AV_CODEC_CAP_HYBRID),
        },
        longName: rawCodec.long_name,
        type: rawCodec.type.value,
      })
    }

    return codecs
  }

  public listMuxers = async () => {
    const wasm = await this.wasm

    const muxerVector = wasm.list_muxers()

    const muxers: Muxer[] = []

    for (let i = 0; i < muxerVector.size(); i++) {
      const rawMuxer = muxerVector.get(i)

      muxers.push({
        name: rawMuxer.name,
        longName: rawMuxer.long_name,
        mimeType: rawMuxer.mime_type,
        extensions: rawMuxer.extensions.split(','),
        audioCodec: rawMuxer.audio_codec && rawMuxer.audio_codec.value,
        videoCodec: rawMuxer.video_codec && rawMuxer.video_codec.value,
      })
    }

    return muxers
  }
}

export type WorkerAPI = FFmpeg

expose(FFmpeg, self as any)
