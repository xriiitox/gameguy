#!/usr/bin/env bash
cmake -GNinja -S . -B build && \
cmake --build build -j 8
