This directory is gathers all third-party libs that need to be compiled to WebAssembly
before use in the application.

They all follow a similar structure, which is to run the build command inside a docker container,
so you need to have downloaded the `trzeci/emscripten` image. If you don't already have it, download it

```bash
docker pull trzeci/emscripten
```

> If you use Linux, you may have to use `sudo` to run the docker command

And now you should be able to build all of them by running the following inside each directory

```bash
yarn
yarn build
```

Remember to also commit the `.wasm` and `.js` builded files.
