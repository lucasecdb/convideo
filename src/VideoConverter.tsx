import classNames from 'classnames'
import React, { useState, useEffect } from 'react'

import Button from './components/Button'
import Checkbox from './components/Checkbox'
import FormField from './components/FormField'
import CircularProgress from './components/CircularProgress'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import { convert, listCodecs, CodecDescription } from './ffmpeg'
import Icon from './components/Icon'
import { downloadFile } from 'utils'

import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const [convertInProgress, setConvertInProgress] = useState(false)
  const [codecs, setCodecs] = useState<CodecDescription[] | null>(null)
  const [asmEnabled, setAsmEnabled] = useState(false)
  const [verbose, setVerbose] = useState(false)

  const videoUrl = URL.createObjectURL(video)

  const handleAsmToggle = () => {
    setAsmEnabled(prevChecked => !prevChecked)
  }

  const handleVerboseToggle = () => {
    setVerbose(prevVerbose => !prevVerbose)
  }

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setConvertInProgress(true)

    try {
      const start = performance.now()

      const convertedVideoBuffer = await convert(videoArrayBuffer, {
        asm: asmEnabled,
        verbose,
        outputFormat: 'mov',
        videoEncoder: 'libx264',
        audioEncoder: 'aac',
      })

      const end = performance.now()

      const elapsedTime = (end - start) / 1000

      console.log(`Conversion duration: ${elapsedTime.toFixed(2)} seconds`)

      downloadFile('output.mov', convertedVideoBuffer)
    } finally {
      setConvertInProgress(false)
    }
  }

  useEffect(() => {
    let current = true

    const load = async () => {
      const loadedCodecs = await listCodecs()

      if (!current) {
        return
      }

      setCodecs(loadedCodecs)
    }

    load()

    return () => {
      current = false
    }
  }, [])

  if (codecs === null) {
    return (
      <div className="flex flex-column items-center justify-center min-vh-100">
        <CircularProgress size={48} />
        <t.Subtitle1 className="mt3">Loading codecs</t.Subtitle1>
      </div>
    )
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
                onChange={handleAsmToggle}
                checked={asmEnabled}
              />
            }
            label={<span>Use asm.js</span>}
            inputId="asm"
          />
          <FormField
            input={
              <Checkbox
                nativeControlId="verbose"
                name="verbose"
                onChange={handleVerboseToggle}
                checked={verbose}
              />
            }
            label={<span>Verbose (debug only)</span>}
            inputId="verbose"
          />
        </div>
      </div>
      <Button
        disabled={convertInProgress}
        raised
        className="mt3 f6 self-center"
        onClick={handleConvert}
      >
        {convertInProgress ? (
          <CircularProgress className={styles.spinner} size={18} />
        ) : (
          'Convert'
        )}
      </Button>
    </div>
  )
}

export default VideoConverter
