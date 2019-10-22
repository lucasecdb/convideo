import { expose } from 'comlink'

import wasmUrl from '../../../lib/ffmpeg/convert.wasm'
import memUrl from '../../../lib/ffmpeg/asm/convert.js.mem'
import { ModuleFactory, initEmscriptenModule } from '../util'

type FFModule = import('../../../lib/ffmpeg/convert').FFModule
type RawCodecOption = import('../../../lib/ffmpeg/convert').CodecOption
type RawMuxer = import('../../../lib/ffmpeg/convert').Muxer
export type ConvertOptions = import('../../../lib/ffmpeg/convert').ConvertOptions

export interface Vector<T> {
  get(index: number): T
  size(): number
}

export interface CodecOption {
  name: string
  help: string
  unit: string
  offset: number
  type: number
  defaultValue: any
  min: number
  max: number
  flags: {
    encodingParam: boolean
    decodingParam: boolean
    audioParam: boolean
    videoParam: boolean
    subtitleParam: boolean
    export: boolean
    readOnly: boolean
    bsfParam: boolean
    filteringParam: boolean
    deprecated: boolean
  }
}

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
  type: string
  options: CodecOption[]
}

export interface Muxer {
  name: string
  longName: string
  mimeType: string
  extensions: string[]
  audioCodec?: number
  videoCodec?: number
}

export interface Metric {
  elapsedTime: number
  file: string
  inputSize: number
  outputSize: number
  format: string
  videoCodec: string
  audioCodec: string
  wasm: boolean
  index: number
}

interface RunInfo {
  wasm: number
  asm: number
}

const vectorToArray = <T>(vector: Vector<T>): T[] => {
  const a = []

  for (let i = 0; i < vector.size(); i++) {
    a.push(vector.get(i))
  }

  return a
}

