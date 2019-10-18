/* eslint-env node */
/* eslint-disable @typescript-eslint/no-var-requires */

const path = require('path')
const WorkerPlugin = require('worker-plugin')

exports.onCreateWebpackConfig = ({ actions, getConfig, plugins }) => {
  const config = getConfig()

  const libPath = path.resolve(__dirname, 'lib')

  config.module.rules = [
    {
      oneOf: [
        // Make sure we use this rule for files in `lib`
        {
          test: /\.js$/,
          include: [libPath],
          type: 'javascript/auto',
        },
        {
          // Emscripten modules don't work with Webpack's Wasm loader.
          test: /\.wasm$/,
          exclude: /_bg\.wasm$/,
          // This is needed to make webpack NOT process wasm files.
          // See https://github.com/webpack/webpack/issues/6725
          type: 'javascript/auto',
          loader: 'file-loader',
          options: {
            name: '[name].[hash:5].[ext]',
          },
        },
        {
          test: /\.mem$/,
          loader: 'file-loader',
          options: {
            name: '[name].[hash:5].[ext]',
          },
        },
        {
          rules: config.module.rules,
        },
      ],
    },
  ]

  config.plugins = [
    ...config.plugins,
    new WorkerPlugin({
      globalObject: 'self',
    }),
  ]

  if (config.optimization) {
    config.optimization.minimizer = [
      plugins.minifyJs({ test: /(!lib\/.*)/ }),
      ...config.optimization.minimizer,
    ]
  }

  config.node = {
    module: 'empty',
    dgram: 'empty',
    dns: 'mock',
    fs: 'empty',
    http2: 'empty',
    net: 'empty',
    tls: 'empty',
    child_process: 'empty',
  }

  actions.replaceWebpackConfig(config)
}
