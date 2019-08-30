import React, { useState } from 'react'

import FileUploader from './FileUploader'
import VideoConverter from './VideoConverter'

import './App.css'

const App: React.FC = () => {
  const [fileBuffer, setFileBuffer] = useState<ArrayBuffer | null>(null)

  const handleFile = async (inputFile: File) => {
    // arrayBuffer is not defined in dom lib, but it
    // exists per the spec: https://developer.mozilla.org/en-US/docs/Web/API/Blob/arrayBuffer
    const buffer = await ((inputFile as unknown) as Body).arrayBuffer()

    setFileBuffer(buffer)
  }

  return (
    <div>
      {fileBuffer ? (
        <VideoConverter
          video={fileBuffer}
          onClose={() => setFileBuffer(null)}
        />
      ) : (
        <FileUploader onFile={handleFile} />
      )}
    </div>
  )
}

export default App
