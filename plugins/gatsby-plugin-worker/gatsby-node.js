/* eslint-env node */
/* eslint-disable @typescript-eslint/no-var-requires */

const WorkerPlugin = require('worker-plugin')

exports.onCreateWebpackConfig = ({ actions }) => {
  actions.setWebpackConfig({
    plugins: [
      new WorkerPlugin({
        globalObject: 'self',
      }),
    ],
  })
}
