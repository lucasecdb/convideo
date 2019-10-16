import { graphql, useStaticQuery } from 'gatsby'
import React from 'react'

interface Props {
  className?: string
  size?: number
  white?: boolean
  type?: boolean
  icon?: boolean
}

const Logo: React.FC<Props> = ({
  className = '',
  size = undefined,
  white = false,
  type = false,
  icon = false,
}) => {
  const {
    logo,
    whiteLogo,
    typeLogo,
    typeWhiteLogo,
    iconLogo,
  } = useStaticQuery(graphql`
    query {
      iconLogo: file(relativePath: { eq: "logo-icon.svg" }) {
        publicURL
      }
      logo: file(relativePath: { eq: "logo.svg" }) {
        publicURL
      }
      whiteLogo: file(relativePath: { eq: "logo-white.svg" }) {
        publicURL
      }
      typeLogo: file(relativePath: { eq: "logo-type.svg" }) {
        publicURL
      }
      typeWhiteLogo: file(relativePath: { eq: "logo-type-white.svg" }) {
        publicURL
      }
    }
  `)

  let image = logo

  if (white) {
    if (type) {
      image = typeWhiteLogo
    } else {
      image = whiteLogo
    }
  } else if (type) {
    image = typeLogo
  } else if (icon) {
    image = iconLogo
  }

  return (
    <img
      className={className}
      width={size}
      height={size}
      src={image.publicURL}
      alt="Convideo"
    />
  )
}

export default Logo
