/* eslint-env node */
/* eslint-disable @typescript-eslint/no-var-requires */

const path = require('path')

const ASSETS_REGEX = /\.(?:js|css|wasm|mem)$/
const IMAGE_REGEX = /\.(?:png|gif|jpg|jpeg|webp|svg)$/

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
    'gatsby-plugin-sharp',
    'gatsby-transformer-sharp',
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
    {
      resolve: 'gatsby-plugin-offline',
      options: {
        workboxConfig: {
          cacheId: 'convideo',
          skipWaiting: true,
          runtimeCaching: [
            {
              // google stylesheets handler
              urlPattern: /^https:\/\/fonts\.googleapis\.com/,
              handler: 'StaleWhileRevalidate',
            },
            {
              // google web fonts handler
              urlPattern: /^https:\/\/fonts\.gstatic\.com/,
              handler: 'CacheFirst',
              options: {
                cacheableResponse: { statuses: [0, 200] },
                expiration: {
                  maxEntries: 20,
                  // cache for a year
                  maxAgeSeconds: 60 * 60 * 24 * 365,
                },
              },
            },
            {
              // image assets
              urlPattern: IMAGE_REGEX,
              handler: 'CacheFirst',
              options: {
                expiration: {
                  maxEntries: 60,
                  // cache for 30 days
                  maxAgeSeconds: 60 * 60 * 24 * 30,
                },
              },
            },
            {
              urlPattern: ASSETS_REGEX,
              handler:
                process.env.NODE_ENV === 'development'
                  ? 'NetworkOnly'
                  : 'StaleWhileRevalidate',
            },
            process.env.NODE_ENV === 'development' && {
              urlPattern: /(__webpack_hmr|hot-update)/,
              handler: 'NetworkOnly',
            },
            {
              urlPattern: /^https:\/\/(www\.)?convideo\.app/,
              handler: 'NetworkFirst',
            },
          ].filter(Boolean),
        },
      },
    },
  ],
}
