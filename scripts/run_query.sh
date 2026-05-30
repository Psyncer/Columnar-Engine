#!/bin/bash

set -euo pipefail

./build/tests/test_queries --run-query "$1" "$2" > "$3" 2> "$4"