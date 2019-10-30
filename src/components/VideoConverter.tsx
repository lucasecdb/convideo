import {
  Button,
  Caption,
  Checkbox,
  CircularProgress,
  FormField,
  Icon,
  Subtitle1,
} from '@lucasecdb/rmdc'
import classNames from 'classnames'
import React, { useEffect, useState } from 'react'

import VideoPlayer from './VideoPlayer'
import {
  Codec,
  Muxer,
  convert,
  listEncoders,
  listMuxers,
  retrieveMetrics,
} from '../ffmpeg'
import { downloadFile } from '../utils/index'

import styles from './VideoConverter.module.scss'

interface Props {
  video: File
  onClose?: () => void
}

const VideoConverter: React.FC<Props> = ({ video, onClose }) => {
  const [conversionInProgress, setConversionInProgress] = useState(false)
  const [convertedVideos, setConvertedVideos] = useState(0)
  const [advancedOptionsOpen, setAdvancedOptionsOpen] = useState(false)

  const [encoders, setEncoders] = useState<Codec[] | null>(null)
  const [muxers, setMuxers] = useState<Muxer[] | null>(null)

  const [asmEnabled, setAsmEnabled] = useState(false)
  const [verbose, setVerbose] = useState(false)
  const [skipDownload, setSkipDownload] = useState(false)
  const [batchMode, setBatchMode] = useState(false)
  const [batchNumber, setBatchNumber] = useState(1)

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

  const handleBatchModeToggle = () => {
    setBatchMode(prevBatchMode => !prevBatchMode)
  }

  const handleBatchNumberChange = (
    evt: React.ChangeEvent<HTMLInputElement>
  ) => {
    const value = Number(evt.target.value)

    if (value <= 0) {
      return
    }

    setBatchNumber(value)
  }

  const handleAdvancedOptionsToggle = () => {
    setAdvancedOptionsOpen(prevOpen => !prevOpen)
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
      'filename,elapsed_time,input_size,output_size,format,video_codec,audio_codec,wasm,index'

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
          (metric.wasm ? 1 : 0) +
          ',' +
          metric.index
      )
      .join('\n')

    const csv = header + '\n' + body

    downloadFile('metrics.csv', csv)
  }

  const handleConvert = async () => {
    const videoArrayBuffer = await new Response(video).arrayBuffer()

    setConversionInProgress(true)
    setConvertedVideos(0)

    if (batchMode) {
      for (let i = 0; i < batchNumber - 1; i++) {
        // eslint-disable-next-line no-await-in-loop
        await convert(videoArrayBuffer, video.name, {
          asm: asmEnabled,
          verbose,
          outputFormat: format.name,
          videoEncoder: videoCodec.name,
          audioEncoder: audioCodec.name,
        })

        setConvertedVideos(prev => prev + 1)
      }
    }

    const convertedVideoBuffer = await convert(videoArrayBuffer, video.name, {
      asm: asmEnabled,
      verbose,
      outputFormat: format.name,
      videoEncoder: videoCodec.name,
      audioEncoder: audioCodec.name,
    })

    setConversionInProgress(false)

    if (convertedVideoBuffer === null || !convertedVideoBuffer.byteLength) {
      return
    }

    if (skipDownload) {
      return
    }

    const videoNameParts = video.name.split('.')
    const filenameWithoutExtension = videoNameParts
      .splice(0, videoNameParts.length - 1)
      .join('.')

    const [defaultExtension] = format.extensions
    const outputFilename = defaultExtension.length
      ? `${filenameWithoutExtension}.${defaultExtension}`
      : filenameWithoutExtension

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
        <Subtitle1 className="mt3">Loading codecs and formats</Subtitle1>
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
          <Subtitle1>Conversion options</Subtitle1>

          <label className="flex flex-column mt3">
            <Caption>Format</Caption>
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
            <Caption>Video Codec</Caption>
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
            <Caption>Audio Codec</Caption>
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

          <button
            id="advanced-options"
            className="bg-transparent color-inherit bn ma0 outline-0 mt3 pv2 pointer flex items-center"
            aria-controls="advanced-options-sect"
            aria-expanded={advancedOptionsOpen}
            onClick={handleAdvancedOptionsToggle}
          >
            <Icon
              aria-hidden="true"
              icon={
                advancedOptionsOpen
                  ? 'keyboard_arrow_up'
                  : 'keyboard_arrow_down'
              }
            />
            <Caption>Advanced options</Caption>
          </button>

          <div
            id="advanced-options-sect"
            className={classNames('flex-column', { flex: advancedOptionsOpen })}
            role="region"
            aria-labelledby="advanced-options"
            hidden={!advancedOptionsOpen}
          >
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

            <div className="flex items-center">
              <FormField
                input={
                  <Checkbox
                    nativeControlId="batchMode"
                    name="batchMode"
                    onChange={handleBatchModeToggle}
                    checked={batchMode}
                  />
                }
                label={<span>Batch mode </span>}
                inputId="batchMode"
              />
              {batchMode && (
                <input
                  className={classNames(
                    styles.numberInput,
                    'bg-transparent color-inherit ml2 pv1 ph2'
                  )}
                  type="number"
                  value={batchNumber}
                  onChange={handleBatchNumberChange}
                  placeholder="Batch number"
                />
              )}
            </div>

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
      </div>
      <Button
        disabled={conversionInProgress}
        raised
        className="mt3 f6 self-center"
        onClick={handleConvert}
      >
        {conversionInProgress ? (
          <>
            {batchMode && (
              <span className="dib mr2">
                {convertedVideos} / {batchNumber}
              </span>
            )}
            <CircularProgress className={styles.spinner} size={18} />
          </>
        ) : (
          'Convert'
        )}
      </Button>
    </div>
  )
}

export default VideoConverter
