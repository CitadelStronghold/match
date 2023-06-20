#!/bin/bash

mkdir -p build

cmake -S cxx -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
