# PTXScope Performance & Scalability Benchmarks

This directory contains the automated evaluation suite for measuring the parsing throughput, static analysis latency, and optimization efficiency of the PTXScope C++ compiler engine.

## Methodology & Execution

The benchmark harness (`benchmark_runner.py`):
1. Evaluates all 6 sample programs (`01_vector_add.ptx` through `06_load_store.ptx`).
2. Evaluates synthetic scaling kernels (50, 200, 500, and 1,000 instructions) to measure linear asymptotic scaling.
3. Repeats measurements 5 times per target to calculate mean wall-clock time and standard deviation ($\mu \pm \sigma$).
4. Produces a machine-readable JSON artifact in `benchmarks/results/benchmark_summary.json`.

## Running the Benchmarks

```powershell
python benchmarks/benchmark_runner.py
```

## Benchmark Metrics Captured

- **Initial Instructions**: Total instruction count before optimization.
- **Optimized Instructions**: Final instruction count following all passes.
- **Transformations**: Aggregate count of modifications performed across all passes.
- **Engine Latency (ms)**: Measured execution time of the internal C++ compiler passes.
- **Wall-Clock Latency (ms)**: Subprocess startup, IO streaming, and JSON serialization turnaround.

> **Note on GPU Runtime Performance**:
> Compiler benchmark figures measure the execution latency and throughput of the compiler itself. Reductions in PTX instruction counts do not imply an identical percentage reduction in GPU kernel runtime execution time due to microarchitectural factors including warp divergence, memory latency hiding, and hardware instruction scheduling.
