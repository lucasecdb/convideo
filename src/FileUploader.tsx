import classNames from 'classnames'
import React from 'react'
import { useDropzone } from 'react-dropzone'

import { ReactComponent as Logo } from './assets/logo-full-white.svg'
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
      tabIndex={-1}
      className={classNames(
        'min-vh-100 h-100 relative flex flex-column items-center justify-center',
        {
          [styles.containerDragActive]: isDragActive,
        }
      )}
    >
      <input {...getInputProps()} className="dn" />
      <Logo className={styles.logo} />
      <t.Headline3 className="mt5 tc">
        Drag & drop or{' '}
        <button className={styles.selectButton} onClick={onClick}>
          select a video
        </button>
      </t.Headline3>
    </div>
  )
}

export default FileUploader
