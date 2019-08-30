import React from 'react'

import Button from './components/Button'

interface Props {
  video: ArrayBuffer
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  return (
    <div>
      <Button onClick={onClose}>Go Back</Button>
      this is your video converter!
    </div>
  )
}

export default VideoConverter
