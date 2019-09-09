import classNames from 'classnames'
import React, { useState, useEffect } from 'react'

import Button from './components/Button'
import Checkbox from './components/Checkbox'
import FormField from './components/FormField'
import CircularProgress from './components/CircularProgress'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import { convert, listCodecs, Codec, listMuxers, Muxer } from './ffmpeg'
import Icon from './components/Icon'
import { downloadFile } from 'utils'

import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const [convertInProgress, setConvertInProgress] = useState(false)
  const [codecs, setCodecs] = useState<Codec[] | null>(null)
  const [muxers, setMuxers] = useState<Muxer[] | null>(null)
  const [asmEnabled, setAsmEnabled] = useState(false)
  const [verbose, setVerbose] = useState(false)

  const [selectedFormat, setSelectedFormat] = useState<number>(0)
  const [selectedVideoCodec, setSelectedVideoCodec] = useState<number>(0)
  const [selectedAudioCodec, setSelectedAudioCodec] = useState<number>(0)

  const videoUrl = URL.createObjectURL(video)

  const handleAsmToggle = () => {
    setAsmEnabled(prevChecked => !prevChecked)
  }

  const handleVerboseToggle = () => {
    setVerbose(prevVerbose => !prevVerbose)
  }

  const format = muxers ? muxers[selectedFormat] : null

  const videoCodecs = codecs ? codecs.filter(codec => codec.type === 0) : []
  const videoCodec = videoCodecs ? videoCodecs[selectedVideoCodec] : null

  const audioCodecs = codecs ? codecs.filter(codec => codec.type === 1) : []
  const audioCodec = audioCodecs ? audioCodecs[selectedAudioCodec] : null

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setConvertInProgress(true)

    try {
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

      if (convertedVideoBuffer === null) {
        return
      }

      const [defaultExtension] = format.extensions
      const outputFilename =
        'output' + (defaultExtension.length ? `.${defaultExtension}` : '')

      downloadFile(outputFilename, convertedVideoBuffer)
    } catch (err) {
      console.error(err)
      setConvertInProgress(false)
    } finally {
      setConvertInProgress(false)
    }
  }

  useEffect(() => {
    let current = true

    const load = async () => {
      const [loadedCodecs, loadedMuxers] = await Promise.all([
        listCodecs(),
        listMuxers(),
      ])

      if (!current) {
        return
      }

      setCodecs(loadedCodecs)
      setMuxers(loadedMuxers)
    }

    load()

    return () => {
      current = false
    }
  }, [])

  if (codecs === null || muxers === null) {
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
              onChange={evt => setSelectedFormat(Number(evt.target.value))}
            >
              {muxers.map((muxer, index) => (
                <option key={muxer.name} value={index}>
                  {muxer.longName}
                </option>
              ))}
            </select>
          </label>

          <label className="flex flex-column mt3">
            <t.Caption>Video Codec</t.Caption>
            <select
              value={selectedVideoCodec}
              onChange={evt => setSelectedVideoCodec(Number(evt.target.value))}
            >
              {videoCodecs.map((codec, index) => (
                <option key={index} value={index}>
                  {codec.longName}
                </option>
              ))}
            </select>
          </label>

          <label className="flex flex-column mt3">
            <t.Caption>Audio Codec</t.Caption>
            <select
              value={selectedAudioCodec}
              onChange={evt => setSelectedAudioCodec(Number(evt.target.value))}
            >
              {audioCodecs.map((codec, index) => (
                <option key={index} value={index}>
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
