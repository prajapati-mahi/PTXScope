#!/usr/bin/env python3
"""
PTXScope Compiler Engine Benchmark & Evaluation Suite
Measures parsing time, analysis time, optimization throughput, transformation counts,
and instruction reduction across standard samples and synthetic scaling benchmarks.
"""

import os
import sys
import time
import json
import statistics
import subprocess
import platform
from pathlib import Path
from typing import Dict, Any, List

def locate_engine() -> str:
    root = Path(__file__).resolve().parent.parent
    candidates = [
        root / "build" / "engine" / "ptxscope.exe",
        root / "build" / "engine" / "ptxscope",
    ]
    for c in candidates:
        if c.exists():
            return str(c)
    raise FileNotFoundError("Could not locate compiled 'ptxscope' executable.")

def generate_synthetic_benchmark(size: int) -> str:
    lines = [
        ".version 7.0",
        ".target sm_70",
        ".address_size 64",
        f".entry synthetic_scale_{size}(.param .u64 ptr_out) {{",
        "  .reg .u64 %rd1;",
        f"  .reg .u32 %r<{size * 2 + 10}>;",
        "  ld.param.u64 %rd1, [ptr_out];",
        "  mov.u32 %r0, 1;"
    ]

    for i in range(1, size + 1):
        # Mix of foldable constant expressions, algebraic identities, redundant moves, and dead code
        op_type = i % 4
        if op_type == 0:
            lines.append(f"  add.u32 %r{i}, 10, {i};")  # Foldable
        elif op_type == 1:
            lines.append(f"  mul.lo.u32 %r{i}, %r{i-1}, 1;")  # Algebraic identity
        elif op_type == 2:
            lines.append(f"  mov.u32 %r{i}, %r{i};")  # Self move
        else:
            lines.append(f"  add.u32 %r{i}, %r{i-1}, %r0;")

    # Write output to store
    lines.append(f"  st.global.u32 [%rd1], %r{size};")
    lines.append("  ret;")
    lines.append("}")
    return "\n".join(lines)

def run_single_benchmark(engine_path: str, ptx_content: str, iterations: int = 5) -> Dict[str, Any]:
    temp_file = Path("temp_bench.ptx")
    temp_file.write_text(ptx_content, encoding="utf-8")

    durations: List[float] = []
    engine_metrics: Dict[str, Any] = {}
    pass_results: List[Any] = []

    # Setup environment with toolchain path for Windows
    env = os.environ.copy()
    local_app_data = os.getenv("LOCALAPPDATA", "")
    llvm_bin = os.path.join(
        local_app_data,
        r"Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
    )
    if os.path.exists(llvm_bin):
        env["PATH"] = llvm_bin + os.pathsep + env.get("PATH", "")

    try:
        for _ in range(iterations):
            start = time.perf_counter()
            res = subprocess.run(
                [engine_path, "json", str(temp_file)],
                capture_output=True,
                text=True,
                timeout=10,
                env=env
            )
            elapsed_ms = (time.perf_counter() - start) * 1000.0
            durations.append(elapsed_ms)

            if res.returncode == 0 and not engine_metrics:
                try:
                    data = json.loads(res.stdout)
                    engine_metrics = data.get("metrics", {})
                    pass_results = data.get("pass_results", [])
                except Exception:
                    pass

    finally:
        if temp_file.exists():
            temp_file.unlink()

    mean_ms = statistics.mean(durations)
    std_ms = statistics.stdev(durations) if len(durations) > 1 else 0.0

    total_transformations = sum(p.get("transformations", 0) for p in pass_results)

    return {
        "mean_wall_ms": round(mean_ms, 2),
        "std_wall_ms": round(std_ms, 2),
        "engine_internal_ms": round(engine_metrics.get("execution_duration_ms", 0.0), 2),
        "initial_instructions": engine_metrics.get("initial_instruction_count", 0),
        "optimized_instructions": engine_metrics.get("optimized_instruction_count", 0),
        "transformations": total_transformations,
        "basic_blocks": engine_metrics.get("basic_block_count", 0)
    }

def main():
    print("=" * 78)
    print("        PTXScope Compiler Engine - Performance Benchmark Suite")
    print("=" * 78)
    print(f"System:     {platform.system()} {platform.release()} ({platform.machine()})")
    print(f"Processor:  {platform.processor()}")
    print(f"Python:     {platform.python_version()}")

    engine = locate_engine()
    print(f"Engine:     {engine}\n")

    samples_dir = Path(__file__).resolve().parent.parent / "samples"
    sample_files = sorted(samples_dir.glob("*.ptx"))

    benchmarks_data: Dict[str, Any] = {
        "environment": {
            "os": platform.system(),
            "release": platform.release(),
            "arch": platform.machine(),
            "python": platform.python_version(),
            "engine": engine,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        },
        "samples": {},
        "synthetic": {}
    }

    print(f"{'Benchmark Target':<28} | {'Init Inst':<9} | {'Opt Inst':<8} | {'Transforms':<10} | {'Latency (ms)':<14}")
    print("-" * 78)

    # 1. Benchmark real sample programs
    for sample in sample_files:
        src = sample.read_text(encoding="utf-8")
        stats = run_single_benchmark(engine, src, iterations=5)
        benchmarks_data["samples"][sample.name] = stats

        lat_str = f"{stats['mean_wall_ms']:.2f} ± {stats['std_wall_ms']:.2f}"
        print(f"{sample.name:<28} | {stats['initial_instructions']:<9} | {stats['optimized_instructions']:<8} | {stats['transformations']:<10} | {lat_str:<14}")

    # 2. Benchmark scaling synthetic programs
    print("-" * 78)
    print("Synthetic Scaling Benchmarks:")
    for size in [50, 200, 500, 1000]:
        synth_src = generate_synthetic_benchmark(size)
        label = f"synthetic_scale_{size}"
        stats = run_single_benchmark(engine, synth_src, iterations=5)
        benchmarks_data["synthetic"][label] = stats

        lat_str = f"{stats['mean_wall_ms']:.2f} ± {stats['std_wall_ms']:.2f}"
        print(f"{label:<28} | {stats['initial_instructions']:<9} | {stats['optimized_instructions']:<8} | {stats['transformations']:<10} | {lat_str:<14}")

    print("=" * 78)

    # Save results to benchmarks/results/benchmark_summary.json
    results_dir = Path(__file__).resolve().parent / "results"
    results_dir.mkdir(parents=True, exist_ok=True)
    out_file = results_dir / "benchmark_summary.json"
    out_file.write_text(json.dumps(benchmarks_data, indent=2), encoding="utf-8")
    print(f"\n[OK] Machine-readable benchmark report saved to: {out_file}\n")

if __name__ == "__main__":
    main()
