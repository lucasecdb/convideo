import classNames from 'classnames'
import React from 'react'
import { useDropzone } from 'react-dropzone'

import { ReactComponent as Logo } from './assets/logo-full-white.svg'
import * as t from './components/Typography'
import styles from './FileUploader.module.scss'

interface Props {
  onFile: (file: File) => void
  onError?: (errorMessage: string) => void
}

const FileUploader: React.FC<Props> = ({ onFile, onError = () => {} }) => {
  const { isDragActive, getRootProps, getInputProps } = useDropzone({
    accept: 'video/*',
    onDrop: (acceptedFiles, rejectedFiles) => {
      const uploadedFiles = acceptedFiles.length + rejectedFiles.length

      if (uploadedFiles > 1) {
        onError('Select only one file at a time')
        return
      }

      if (rejectedFiles.length > 0) {
        const [file] = rejectedFiles

        onError(`Invalid file format '${file.type}'`)
        return
      }

      onFile(acceptedFiles[0])
    },
  })

  const { onClick, ...rootProps } = getRootProps()

  return (
    <div
      {...rootProps}
      tabIndex={-1}
      className={classNames(
        'outline-0 min-vh-100 h-100 relative flex flex-column items-center justify-center',
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
