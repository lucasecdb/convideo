import { wrap } from 'comlink'

type WorkerAPI = import('./worker').WorkerAPI
type ConvertOptions = import('./worker').ConvertOptions

export type Codec = import('./worker').Codec
export type Muxer = import('./worker').Muxer

interface Options extends ConvertOptions {
  asm?: boolean
}

const getAPI = (() => {
  let worker: Worker
  let workerAPI: Promise<WorkerAPI>

  return async () => {
    if (!worker) {
      worker = new Worker('./worker', { name: 'ffmpeg-worker', type: 'module' })
      const FFmpeg = wrap<WorkerAPI>(worker)
      // @ts-ignore
      workerAPI = new FFmpeg()
    }

    return workerAPI
  }
})()

export async function convert(
  data: ArrayBuffer,
  { asm = false, ...opts }: Options
) {
  const api = await getAPI()

  if (asm) {
    return api.convertAsm(data, opts)
  }

  return api.convert(data, opts)
}

export async function listEncoders() {
  const api = await getAPI()

  return api.listEncoders()
}

export async function listMuxers() {
  const api = await getAPI()

  return api.listMuxers()
}
