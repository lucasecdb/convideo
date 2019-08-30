import classNames from 'classnames'
import React from 'react'

import styles from './Button.module.scss'

const Button: React.FC<React.ButtonHTMLAttributes<HTMLButtonElement>> = ({
  children,
  className,
  ...props
}) => {
  const classes = classNames(className, styles.button)

  return (
    <button className={classes} {...props}>
      {children}
    </button>
  )
}

export default Button
