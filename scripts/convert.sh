#!/bin/bash

set -euo pipefail

./build/tests/test_queries --convert tests/schema_sample.csv tests/hits.csv tests/big.columnar
