import React, { useState } from 'react'

import FileUploader from './FileUploader'
import VideoConverter from './VideoConverter'

import './App.css'

const App: React.FC = () => {
  const [file, setFile] = useState<File | null>(null)

  const handleFile = async (inputFile: File) => {
    setFile(inputFile)
  }

  return file ? (
    <VideoConverter video={file} onClose={() => setFile(null)} />
  ) : (
    <FileUploader onFile={handleFile} />
  )
}

export default App
