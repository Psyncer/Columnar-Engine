#!/bin/bash

set -euo pipefail

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build . --target test_queries -j"$(nproc)"

cd ..