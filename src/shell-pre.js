(function() {
var __ffmpegInitPromise = undefined

var __initFFMPEG = function (options) {
  if (__ffmpegInitPromise) {
    return __ffmpegInitPromise
  }

  __ffmpegInitPromise = new Promise(function (resolve, reject) {
    var Module = typeof options !== 'undefined' ? options : {}

    // EMCC only allows for a single onAbort function (not an array of functions)
    // So if the user defined their own onAbort function, we remember it and call it
    var originalOnAbortFunction = Module['onAbort']
    Module['onAbort'] = function (error) {
      reject(new Error(error))
      if (originalOnAbortFunction) {
        originalOnAbortFunction(error)
      }
    }

    Module['postRun'] = Module['postRun'] || []
    Module['postRun'].push(function () {
      // When Emscripted calls postRun, this promise resolves with the built Module
      resolve(Module)
    })

    // There is a section of code in the emcc-generated code below that looks like this:
    // (Note that this is lowercase `module`)
    // if (typeof module !== 'undefined') {
    //     module['exports'] = Module;
    // }
    // When that runs, it's going to overwrite our own modularization export efforts in shell-post.js!
    // The only way to tell emcc not to emit it is to pass the MODULARIZE=1 or MODULARIZE_INSTANCE=1 flags,
    // but that carries with it additional unnecessary baggage/bugs we don't want either.
    // So, we have three options:
    // 1) We undefine `module`
    // 2) We remember what `module['exports']` was at the beginning of this function and we restore it later
    // 3) We write a script to remove those lines of code as part of the Make process.
    //
    // Since those are the only lines of code that care about module, we will undefine it. It's the most straightforward
    // of the options, and has the side effect of reducing emcc's efforts to modify the module if its output were to change in the future.
    // That's a nice side effect since we're handling the modularization efforts ourselves
    module = undefined
