#!/bin/bash

set -euo pipefail

./build/tests/test_queries --convert tests/schema_sample.csv "$1" "$2"