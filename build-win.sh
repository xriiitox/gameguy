#!/usr/bin/env bash
cmake -GNinja -S . -B build/windows -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake && \
cmake --build build/windows -j 8
