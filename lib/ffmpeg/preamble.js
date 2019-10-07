/* global FS, Module */
/* eslint-disable no-var */

var initializedFS = false

Module['convert'] = function(data, opts) {
  if (!initializedFS) {
    FS.mkdir('/work')
    FS.chdir('/work')
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

  Module['_main'](args.length, args)

  function listFiles(dir) {
    var contents = FS.lookupPath(dir).node.contents

    return Object.keys(contents).map(function(filename) {
      return contents[filename]
    })
  }

  var outputFile = listFiles('/work').find(function(file) {
    return file.name === 'output'
  })

  return outputFile.content
}
