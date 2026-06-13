#!/bin/sh
# Build v1_dashboard natively on Tiny Core Linux (i686)

# 1. Install build dependencies
tce-load -wi gcc.tcz
tce-load -wi make.tcz
tce-load -wi cmake.tcz

# 2. Build
mkdir -p build_linux
cd build_linux
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
make -j1

echo "Done. Binary at: build_linux/dashboard_app"
