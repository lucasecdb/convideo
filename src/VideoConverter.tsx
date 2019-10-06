import classNames from 'classnames'
import React, { useEffect, useState } from 'react'

import Button from './components/Button'
import Checkbox from './components/Checkbox'
import FormField from './components/FormField'
import CircularProgress from './components/CircularProgress'
import * as t from './components/Typography'
import VideoPlayer from './VideoPlayer'
import {
  Codec,
  Muxer,
  convert,
  listEncoders,
  listMuxers,
  retrieveMetrics,
} from './ffmpeg'
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
  const [skipDownload, setSkipDownload] = useState(false)

  const [selectedFormat, setSelectedFormat] = useState<string>('matroska')
  const [selectedVideoCodec, setSelectedVideoCodec] = useState<string>('mpeg4')
  const [selectedAudioCodec, setSelectedAudioCodec] = useState<string>('aac')

  const videoUrl = URL.createObjectURL(video)

  const handleAsmToggle = () => {
    setAsmEnabled(prevChecked => !prevChecked)
  }

  const handleVerboseToggle = () => {
    setVerbose(prevVerbose => !prevVerbose)
  }

  const handleSkipDownloadToggle = () => {
    setSkipDownload(prevSkip => !prevSkip)
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

  const handleMetricsDownload = async () => {
    const metrics = await retrieveMetrics()

    const header =
      'filename,elapsed_time,input_size,output_size,format,video_codec,audio_codec,wasm'

    const body = metrics
      .map(
        metric =>
          JSON.stringify(metric.file) +
          ',' +
          metric.elapsedTime +
          ',' +
          metric.inputSize +
          ',' +
          metric.outputSize +
          ',' +
          JSON.stringify(metric.format) +
          ',' +
          JSON.stringify(metric.videoCodec) +
          ',' +
          JSON.stringify(metric.audioCodec) +
          ',' +
          (metric.wasm ? 1 : 0)
      )
      .join('\n')

    const csv = header + '\n' + body

    downloadFile('metrics.csv', csv)
  }

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setConvertInProgress(true)

    const convertedVideoBuffer = await convert(videoArrayBuffer, video.name, {
      asm: asmEnabled,
      verbose,
      outputFormat: format.name,
      videoEncoder: videoCodec.name,
      audioEncoder: audioCodec.name,
    })

    setConvertInProgress(false)

    if (convertedVideoBuffer === null || !convertedVideoBuffer.byteLength) {
      return
    }

    if (skipDownload) {
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

          <FormField
            input={
              <Checkbox
                nativeControlId="skipDownload"
                name="skipDownload"
                onChange={handleSkipDownloadToggle}
                checked={skipDownload}
              />
            }
            label={<span>Skip download</span>}
            inputId="skipDownload"
          />

          <Button
            className="mt3"
            onClick={handleMetricsDownload}
            dense
            type="button"
          >
            Download metrics
          </Button>
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
