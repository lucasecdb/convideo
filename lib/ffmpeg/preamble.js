/* global FS, Module, callMain */
/* eslint-disable no-var */

var initializedFS = false
var DATA_DIR = '/data'

function convertToUint8Array(data) {
  if (Array.isArray(data) || data instanceof ArrayBuffer) {
    data = new Uint8Array(data)
  } else if (!data) {
    // `null` for empty files.
    data = new Uint8Array(0)
  } else if (!(data instanceof Uint8Array)) {
    // Avoid unnecessary copying.
    data = new Uint8Array(data.buffer)
  }
  return data
}

Module['convert'] = function(data, opts) {
  if (!initializedFS) {
    FS.mkdir(DATA_DIR)
    FS.chdir(DATA_DIR)
    initializedFS = true
  }

  var fd = FS.open('input', 'w+')
  FS.write(fd, data, 0, data.length)
  FS.close(fd)

  var args = [
    '-nostdin',
    '-loglevel',
    opts['verbose'] ? 'verbose' : 'quiet',
    '-i',
    'input',
    '-vcodec',
    opts['videoEncoder'],
    '-acodec',
    opts['audioEncoder'],
    '-f',
    opts['outputFormat'],
    'output',
  ]

  callMain(args)

  var outputFile = FS.lookupPath(DATA_DIR).node.contents['output']

  return convertToUint8Array(outputFile.content)
}
