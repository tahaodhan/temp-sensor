#!/bin/bash

# Set these based on your actual folder locations
WASI_SDK_PATH="$HOME/wasi-sdk"
WAMR_DIR="$HOME/wasm-micro-runtime"

echo "Build wasm app .."
$WASI_SDK_PATH/bin/clang -O3 \
    -z stack-size=4096 -Wl,--initial-memory=65536 \
    -o temp-run.wasm temp-sense.c \
    -Wl,--export=main -Wl,--export=__main_argc_argv \
    -Wl,--export=__data_end -Wl,--export=__heap_base \
    -Wl,--strip-all,--no-entry \
    -Wl,--allow-undefined \
    -nostdlib

echo "Build binarydump tool .."
rm -fr build && mkdir build && cd build
cmake $WAMR_DIR/test-tools/binarydump-tool
make
cd ..

echo "Generate test_wasm.h .."
./build/binarydump -o ../main/test_wasm.h -n wasm_test_file_interp temp-run.wasm

echo "Done"
