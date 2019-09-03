import { wrap } from 'comlink'

type FFmpegWorkerAPI = import('./worker').FFmpegWorkerAPI

interface Options {
  asm?: boolean
}

const defaultOptions: Options = {
  asm: false,
}

let worker: Worker
let workerAPI: FFmpegWorkerAPI

export async function convert(
  data: ArrayBuffer,
  opts: Options = defaultOptions
) {
  if (!worker) {
    worker = new Worker('./worker', { name: 'ffmpeg-worker', type: 'module' })
    workerAPI = wrap<FFmpegWorkerAPI>(worker)
  }

  if (opts.asm) {
    return workerAPI.convertAsm(data)
  }

  return workerAPI.convert(data)
}
