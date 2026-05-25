#!/bin/bash
set -e
mkdir -p build-release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -G Ninja -DENABLE_PGO=GENERATE
cmake --build build-release -j4

./script/convert.sh tests/bench/sample.csv aboba
./build-release/src/executable/execute_query 2> /dev/null > /dev/null
./build-release/src/executable/extract_table aboba bebra

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=USE

cmake --build build-release -j4

