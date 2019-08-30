import React, { useState } from 'react'

import FileUploader from './FileUploader'

import './App.css'

const App: React.FC = () => {
  const [file, setFile] = useState<File | null>(null)

  const handleFile = (inputFile: File) => {
    setFile(inputFile)
  }

  return (
    <div>
      <FileUploader onFile={handleFile} />
    </div>
  )
}

export default App
