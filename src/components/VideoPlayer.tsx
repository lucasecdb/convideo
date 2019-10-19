import classNames from 'classnames'
import React from 'react'

import styles from './VideoPlayer.module.scss'

interface Props extends React.VideoHTMLAttributes<HTMLVideoElement> {
  src: string
}

const VideoPlayer: React.FC<Props> = ({ src, className, ...props }) => {
  return (
    <div className={classNames(className, styles.container)}>
      <video {...props} className={styles.player} controls>
        <source src={src} />
      </video>
    </div>
  )
}

export default VideoPlayer
