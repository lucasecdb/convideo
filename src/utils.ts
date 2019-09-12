export const downloadFile = (filename: string, data: ArrayBuffer) => {
  const dataBlob = new Blob([data])
  const url = URL.createObjectURL(dataBlob)

  const anchor = document.createElement('a')
  anchor.download = filename
  anchor.href = url

  document.body.appendChild(anchor)

  anchor.click()

  document.body.removeChild(anchor)

  URL.revokeObjectURL(url)
}
