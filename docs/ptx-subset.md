# Supported PTX Grammar & Instruction Subset

PTXScope implements an explicit, well-defined subset of NVIDIA PTX (Parallel Thread Execution) ISA v7.0–v8.0+. Rather than silently ignoring unrecognized syntax or fabricating incorrect interpretation, PTXScope enforces strict grammar parsing and emits rule-based compiler diagnostics for unsupported constructs.

---

## 1. Directives

| Directive | Syntax Example | Meaning |
|---|---|---|
| `.version` | `.version 7.0` | PTX ISA version specification |
| `.target` | `.target sm_70` | Target architecture generation (e.g. sm_70, sm_80) |
| `.address_size` | `.address_size 64` | Address space size (32 or 64 bits) |
| `.entry` | `.entry kernel_name(params) { ... }` | Compute kernel entry point definition |
| `.func` | `.func func_name(params) { ... }` | Device function definition |
| `.param` | `.param .u64 ptr_a` | Kernel or function parameter declaration |
| `.reg` | `.reg .u32 %r<10>;` | Register allocation (ranges `%r<N>` or lists `%r1, %r2`) |
| `.global` | `.global .u32 arr[100];` | Global memory variable declaration |
| `.shared` | `.shared .u32 smem[256];` | Warp/block shared memory declaration |
| `.const` | `.const .f32 weights[32];` | Constant memory declaration |
| `.local` | `.local .b32 stack_var;` | Thread-local memory space declaration |
| `.visible` | `.visible .entry ...` | Exported symbol linkage modifier |

---

## 2. Core Instructions

### Movement & Loading
- **`mov`**: Register copy, constant loading, or special register read.
  - `mov.u32 %r1, 42;`
  - `mov.u32 %r2, %ctaid.x;`
  - `mov.u64 %rd1, %rd2;`
- **`ld`**: Memory load from explicit address space.
  - `ld.param.u64 %rd1, [param_ptr];`
  - `ld.global.u32 %r1, [%rd1];`
  - `ld.global.u32 %r2, [%rd1 + 8];`
- **`st`**: Memory store to address space.
  - `st.global.u32 [%rd1], %r1;`
  - `st.global.s32 [%rd1 + 4], %s1;`

### Integer & Arithmetic Operations
- **`add`**: Addition with signed/unsigned integer or floating-point types.
  - `add.u32 %r1, %r2, %r3;`
  - `add.s32 %s1, %s2, -10;`
  - `add.u64 %rd1, %rd2, 8;`
  - `add.f32 %f1, %f2, %f3;`
- **`sub`**: Subtraction.
  - `sub.u32 %r1, %r2, 1;`
  - `sub.s32 %s1, %s2, %s3;`
- **`mul`**: Multiplication with width modifiers (`.lo`, `.hi`, `.wide`).
  - `mul.lo.u32 %r1, %r2, %r3;`
  - `mul.wide.u32 %rd1, %r1, 4;`
- **`mad`**: Multiply-add (`d = a * b + c`).
  - `mad.lo.s32 %r4, %r1, %r2, %r3;`

### Comparison & Predication
- **`setp`**: Set predicate register based on comparison operators (`.eq`, `.ne`, `.lt`, `.le`, `.gt`, `.ge`).
  - `setp.lt.s32 %p0, %i, %n;`
  - `setp.eq.u32 %p1, %r1, 0;`
- **Predicated Execution**:
  - `@p0 add.u32 %r1, %r2, 1;` (executes only if `%p0` is true)
  - `@!p1 bra exit_label;` (executes only if `%p1` is false)

### Control Flow & Synchronization
- **`bra`**: Branch to target label (unconditional or predicated).
  - `bra loop_header;`
  - `@p0 bra.uni loop_exit;`
- **`ret`**: Return from function or kernel.
- **`exit`**: Terminate thread execution.
- **`bar.sync`**: Warp / CTA barrier synchronization.
  - `bar.sync 0;`

---

## 3. Special Hardware Registers

| Register | Description |
|---|---|
| `%tid.x`, `%tid.y`, `%tid.z` | Thread ID within current block |
| `%ntid.x`, `%ntid.y`, `%ntid.z` | Block dimensions (number of threads) |
| `%ctaid.x`, `%ctaid.y`, `%ctaid.z` | Block / CTA ID within current grid |
| `%nctaid.x`, `%nctaid.y`, `%nctaid.z` | Grid dimensions (number of blocks) |
| `%laneid` | Thread lane within warp (0–31) |
| `%warpid` | Warp identifier |
| `%clock`, `%clock64` | Hardware cycle timer |

---

## 4. Unsupported Syntax Handling

When the compiler encounters instructions or directives outside this subset (such as tensor core WMMA, inline PTX asm calls, texture operations), it emits diagnostic warning `W010_UNSUPPORTED_INSTRUCTION`:

```
[WARNING] (W010_UNSUPPORTED_INSTRUCTION) line 22:5 - Instruction 'wmma' is outside the primary analyzed PTX subset.
  Explanation: PTXScope supports static analysis for mov, add, sub, mul, mad, setp, ld, st, bra, ret, exit, bar.
  Suggested Action: Verify that this instruction is supported by target hardware.
```
This ensures clear boundaries between modeled compiler semantics and hardware-dependent extensions.
