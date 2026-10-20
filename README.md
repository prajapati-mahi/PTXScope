# PTXScope — GPU Assembly Analyzer, Optimizer & Debugging Workbench

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![FastAPI](https://img.shields.io/badge/FastAPI-1.0.0-009688.svg)](https://fastapi.tiangolo.com)
[![React](https://img.shields.io/badge/React-18-61DAFB.svg)](https://react.dev)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**PTXScope** is a static analysis, optimization, and debugging workbench for NVIDIA Parallel Thread Execution (PTX) assembly. Built from the ground up in standard C++17 with a FastAPI backend and a React/TypeScript/Monaco workbench frontend, PTXScope demonstrates compiler engineering fundamentals: lexical analysis, recursive-descent parsing, structured intermediate representation (IR), control flow graph (CFG) construction, backward data-flow liveness analysis, safe optimization passes, and rule-based performance diagnostics.

Designed specifically to showcase compiler systems engineering for a **Software Engineering Intern role on a PTX Compiler team**.

---

## Key Capabilities

1. **Native C++17 Compiler Engine**:
   - **Lexer & Parser**: Parses a strict, documented PTX v7.x–v8.x subset with error recovery and source span tracking.
   - **Intermediate Representation (IR)**: Structured `Module`, `Function`, `BasicBlock`, `Instruction`, and typed `Operand` models.
   - **Control Flow Graph (CFG)**: Computes basic block edges, conditional branch predicates, fallthrough paths, and reachability.
   - **Static & Liveness Analysis**: Def-use chains and fixed-point backward data-flow liveness analysis ($\text{LiveIn} / \text{LiveOut}$).
   - **Four Safe Optimization Passes**:
     - `const-fold`: Type-aware compile-time arithmetic folding with bitwidth truncation.
     - `algebraic-simp`: Identity rules ($x+0$, $x \times 1$, $x \times 0$, $x - 0$).
     - `redundant-mov`: Identity self-move and duplicate copy elimination.
     - `dce`: Unreachable basic block pruning and dead register write removal while strictly preserving observable memory stores (`st.global`), synchronizations (`bar.sync`), and terminators (`ret`).
2. **Rule-Based Diagnostics Engine**:
   - Categorized diagnostics (`ERROR`, `WARNING`, `OPTIMIZATION_SUGGESTION`, `NOTE`) with explanation and suggested actions.
   - Detects unused register allocations, unreachable code, self-moves, duplicate computations, long dependency chains, and undeclared registers.
3. **Hardware Independence & Safe ptxas Toolchain Integration**:
   - Operates completely without an NVIDIA GPU or proprietary drivers.
   - Automatically detects host NVIDIA `ptxas` compiler if installed to provide optional hardware assembly validation (`sm_70` through `sm_90`) without mock data.
4. **Interactive Full-Stack Workbench**:
   - **Monaco Code Editor**: Syntax highlighting with custom PTX Monarch tokenizer, line gutters, and diagnostic squiggles.
   - **Control Flow Graph Viewer**: Interactive basic block graph displaying instructions, entry highlights, unreachable blocks, and branch types.
   - **Optimization Diff View**: Side-by-side original vs transformed assembly with a step-by-step pass transformation audit.
   - **Register & Liveness Census**: Full table of register definitions, usages, and block liveness sets.
   - **Compilation Dashboard**: Metrics bar displaying instruction reduction %, block counts, latency (ms), and diagnostic tallies.
5. **Sample Suite & Reproducible Benchmarks**:
   - 6 documented sample programs illustrating common GPU compute patterns.
   - Automated benchmark runner recording parser, analyzer, and optimization scaling across micro and macro programs.

---

## Repository Structure

```
PTXScope/
├── engine/                       # C++17 Compiler Engine
│   ├── include/ptxscope/         # Public headers (lexer, parser, ir, cfg, analysis, passes)
│   ├── src/                      # Engine implementation files
│   ├── tests/                    # C++ unit tests (17 test cases, 100% pass rate)
│   └── CMakeLists.txt            # CMake build definition
├── backend/                      # Python FastAPI Service
│   ├── app/                      # Main application, models, and engine subprocess bridge
│   ├── tests/                    # Backend API integration tests (8 pytest cases)
│   └── requirements.txt          # Python dependencies
├── frontend/                     # React + TypeScript + Monaco Workbench
│   ├── src/                      # App, Monaco PTX grammar, and visualizer components
│   ├── public/                   # Static assets
│   ├── package.json              # npm dependencies
│   └── vite.config.ts            # Vite bundler configuration
├── samples/                      # 6 verified PTX sample programs
│   ├── 01_vector_add.ptx         # Parallel vector addition
│   ├── 02_constant_folding.ptx   # Constant arithmetic folding
│   ├── 03_redundant_moves.ptx    # Copy & algebraic identities
│   ├── 04_dead_code.ptx          # Dead code & unreachable blocks
│   ├── 05_branching_loop.ptx     # Multi-block loop CFG
│   └── 06_load_store.ptx         # Memory pipeline & barriers
├── benchmarks/                   # Scalability & Throughput Benchmarks
│   ├── benchmark_runner.py       # Performance evaluation runner
│   └── results/                  # Generated benchmark metrics (JSON)
├── docs/                         # Detailed Technical Documentation
│   ├── architecture.md           # System design and pipeline details
│   ├── ptx-subset.md             # Supported grammar and instruction reference
│   ├── optimization-design.md    # Safety rules and transformation invariants
│   └── testing.md                # Verification methodology
├── scripts/                      # Build and run scripts
│   ├── build_engine.ps1 / .sh    # Build C++ engine
│   ├── run_all_tests.ps1 / .sh   # Master test runner
│   └── start_all.ps1 / .sh       # Full-stack dev launcher
├── README.md
└── LICENSE
```

---

## Quickstart & Installation

### Prerequisites
- **C++ Compiler**: Clang 15+ or GCC 10+ or MSVC 2019+ (C++17 support required)
- **CMake & Ninja**: CMake 3.15+ and Ninja build system
- **Python**: Python 3.10+
- **Node.js**: Node.js v18+ and npm

### 1. Build the C++ Compiler Engine

Using PowerShell (Windows):
```powershell
./scripts/build_engine.ps1
```

Using Bash (Linux/macOS):
```bash
chmod +x scripts/*.sh
./scripts/build_engine.sh
```

Or manually with CMake:
```bash
cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The CLI executable will be generated at `build/engine/ptxscope` (or `ptxscope.exe`).

### 2. Install Backend Dependencies

```bash
cd backend
pip install -r requirements.txt
cd ..
```

### 3. Install Frontend Dependencies & Build

```bash
cd frontend
npm install
npm run build
cd ..
```

---

## Running the Full-Stack Workbench

You can start both the FastAPI backend and the Vite frontend simultaneously:

**Windows PowerShell**:
```powershell
./scripts/start_all.ps1
```

**Linux/macOS**:
```bash
./scripts/start_all.sh
```

Or start each service in separate terminals:
- **Backend**:
  ```bash
  cd backend
  uvicorn app.main:app --host 127.0.0.1 --port 8000 --reload
  ```
- **Frontend**:
  ```bash
  cd frontend
  npm run dev
  ```

Open your browser to: **`http://localhost:5173`**

---

## CLI Engine Usage

The native C++ engine can be run directly from the command line:

```bash
# Tokenize and parse assembly
./build/engine/ptxscope parse samples/01_vector_add.ptx

# Run static analysis, register def-use census, and diagnostics
./build/engine/ptxscope analyze samples/04_dead_code.ptx

# Run optimization passes and emit transformed PTX
./build/engine/ptxscope optimize samples/02_constant_folding.ptx --passes const-fold,dce

# Export Control Flow Graph to Graphviz DOT
./build/engine/ptxscope cfg samples/05_branching_loop.ptx --format dot

# Validate against local NVIDIA ptxas toolchain
./build/engine/ptxscope validate samples/01_vector_add.ptx --gpu-name sm_70

# Emit structured JSON representation
./build/engine/ptxscope json samples/01_vector_add.ptx
```

---

## Running the Automated Test Suites

PTXScope includes an automated test runner that runs all C++ unit tests, backend API integration tests, and frontend build checks:

```powershell
./scripts/run_all_tests.ps1
```

```
=======================================================
           PTXScope Master Test Runner                 
=======================================================

[1/3] Running C++ Compiler Engine Unit Tests...
[ RUN      ] CorrectnessTest.FloatPreservation (OK)
[ RUN      ] CorrectnessTest.PipelineOptimizationChain (OK)
[ RUN      ] OptimizerTest.ConstantFolding (OK)
[ RUN      ] OptimizerTest.AlgebraicSimplification (OK)
[ RUN      ] OptimizerTest.RedundantMoveElimination (OK)
[ RUN      ] OptimizerTest.DeadCodePreservesSideEffects (OK)
[ RUN      ] AnalysisTest.UnusedRegisterAndDiagnostics (OK)
[ RUN      ] AnalysisTest.LivenessSets (OK)
[ RUN      ] CFGTest.BranchEdgesAndReachability (OK)
[ RUN      ] IRTest.OperandProperties (OK)
[ RUN      ] IRTest.DefUseDetection (OK)
[ RUN      ] IRTest.SideEffectDetection (OK)
[ RUN      ] ParserTest.ParseKernelStructure (OK)
[ RUN      ] ParserTest.SyntaxErrorRecovery (OK)
[ RUN      ] LexerTest.DirectivesAndRegisters (OK)
[ RUN      ] LexerTest.CommentsAndLocations (OK)
[ RUN      ] LexerTest.PredicatesAndOperators (OK)
Total Tests: 17 | Passed: 17 | Failed: 0
[PASS] C++ Engine tests passed!

[2/3] Running FastAPI Backend Integration Tests...
backend/tests/test_api.py::test_health PASSED
backend/tests/test_api.py::test_capabilities PASSED
backend/tests/test_api.py::test_samples_endpoints PASSED
backend/tests/test_api.py::test_workbench_endpoint PASSED
backend/tests/test_api.py::test_malformed_ptx PASSED
backend/tests/test_api.py::test_cfg_endpoint PASSED
backend/tests/test_api.py::test_optimize_endpoint PASSED
backend/tests/test_api.py::test_validate_endpoint PASSED
Total API Tests: 8 | Passed: 8 | Failed: 0
[PASS] Backend API tests passed!

[3/3] Running Frontend Typecheck & Build...
vite v5.4.21 building for production...
dist/index.html   1.10 kB
dist/assets/      240 kB
[PASS] Frontend build succeeded!

=======================================================
   ALL TESTS PASSED! PTXScope is 100% verified.       
=======================================================
```

---

## Running Benchmarks

Measure compiler parsing, throughput, and transformation latency:

```bash
python benchmarks/benchmark_runner.py
```

Outputs performance summary and saves `benchmarks/results/benchmark_summary.json`.

---

## Technical Documentation

- **[System Architecture (`docs/architecture.md`)](docs/architecture.md)**: Three-tier design, pipeline flow, and compiler invariants.
- **[PTX Grammar Subset (`docs/ptx-subset.md`)](docs/ptx-subset.md)**: Specification of parsed directives, instructions, types, and special registers.
- **[Optimization Design (`docs/optimization-design.md`)](docs/optimization-design.md)**: Constant folding, algebraic identity rules, and DCE safety proofs.
- **[Testing Methodology (`docs/testing.md`)](docs/testing.md)**: Verification harnesses and test design.

---

## License

PTXScope is released under the [MIT License](LICENSE).
