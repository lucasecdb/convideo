import React, { useState } from 'react'

import Dialog, {
  DialogTitle,
  DialogContent,
  DialogActions,
} from './components/Dialog'
import Button from './components/Button'
import FileUploader from './FileUploader'
import VideoConverter from './VideoConverter'

import './App.css'

const FILE_SIZE_LIMIT = 200 * 1024 * 1024 // 200 Mb

const App: React.FC = () => {
  const [file, setFile] = useState<File | null>(null)
  const [showFileSizeError, setShowFileSizeError] = useState(false)

  const handleFile = async (inputFile: File) => {
    if (inputFile.size > FILE_SIZE_LIMIT) {
      setShowFileSizeError(true)
      return
    }
    setFile(inputFile)
  }

  const handleDialogClose = () => {
    setShowFileSizeError(false)
  }

  return file ? (
    <VideoConverter video={file} onClose={() => setFile(null)} />
  ) : (
    <>
      <FileUploader onFile={handleFile} />
      <Dialog
        open={showFileSizeError}
        onClose={handleDialogClose}
        role="alertdialog"
      >
        <DialogTitle>File too large</DialogTitle>
        <DialogContent>
          The size of the uploaded file exceeds our limit (200 Mb)
        </DialogContent>
        <DialogActions>
          <Button onClick={handleDialogClose}>Close</Button>
        </DialogActions>
      </Dialog>
    </>
  )
}

export default App
