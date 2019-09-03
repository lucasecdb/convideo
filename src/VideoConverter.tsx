import classNames from 'classnames'
import React, { useState } from 'react'

import Button from './components/Button'
import Checkbox from './components/Checkbox'
import FormField from './components/FormField'
import CircularProgress from './components/CircularProgress'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import { convert } from './ffmpeg'
import Icon from './components/Icon'
import { downloadFile } from 'utils'

import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const [loading, setLoading] = useState(false)
  const [asmEnabled, setAsmEnabled] = useState(false)
  const videoUrl = URL.createObjectURL(video)

  const handleCheckboxToggle = () => {
    setAsmEnabled(prevChecked => !prevChecked)
  }

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setLoading(true)

    try {
      const convertedVideoBuffer = await convert(videoArrayBuffer, {
        asm: asmEnabled,
      })

      downloadFile('output.mkv', convertedVideoBuffer)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="pa3 flex flex-column">
      <Button
        icon={<Icon icon="close" />}
        className="self-start"
        onClick={onClose}
      >
        Close
      </Button>
      <div className="flex flex-column flex-row-l mt3">
        <VideoPlayer className={styles.videoRoot} src={videoUrl} />
        <div
          className={classNames(
            styles.optionsContainer,
            'flex flex-column ml0 mt3 mt0-l ml3-l'
          )}
        >
          <t.Subtitle1>Conversion options</t.Subtitle1>
          <FormField
            input={
              <Checkbox
                nativeControlId="asm"
                name="asm"
                onChange={handleCheckboxToggle}
                checked={asmEnabled}
              />
            }
            label={<span>Use asm.js</span>}
            inputId="asm"
          />
        </div>
      </div>
      <Button
        disabled={loading}
        raised
        className="mt3 f6 self-center"
        onClick={handleConvert}
      >
        {loading ? (
          <CircularProgress className={styles.spinner} size={18} />
        ) : (
          'Convert'
        )}
      </Button>
    </div>
  )
}

export default VideoConverter
