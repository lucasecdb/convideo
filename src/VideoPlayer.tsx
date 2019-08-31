import React from 'react'

interface Props extends React.VideoHTMLAttributes<HTMLVideoElement> {
  src: string
}

const VideoPlayer: React.FC<Props> = ({ src, ...props }) => {
  return (
    <video {...props} controls>
      <source src={src} />
    </video>
  )
}

export default VideoPlayer
