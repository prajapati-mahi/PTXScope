#!/usr/bin/env bash
set -e

echo "=== Running C++ Engine Unit Tests ==="
./build/engine/tests/ptxscope_tests

echo "=== Running Backend Integration Tests ==="
PYTHONPATH=backend python3 -m pytest backend/tests -v

echo "=== Running Frontend Typecheck & Build ==="
(cd frontend && npm run build)

echo "=== Running Compiler Benchmarks ==="
python3 benchmarks/benchmark_runner.py

echo "=== ALL TESTS & BENCHMARKS PASSED ==="
