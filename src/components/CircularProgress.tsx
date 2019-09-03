import classNames from 'classnames'
import React from 'react'

import styles from './CircularProgress.module.scss'

interface Props {
  className?: string
  size?: number
}

const CircularProgress: React.FC<Props> = ({ className, size = 28 }) => {
  return (
    <div
      className={classNames(className, styles.spinner)}
      style={{ ['--size' as any]: `${size}px` }}
    >
      <div className={styles.spinnerContainer}>
        <div className={styles.spinnerLayer}>
          <div
            className={`${styles.spinnerCircleClipper} ${styles.spinnerLeft}`}
          >
            <div className={styles.spinnerCircle}></div>
          </div>
          <div className={styles.spinnerGapPatch}>
            <div className={styles.spinnerCircle}></div>
          </div>
          <div
            className={`${styles.spinnerCircleClipper} ${styles.spinnerRight}`}
          >
            <div className={styles.spinnerCircle}></div>
          </div>
        </div>
      </div>
    </div>
  )
}

export default CircularProgress