export type FileRuntimeMap = Record<string, RunInfo>

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
    filename: string,
    opts: ConvertOptions,
    id: number,
    wasm: boolean
  ) => {
    try {
      const start = performance.now()
      const resultView = instance.convert(new Uint8ClampedArray(data), opts)
      const end = performance.now()

      const elapsedTime = (end - start) / 1000

      const result = new Uint8ClampedArray(resultView)

      const runtimeMetrics: Metric = {
        elapsedTime,
        file: filename,
        inputSize: data.byteLength,
        outputSize: result.byteLength,
        format: opts.outputFormat,
        videoCodec: opts.videoEncoder,
        audioCodec: opts.audioEncoder,
        index: id,
        wasm,
      }

      return { data: result.buffer as ArrayBuffer, metrics: runtimeMetrics }
    } catch (err) {
      console.error(err)

      return { data: null }
    }
  }

  public convert = async (
    id: number,
    data: ArrayBuffer,
    filename: string,
    opts: ConvertOptions
  ) => {
    const wasm = await this.wasm

    return this._convert(wasm, data, filename, opts, id, true)
  }

  public convertAsm = async (
    id: number,
    data: ArrayBuffer,
    filename: string,
    opts: ConvertOptions
  ) => {
    const asm = await this.asm

    return this._convert(asm, data, filename, opts, id, false)
  }

  public listEncoders = async () => {
    const wasm = await this.wasm

    const codecsVector = wasm.list_encoders()

    const codecs: Codec[] = vectorToArray(codecsVector)
      .filter(
        (rawCodec, index, array) =>
          index ===
          array.findIndex(codec => codec.id.value === rawCodec.id.value)
      )
      .map(rawCodec => {
        const codecOptions = vectorToArray<RawCodecOption>(
          wasm.list_codec_options(rawCodec.id.value)
        ).map(rawOption => ({
          name: rawOption.name,
          help: rawOption.help,
          unit: rawOption.unit,
          offset: rawOption.offset,
          type: this.codecTypeToString(wasm, rawOption.type.value),
          min: rawOption.min,
          max: rawOption.max,
          defaultValue: rawOption.default_val,
          flags: {
            encodingParam: !!(
              rawOption.flags & wasm.AV_OPT_FLAG_ENCODING_PARAM
            ),
            decodingParam: !!(
              rawOption.flags & wasm.AV_OPT_FLAG_DECODING_PARAM
            ),
            audioParam: !!(rawOption.flags & wasm.AV_OPT_FLAG_AUDIO_PARAM),
            videoParam: !!(rawOption.flags & wasm.AV_OPT_FLAG_VIDEO_PARAM),
            subtitleParam: !!(
              rawOption.flags & wasm.AV_OPT_FLAG_SUBTITLE_PARAM
            ),
            export: !!(rawOption.flags & wasm.AV_OPT_FLAG_EXPORT),
            readOnly: !!(rawOption.flags & wasm.AV_OPT_FLAG_READONLY),
            bsfParam: !!(rawOption.flags & wasm.AV_OPT_FLAG_BSF_PARAM),
            filteringParam: !!(
              rawOption.flags & wasm.AV_OPT_FLAG_FILTERING_PARAM
            ),
            deprecated: !!(rawOption.flags & wasm.AV_OPT_FLAG_DEPRECATED),
          },
        }))

        return {
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
          options: codecOptions,
        }
      })

    return codecs
  }

  public listMuxers = async () => {
    const wasm = await this.wasm

    const muxerVector = wasm.list_muxers()

    const muxers: Muxer[] = vectorToArray<RawMuxer>(muxerVector).map(
      rawMuxer => ({
        name: rawMuxer.name,
        longName: rawMuxer.long_name,
        mimeType: rawMuxer.mime_type,
        extensions: rawMuxer.extensions.split(','),
        audioCodec: rawMuxer.audio_codec && rawMuxer.audio_codec.value,
        videoCodec: rawMuxer.video_codec && rawMuxer.video_codec.value,
      })
    )

    return muxers
  }

  private codecTypeToString = (instance: FFModule, type: number) => {
    switch (type) {
      case instance.AVOptionType.AV_OPT_TYPE_FLAGS.value:
        return 'flags'
      case instance.AVOptionType.AV_OPT_TYPE_INT.value:
        return 'int'
      case instance.AVOptionType.AV_OPT_TYPE_INT64.value:
        return 'int64'
      case instance.AVOptionType.AV_OPT_TYPE_DOUBLE.value:
        return 'double'
      case instance.AVOptionType.AV_OPT_TYPE_FLOAT.value:
        return 'float'
      case instance.AVOptionType.AV_OPT_TYPE_STRING.value:
        return 'string'
      case instance.AVOptionType.AV_OPT_TYPE_RATIONAL.value:
        return 'rational'
      case instance.AVOptionType.AV_OPT_TYPE_BINARY.value:
        return 'binary'
      case instance.AVOptionType.AV_OPT_TYPE_DICT.value:
        return 'dict'
      case instance.AVOptionType.AV_OPT_TYPE_UINT64.value:
        return 'uint64'
      case instance.AVOptionType.AV_OPT_TYPE_CONST.value:
        return 'const'
      case instance.AVOptionType.AV_OPT_TYPE_IMAGE_SIZE.value:
        return 'image_size'
      case instance.AVOptionType.AV_OPT_TYPE_PIXEL_FMT.value:
        return 'pixel_fmt'
      case instance.AVOptionType.AV_OPT_TYPE_SAMPLE_FMT.value:
        return 'sample_fmt'
      case instance.AVOptionType.AV_OPT_TYPE_VIDEO_RATE.value:
        return 'video_rate'
      case instance.AVOptionType.AV_OPT_TYPE_DURATION.value:
        return 'duration'
      case instance.AVOptionType.AV_OPT_TYPE_COLOR.value:
        return 'color'
      case instance.AVOptionType.AV_OPT_TYPE_CHANNEL_LAYOUT.value:
        return 'channel_layout'
      case instance.AVOptionType.AV_OPT_TYPE_BOOL.value:
        return 'boolean'
      default:
        throw new Error('Invalid type: ' + type)
    }
  }
}

export type WorkerAPI = FFmpeg

expose(FFmpeg, self as any)
