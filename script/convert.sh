#!/bin/bash
set -e

INPUT_CSV=$1
COLUMNAR=$2

./build-release/src/executable/create_table tests/bench/scheme.csv "${INPUT_CSV}" "${COLUMNAR}"
