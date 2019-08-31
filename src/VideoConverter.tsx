import classNames from 'classnames'
import React from 'react'

import Button from './components/Button'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import convert from './ffmpeg/convert'

import styles from './VideoConverter.module.scss'
import Icon from 'components/Icon'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const videoUrl = URL.createObjectURL(video)

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    const convertedVideoBuffer = await convert(videoArrayBuffer)

    console.log(convertedVideoBuffer)
  }

  return (
    <div className="pa3 flex flex-column">
      <Button
        icon={<Icon icon="arrow_back" />}
        className="self-start"
        onClick={onClose}
      >
        Go Back
      </Button>
      <div className="flex flex-column flex-row-l mt3">
        <VideoPlayer
          className={classNames(styles.player, 'w-100')}
          src={videoUrl}
        />
        <div
          className={classNames(
            styles.optionsContainer,
            'flex flex-column w-100'
          )}
        >
          <t.Subtitle1>Conversion options</t.Subtitle1>
        </div>
      </div>
      <Button raised className="mt3 f6 self-center" onClick={handleConvert}>
        Convert
      </Button>
    </div>
  )
}

export default VideoConverter
