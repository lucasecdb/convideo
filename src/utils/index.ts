export const downloadFile = (
  filename: string,
  data: ArrayBuffer | string,
  mimeType = 'application/octet-stream'
) => {
  const dataBlob = new Blob([data], { type: mimeType })

  // Edge and IE10+
  if (navigator.msSaveOrOpenBlob) {
    navigator.msSaveOrOpenBlob(dataBlob, filename)
  } else {
    const url = URL.createObjectURL(dataBlob)

    const anchor = document.createElement('a')

    if ('download' in anchor) {
      anchor.download = filename
      anchor.href = url

      document.body.appendChild(anchor)

      anchor.click()

      document.body.removeChild(anchor)

      URL.revokeObjectURL(url)
    } else {
      // fallback to load on iframe
      const frame = document.createElement('iframe')

      document.body.appendChild(frame)

      frame.src = url

      Promise.race([
        new Promise(resolve => frame.addEventListener('load', resolve)),
        new Promise(resolve =>
          setTimeout(
            resolve,
            333 /* magic number, same value used by download.js lib */
          )
        ),
      ]).then(() => {
        document.body.removeChild(frame)
      })
    }
  }
}
