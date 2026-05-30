#!/bin/bash
set -e
mkdir -p build-release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build-release

