# WebAssembly device target

This target compiles the shared LVGL navigation UI to JavaScript and WebAssembly for the local browser debug shell. It is not a production web application. Configure it through the parent `tools/debug/moto-gps-simulator/CMakeLists.txt`; see [`shell/README.md`](../shell/README.md) for the pinned LVGL, Emscripten, and run steps.

The `moto-gps-device.js` and `.wasm` outputs are written to `shell/runtime/`, which is generated and ignored by Git.
