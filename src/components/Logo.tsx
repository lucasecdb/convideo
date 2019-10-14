import React from 'react'

import fullWhiteLogo from '../assets/logo-full-white.svg'
import typeWhiteLogo from '../assets/logo-type-white.svg'
import typeLogo from '../assets/logo-type.svg'
import logo from '../assets/logo.svg'

interface Props {
  className: string
  size: number
  white: boolean
  type: boolean
}

const Logo: React.FC<Props> = props => {
  const { className = '', size = undefined, white, type } = props

  let url = logo

  if (white) {
    if (type) {
      url = typeWhiteLogo
    } else {
      url = fullWhiteLogo
    }
  } else if (type) {
    url = typeLogo
  }

  return (
    <img
      className={className}
      width={size}
      height={size}
      src={url}
      alt="Convideo"
    />
  )
}

export default Logo
