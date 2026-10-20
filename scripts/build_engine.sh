#!/usr/bin/env bash
set -e

echo "=== Building PTXScope C++ Compiler Engine ==="
cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=clang++ || cmake -B build -DCMAKE_CXX_COMPILER=g++
cmake --build build

echo "=== Engine build complete ==="
echo "Executable: build/engine/ptxscope"
echo "Tests:      build/engine/tests/ptxscope_tests"
