import classNames from 'classnames'
import React, { useRef, useState } from 'react'
import { fromEvent } from 'file-selector'

import * as t from './components/Typography'
import styles from './FileUploader.module.scss'

interface Props {
  onFile: (file: File) => void
}

const FileUploader: React.FC<Props> = ({ onFile }) => {
  const rootRef = useRef<HTMLDivElement>(null)
  const inputRef = useRef<HTMLInputElement>(null)
  const [dragActive, setDragActive] = useState(false)
  const [dragTargets, setDragTargets] = useState<EventTarget[]>([])

  const resetFileInput = () => {
    inputRef.current!.value = ''
  }

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const fileInput = e.target
    const file = fileInput.files && fileInput.files[0]

    if (!file) {
      return
    }

    resetFileInput()
    onFile(file)
  }

  const handleFileSelect = () => {
    inputRef.current!.click()
  }

  const handleDragEnter = (evt: React.DragEvent<HTMLDivElement>) => {
    evt.preventDefault()
    // persist since we are including the target in
    // the state
    evt.persist()

    if (dragTargets.indexOf(evt.target) === -1) {
      setDragTargets(prevTargets => [...prevTargets, evt.target])
    }
    setDragActive(true)
  }

  const handleDragLeave = (evt: React.DragEvent<HTMLDivElement>) => {
    evt.preventDefault()
    evt.stopPropagation()

    const targets = dragTargets.filter(
      target =>
        target !== evt.target &&
        rootRef.current &&
        rootRef.current.contains(target as Node)
    )

    setDragTargets(targets)

    if (targets.length > 0) {
      // don't disable drag if we are in the children of the root
      // element
      return
    }

    setDragActive(false)
  }

  const handleDragOver = (evt: React.DragEvent<HTMLDivElement>) => {
    evt.preventDefault()
    evt.stopPropagation()
  }

  const handleDrop = async (evt: React.DragEvent<HTMLDivElement>) => {
    evt.preventDefault()
    evt.persist()
    evt.stopPropagation()

    setDragActive(false)
    setDragTargets([])

    const [maybeFile] = await fromEvent(evt.nativeEvent)

    if (!maybeFile) {
      return
    }

    let file: File | null

    if ('getAsFile' in maybeFile) {
      file = maybeFile.getAsFile()
    } else {
      file = maybeFile
    }

    if (file == null) {
      return
    }

    onFile(file)
  }

  return (
    <div
      ref={rootRef}
      className={classNames(styles.container, {
        [styles.containerDrag]: dragActive,
      })}
      onDragEnter={handleDragEnter}
      onDragOver={handleDragOver}
      onDragLeave={handleDragLeave}
      onDrop={handleDrop}
    >
      <t.Headline2 className={styles.intro}>
        Drag & drop, or{' '}
        <button className={styles.selectButton} onClick={handleFileSelect}>
          select a video
        </button>
        <input
          className="dn"
          ref={inputRef}
          accept="video/*"
          autoComplete="off"
          type="file"
          tabIndex={-1}
          onChange={handleFileChange}
        />
      </t.Headline2>
    </div>
  )
}

export default FileUploader
