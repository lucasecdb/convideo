/* global FS, Module, callMain */
/* eslint-disable no-var */

var initializedFS = false
var DATA_DIR = '/data'

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

  return outputFile.content
}
