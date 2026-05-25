#!/bin/bash
set -e

QUERY_NUM=$1
COLUMNAR=$2
OUTPUT=$3
LOGS=$4

sudo nice -n -20 ./build-release/src/executable/execute_query "${QUERY_NUM}" "${COLUMNAR}" > "${OUTPUT}" 2> "${LOGS}"
