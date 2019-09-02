import { wrap } from 'comlink'

type FFmpegWorkerAPI = import('./worker').FFmpegWorkerAPI

let worker: Worker
let workerAPI: FFmpegWorkerAPI

export async function convert(data: ArrayBuffer) {
  if (!worker) {
    worker = new Worker('./worker', { name: 'ffmpeg-worker', type: 'module' })
    workerAPI = wrap<FFmpegWorkerAPI>(worker)
  }

  return workerAPI.convert(data)
}
