#!/bin/bash

set -euo pipefail

#for Q in {0..42}; do
for Q in {31..31}; do
    OUTPUT=/home/mike/Columnar-Engine/small/query_${Q}.csv
    LOGS=/home/mike/Columnar-Engine/small/query_${Q}.log
    ./build/tests/test_queries --run-query "${Q}" tests/big.columnar
done
