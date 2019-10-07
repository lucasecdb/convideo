var Module = {}

Module["convert"] = function(data, opts) {
  FS.mkdir('/work')
  FS.chdir('/work')

  const fd = FS.open('input', "w+")
  FS.write(fd, data, 0, data.length)
  FS.close(fd)

  Module.run([
    'ffmpeg',
    '-loglevel',
    opts.verbose ? 'verbose' : 'quiet',
    '-i',
    'input',
    '-vcodec',
    opts.videoCodec,
    '-acodec',
    opts.audioCodec,
    '-f',
    opts.outputFormat,
    'output'
  ])

  function listFiles(dir) {
    const contents = FS.lookupPath(dir).node.contents

    return Object
      .keys(contents)
      .map(filename => contents[filename])
  }

  const outputFile = listFiles("/work").find(file => file.name === 'output')

  return outputFile.content
}
