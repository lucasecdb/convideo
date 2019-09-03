import { wrap } from 'comlink'

type FFmpegWorkerAPI = import('./worker').FFmpegWorkerAPI
type ConvertOptions = import('./worker').ConvertOptions

interface Options extends ConvertOptions {
  asm?: boolean
}

let worker: Worker
let workerAPI: FFmpegWorkerAPI

export async function convert(
  data: ArrayBuffer,
  { asm = false, ...opts }: Options
) {
  if (!worker) {
    worker = new Worker('./worker', { name: 'ffmpeg-worker', type: 'module' })
    workerAPI = wrap<FFmpegWorkerAPI>(worker)
  }

  if (asm) {
    return workerAPI.convertAsm(data, opts)
  }

  return workerAPI.convert(data, opts)
}
