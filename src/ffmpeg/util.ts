type ModuleFactory<M extends EmscriptenModule> = (
  opts: Partial<EmscriptenModule>
) => M

export function initEmscriptenModule<T extends EmscriptenModule>(
  moduleFactory: ModuleFactory<T>,
  { wasmUrl, memUrl }: { wasmUrl?: string; memUrl?: string },
  opts: Partial<EmscriptenModule> = {}
): Promise<T> {
  return new Promise(resolve => {
    const module = moduleFactory({
      ...opts,
      // Just to be safe, don't automatically invoke any wasm functions
      noInitialRun: true,
      locateFile(url: string): string {
        // Redirect the request for the wasm binary to whatever webpack gave us.
        if (url.endsWith('.wasm') && wasmUrl) {
          return wasmUrl
        }
        if (url.endsWith('.mem') && memUrl) {
          return memUrl
        }
        return url
      },
      onRuntimeInitialized() {
        // An Emscripten is a then-able that resolves with itself, causing an infite loop when you
        // wrap it in a real promise. Delete the `then` prop solves this for now.
        // https://github.com/kripken/emscripten/issues/5820
        delete (module as any).then
        resolve(module)
      },
    })
  })
}
