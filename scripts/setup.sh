#!/bin/bash

set -euo pipefail

apt-get update

apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    clang \
    libabsl-dev \
    libre2-dev \
    pkg-config

rm -rf /var/lib/apt/lists/*