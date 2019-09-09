import classNames from 'classnames'
import React, { useState, useEffect } from 'react'

import Button from './components/Button'
import Checkbox from './components/Checkbox'
import FormField from './components/FormField'
import CircularProgress from './components/CircularProgress'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import { convert, listEncoders, Codec, listMuxers, Muxer } from './ffmpeg'
import Icon from './components/Icon'
import { downloadFile } from 'utils'

import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const [convertInProgress, setConvertInProgress] = useState(false)
  const [encoders, setEncoders] = useState<Codec[] | null>(null)
  const [muxers, setMuxers] = useState<Muxer[] | null>(null)
  const [asmEnabled, setAsmEnabled] = useState(false)
  const [verbose, setVerbose] = useState(false)

  const [selectedFormat, setSelectedFormat] = useState<string>('matroska')
  const [selectedVideoCodec, setSelectedVideoCodec] = useState<string>(
    'libx264'
  )
  const [selectedAudioCodec, setSelectedAudioCodec] = useState<string>('aac')

  const videoUrl = URL.createObjectURL(video)

  const handleAsmToggle = () => {
    setAsmEnabled(prevChecked => !prevChecked)
  }

  const handleVerboseToggle = () => {
    setVerbose(prevVerbose => !prevVerbose)
  }

  const format = muxers
    ? muxers.find(muxer => muxer.name === selectedFormat)
    : null

  const videoCodecs = encoders ? encoders.filter(codec => codec.type === 0) : []
  const videoCodec = videoCodecs
    ? videoCodecs.find(codec => codec.name === selectedVideoCodec)
    : null

  const audioCodecs = encoders ? encoders.filter(codec => codec.type === 1) : []
  const audioCodec = audioCodecs
    ? audioCodecs.find(codec => codec.name === selectedAudioCodec)
    : null

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setConvertInProgress(true)

    const start = performance.now()

    const convertedVideoBuffer = await convert(videoArrayBuffer, {
      asm: asmEnabled,
      verbose,
      outputFormat: format.name,
      videoEncoder: videoCodec.name,
      audioEncoder: audioCodec.name,
    })

    const end = performance.now()

    const elapsedTime = (end - start) / 1000

    console.log(`Conversion duration: ${elapsedTime.toFixed(2)} seconds`)

    setConvertInProgress(false)

    if (convertedVideoBuffer === null || !convertedVideoBuffer.byteLength) {
      return
    }

    const [defaultExtension] = format.extensions
    const outputFilename =
      'output' + (defaultExtension.length ? `.${defaultExtension}` : '')

    downloadFile(outputFilename, convertedVideoBuffer)
  }

  useEffect(() => {
    let current = true

    const load = async () => {
      const [loadedEncoders, loadedMuxers] = await Promise.all([
        listEncoders(),
        listMuxers(),
      ])

      if (!current) {
        return
      }

      setEncoders(loadedEncoders)
      setMuxers(loadedMuxers)
    }

    load()

    return () => {
      current = false
    }
  }, [])

  if (encoders === null || muxers === null) {
    return (
      <div className="flex flex-column items-center justify-center min-vh-100">
        <CircularProgress size={48} />
        <t.Subtitle1 className="mt3">Loading codecs and formats</t.Subtitle1>
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

          <label className="flex flex-column mt3">
            <t.Caption>Format</t.Caption>
            <select
              value={selectedFormat}
              onChange={evt => setSelectedFormat(evt.target.value)}
            >
              {muxers.map(muxer => (
                <option key={muxer.name} value={muxer.name}>
                  {muxer.longName}
                </option>
              ))}
            </select>
          </label>

          <label className="flex flex-column mt3">
            <t.Caption>Video Codec</t.Caption>
            <select
              value={selectedVideoCodec}
              onChange={evt => setSelectedVideoCodec(evt.target.value)}
            >
              {videoCodecs.map(codec => (
                <option key={codec.id} value={codec.name}>
                  {codec.longName}
                </option>
              ))}
            </select>
          </label>

          <label className="flex flex-column mt3">
            <t.Caption>Audio Codec</t.Caption>
            <select
              value={selectedAudioCodec}
              onChange={evt => setSelectedAudioCodec(evt.target.value)}
            >
              {audioCodecs.map(codec => (
                <option key={codec.id} value={codec.name}>
                  {codec.longName}
                </option>
              ))}
            </select>
          </label>

          <FormField
            className="mt3"
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
