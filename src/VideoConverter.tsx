import classNames from 'classnames'
import React from 'react'

import Button from './components/Button'
import VideoPlayer from './VideoPlayer'
import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const videoUrl = URL.createObjectURL(video)

  return (
    <div className="pa3 flex flex-column">
      <Button className="self-start" onClick={onClose}>
        Go Back
      </Button>
      <VideoPlayer
        className={classNames(styles.player, 'mt3')}
        src={videoUrl}
      />
      <h3 className="mt3">Conversion options</h3>
    </div>
  )
}

export default VideoConverter
