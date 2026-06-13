#!/bin/bash

set -euo pipefail

./build/src/runner/runner --run-query "$1" "$2" > "$3" 2> "$4"
