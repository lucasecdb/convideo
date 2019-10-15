/* eslint-env node */
/* eslint-disable @typescript-eslint/no-var-requires */

const path = require('path')

module.exports = {
  siteMetadata: {
    title: 'Convideo',
    description: 'Simple video converter tool',
    siteUrl: 'https://convideo.app/',
    author: 'Lucas Cordeiro',
  },
  plugins: [
    {
      resolve: 'gatsby-source-filesystem',
      options: {
        path: `${__dirname}/src/assets`,
        name: 'assets',
      },
    },
    'gatsby-plugin-typescript',
    'gatsby-plugin-react-helmet',
    'gatsby-transformer-sharp',
    'gatsby-plugin-sharp',
    {
      resolve: 'gatsby-plugin-sass',
      options: {
        includePaths: [path.resolve('node_modules')],
      },
    },
    {
      resolve: 'gatsby-plugin-google-fonts',
      options: {
        fonts: ['Roboto:300,400,500,600', 'Material+Icons'],
      },
    },
    {
      resolve: 'gatsby-plugin-manifest',
      options: {
        short_name: 'Convideo',
        name: 'Convideo',
        icons: [
          {
            src: 'favicon.ico',
            sizes: '48x48 32x32 24x24 16x16',
            type: 'image/x-icon',
          },
          {
            src: 'logo192.png',
            type: 'image/png',
            sizes: '192x192',
          },
          {
            src: 'logo512.png',
            type: 'image/png',
            sizes: '512x512',
          },
        ],
        start_url: '.',
        display: 'standalone',
        theme_color: '#121212',
        background_color: '#121212',
      },
    },
  ],
}
