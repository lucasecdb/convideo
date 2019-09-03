import classNames from 'classnames'
import React from 'react'
import { useDropzone } from 'react-dropzone'

import * as t from './components/Typography'
import styles from './FileUploader.module.scss'

interface Props {
  onFile: (file: File) => void
}

const FileUploader: React.FC<Props> = ({ onFile }) => {
  const { isDragActive, getRootProps, getInputProps } = useDropzone({
    accept: 'video/*',
    onDropAccepted: files => {
      onFile(files[0])
    },
  })

  return (
    <div
      {...getRootProps()}
      className={classNames(styles.container, {
        [styles.containerDrag]: isDragActive,
      })}
    >
      <input {...getInputProps()} className="dn" />
      <t.Headline2 className={classNames(styles.intro, 'tc')}>
        Drag & drop, or{' '}
        <span className={styles.selectButton}>click to select a video</span>
      </t.Headline2>
    </div>
  )
}

export default FileUploader
