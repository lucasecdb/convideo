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

  const { onClick, ...rootProps } = getRootProps()

  return (
    <div
      {...rootProps}
      className={classNames('min-vh-100 h-100 relative', styles.container, {
        [styles.containerDragActive]: isDragActive,
      })}
    >
      <input {...getInputProps()} className="dn" />
      <t.Headline2 className={classNames(styles.intro, 'tc')}>
        Drag & drop or{' '}
        <button className={styles.selectButton} onClick={onClick}>
          select a video
        </button>
      </t.Headline2>
    </div>
  )
}

export default FileUploader
