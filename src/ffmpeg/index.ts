import { wrap } from 'comlink'

type WorkerAPI = import('./worker').WorkerAPI
type ConvertOptions = import('./worker').ConvertOptions

export type Codec = import('./worker').Codec
export type Muxer = import('./worker').Muxer

type Metric = import('./worker').Metric
type FileRuntimeMap = import('./worker').FileRuntimeMap

interface Options extends ConvertOptions {
  asm?: boolean
}

const fileRunMap: FileRuntimeMap = {}
const runtimeMetrics: Metric[] = []

const getAPI = (() => {
  let worker: Worker
  let workerAPI: Promise<WorkerAPI>

  return async (reset = false) => {
    if (!worker || reset) {
      worker = new Worker('./worker', { name: 'ffmpeg-worker', type: 'module' })
      const FFmpeg = wrap<WorkerAPI>(worker)
      // @ts-ignore
      workerAPI = new FFmpeg()
    }

    return workerAPI
  }
})()

export async function convert(
  inputData: ArrayBuffer,
  filename: string,
  { asm = false, ...opts }: Options
) {
  const api = await getAPI(true)

  if (!fileRunMap[filename]) {
    fileRunMap[filename] = {
      wasm: 0,
      asm: 0,
    }
  }

  if (asm) {
    const id = fileRunMap[filename].asm++
    const { data, metrics } = await api.convertAsm(
      id,
      inputData,
      filename,
      opts
    )

    runtimeMetrics.push(metrics)

    return data
  }

  const id = fileRunMap[filename].wasm++
  const { data, metrics } = await api.convert(id, inputData, filename, opts)

  runtimeMetrics.push(metrics)

  return data
}

export async function listEncoders() {
  const api = await getAPI()

  return api.listEncoders()
}

export async function listMuxers() {
  const api = await getAPI()

  return api.listMuxers()
}

export async function retrieveMetrics() {
  return runtimeMetrics
}
