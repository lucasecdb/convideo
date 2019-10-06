/* env: webworker */

import { precacheAndRoute } from 'workbox-precaching'
import { CacheableResponsePlugin } from 'workbox-cacheable-response'
import { ExpirationPlugin } from 'workbox-expiration'
import {
  CacheFirst,
  NetworkFirst,
  NetworkOnly,
  StaleWhileRevalidate,
} from 'workbox-strategies'
import { registerRoute } from 'workbox-routing'
import { skipWaiting } from 'workbox-core'

const ASSETS_REGEX = /\.(?:js|css|wasm|mem)$/
const IMAGE_REGEX = /\.(?:png|gif|jpg|jpeg|webp|svg)$/

declare global {
  interface Window {
    __WB_MANIFEST: { url: string; revision: string }[]
  }
}

precacheAndRoute(self.__WB_MANIFEST, {})
skipWaiting()

const networkFirstHandler = new NetworkFirst()

// api
registerRoute(/_c/, networkFirstHandler.handle.bind(networkFirstHandler))

// google fonts
const googleStylesheetsHandler = new StaleWhileRevalidate({
  cacheName: 'google-fonts-stylesheets',
})

registerRoute(
  /^https:\/\/fonts\.googleapis\.com/,
  googleStylesheetsHandler.handle.bind(googleStylesheetsHandler)
)

const googleWebfontsHandler = new CacheFirst({
  cacheName: 'google-fonts-webfonts',
  plugins: [
    new CacheableResponsePlugin({
      statuses: [0, 200],
    }),
    new ExpirationPlugin({
      maxEntries: 20,
      // cache for a year
      maxAgeSeconds: 60 * 60 * 24 * 365,
    }),
  ],
})

registerRoute(
  /^https:\/\/fonts\.gstatic\.com/,
  googleWebfontsHandler.handle.bind(googleWebfontsHandler)
)

// image assets
const imagesHandler = new CacheFirst({
  cacheName: 'images',
  plugins: [
    new ExpirationPlugin({
      maxEntries: 60,
      // cache for 30 days
      maxAgeSeconds: 60 * 60 * 24 * 30,
    }),
  ],
})

registerRoute(IMAGE_REGEX, imagesHandler.handle.bind(imagesHandler))

// javascript and css
const assetsHandler = new (process.env.NODE_ENV === 'development'
  ? NetworkOnly
  : StaleWhileRevalidate)({
  cacheName: 'static-resources',
})
registerRoute(ASSETS_REGEX, assetsHandler.handle.bind(assetsHandler))

if (process.env.NODE_ENV === 'development') {
  const hmrHandler = new NetworkOnly()

  registerRoute(
    /(__webpack_hmr|hot-update)/,
    hmrHandler.handle.bind(hmrHandler)
  )
} else {
  registerRoute(
    /^https:\/\/(www\.)?convideo\.app/,
    networkFirstHandler.handle.bind(networkFirstHandler)
  )
}
