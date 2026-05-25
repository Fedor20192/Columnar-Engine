#!/bin/bash
set -e
mkdir -p build-release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -G Ninja -DENABLE_PGO=GENERATE
cmake --build build-release -j4

./script/convert.sh tests/bench/sample.csv test
./build-release/src/executable/execute_query
for QUERY_NUM in {0..42}; do
  OUTPUT="/dev/null"
  LOGS="/dev/null"
  ./script/run_query.sh "${QUERY_NUM}" test "${OUTPUT}" "${LOGS}"
done

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=USE

cmake --build build-release -j4

