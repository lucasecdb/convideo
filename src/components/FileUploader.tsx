import { t } from '@lucasecdb/rmdc'
import classNames from 'classnames'
import React from 'react'
import { useDropzone } from 'react-dropzone'

import Logo from './Logo'

import styles from './FileUploader.module.scss'

interface Props {
  className?: string
  onFile: (file: File) => void
  onError?: (errorMessage: string) => void
}

const FileUploader: React.FC<Props> = ({
  className,
  onFile,
  onError = () => {},
}) => {
  const { isDragActive, getRootProps, getInputProps } = useDropzone({
    accept: 'video/*',
    onDrop: (acceptedFiles, fileRejections) => {
      const uploadedFiles = acceptedFiles.length + fileRejections.length

      if (uploadedFiles > 1) {
        onError('Select only one file at a time')
        return
      }

      if (fileRejections.length > 0) {
        const [fileRejection] = fileRejections

        onError(`Invalid file format '${fileRejection.file.type}'`)
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
        className,
        'outline-0 relative flex flex-column items-center justify-center',
        {
          [styles.containerDragActive]: isDragActive,
        }
      )}
    >
      <input {...getInputProps()} className="dn" />
      <Logo white className={styles.logo} />
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
