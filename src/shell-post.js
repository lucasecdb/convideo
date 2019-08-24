    return Module
  })

  return __ffmpegInitPromise
}

if (typeof exports === 'object' && typeof module === 'object') {
  module.exports = __initFFMPEG
  // This will allow the module to be used in ES6 or CommonJS
  module.exports.default = __initFFMPEG
} else if (typeof define === 'function' && define['amd']) {
  define([], function() { return __initFFMPEG });
} else if (typeof exports === 'object') {
  exports["Module"] = __initFFMPEG
} else if (typeof window !== 'undefined') {
  window['FFMPEG'] = __initFFMPEG
}
})()
