#!/bin/bash
#
# About: Setup wasmtime, a runtime for WebAssembly (outside browser).
#

# It's annoying me that the project recommends to run a downloaded script to
# setup, but it's a PhD research... And always run it in a isolated VM.
curl https://wasmtime.dev/install.sh -sSf | bash
rustup target add wasm32-wasi
