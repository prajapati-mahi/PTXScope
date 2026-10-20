# Testing & Verification Methodology

PTXScope employs a multi-tiered test architecture covering unit-level compiler components, end-to-end correctness, REST API contracts, frontend type safety, and scaling performance benchmarks.

---

## 1. Test Architecture Overview

| Layer | Harness | Target | Location |
|---|---|---|---|
| **C++ Compiler Engine** | Standalone C++ Framework | Lexer, Parser, IR, CFG, Analysis, Passes | `engine/tests/` |
| **Backend Integration** | `pytest` + `TestClient` | REST endpoints, subprocess bridge, ptxas | `backend/tests/` |
| **Frontend Verification**| `tsc` (strict) + `vite build` | React components, Monaco grammar, types | `frontend/` |
| **Benchmark Suite** | Python Runner | Throughput, latency, scaling | `benchmarks/` |

---

## 2. C++ Compiler Engine Unit Tests (`engine/tests/`)

Implemented in `engine/tests/` with zero third-party dependencies via `test_framework.hpp`.

### Test Suites:
1. **`LexerTest` (`test_lexer.cpp`)**:
   - Tests directive and register tokenization (`.version`, `%r1`, `%rd0`).
   - Tests comment handling (`//`, `/* */`) and line tracking.
   - Tests predicate parsing (`@p0`, `@!p1`).
2. **`ParserTest` (`test_parser.cpp`)**:
   - Tests full vector add kernel parsing and parameter extraction.
   - Tests error recovery upon encountering unexpected tokens or missing semicolons.
3. **`IRTest` (`test_ir.cpp`)**:
   - Tests operand properties and string serialization.
   - Tests destination register (`get_def_reg()`) and source register (`get_use_regs()`) extraction.
   - Tests side-effect detection (`st`, `bar.sync`, `ret`).
4. **`CFGTest` (`test_cfg.cpp`)**:
   - Tests basic block boundary creation.
   - Tests branch edges (conditional true, fallthrough, unconditional).
   - Tests reachability analysis detecting unreachable blocks.
   - Tests DOT and JSON export formats.
5. **`AnalysisTest` (`test_analysis.cpp`)**:
   - Tests register def-use counting and live range analysis.
   - Tests backward data-flow liveness fixed-point convergence (Gen/Kill/LiveIn/LiveOut).
   - Tests rule-based diagnostic alerts (`W001_UNUSED_REGISTER`, `W002_UNREACHABLE_CODE`).
6. **`OptimizerTest` (`test_optimizer.cpp`)**:
   - Tests constant folding on integer addition and multiplication.
   - Tests algebraic simplification identities ($x+0$, $x \times 1$, $x \times 0$).
   - Tests redundant self-move elimination (`mov %r1, %r1`).
   - Tests dead code elimination preserving side-effecting stores.
7. **`CorrectnessTest` (`test_correctness.cpp`)**:
   - Verifies that floating-point operations are strictly preserved and not folded as integers.
   - Verifies multi-pass pipeline transformation chains.

### Running C++ Tests:
```powershell
./build/engine/tests/ptxscope_tests.exe
```

---

## 3. Backend Integration Tests (`backend/tests/`)

Implemented using `pytest` and Starlette's `TestClient`.

### Covered Endpoints:
- `test_health`: Verifies health check returns engine and ptxas readiness.
- `test_capabilities`: Verifies supported passes, instructions, and directives.
- `test_samples_endpoints`: Verifies all 6 sample programs load successfully.
- `test_workbench_endpoint`: Runs complete vector addition PTX through full analysis and optimization.
- `test_malformed_ptx`: Verifies graceful diagnostic error emission on broken input without crashing.
- `test_cfg_endpoint`: Verifies structured JSON CFG output.
- `test_optimize_endpoint`: Verifies targeted pass execution.
- `test_validate_endpoint`: Tests real ptxas integration status.

### Running Backend Tests:
```powershell
$env:PYTHONPATH = "backend"
python -m pytest backend/tests -v
```

---

## 4. Frontend Typecheck & Build

The frontend is compiled under strict TypeScript settings (`strict: true`, `noUnusedLocals: true`, `noUnusedParameters: true`).

```powershell
cd frontend
npm run build
```

---

## 5. Master Test Runner Script

To execute all three test suites sequentially and output a unified status report:

```powershell
./scripts/run_all_tests.ps1
```
*(On Linux/macOS: `./scripts/run_all_tests.sh`)*
