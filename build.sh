#!/bin/bash

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p $BUILD_DIR
fi

cd $BUILD_DIR

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo "Build complete. Output files:"
ls -lh *.elf *.hex *.bin 2>/dev/null || echo "No output files found"