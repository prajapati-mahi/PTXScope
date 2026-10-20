# PTXScope Optimization Passes & Safety Design

PTXScope implements four static compiler optimization passes. Correctness and semantic preservation are prioritized over aggressive speculative transformations.

---

## 1. Safety Preconditions & Invariants

Each pass adheres to strict compiler invariants:
1. **Observable Side Effects Must Be Preserved**: Memory stores (`st.global`, `st.shared`, `st.local`), synchronizations (`bar.sync`), and terminators (`ret`, `exit`) must never be eliminated as dead code merely because their output register is unread.
2. **Type Semantics & Bitwidth Preservation**: Transformations must strictly observe signedness (`.s32` vs `.u32`) and target bitwidth truncation (e.g. 32-bit arithmetic must wrap modulo $2^{32}$).
3. **Floating-Point Conservatism**: Floating-point instructions (`.f32`, `.f64`) are **never** treated as interchangeable with integer types. Constant folding for floating-point values is skipped to preserve IEEE-754 rounding, subnormals, and NaN semantics.
4. **Predication Preservation**: If an instruction is predicated (`@p0` or `@!p1`), any transformation must preserve the predicate condition.

---

## 2. Pass 1: Constant Folding (`const-fold`)

### Objective
Evaluate expressions whose operands are known compile-time immediate constants, replacing the arithmetic instruction with a simple `mov` instruction.

### Transformation Rules
- `add.[type] %dest, C1, C2;` $\to$ `mov.[type] %dest, (C1 + C2);`
- `sub.[type] %dest, C1, C2;` $\to$ `mov.[type] %dest, (C1 - C2);`
- `mul.lo.[type] %dest, C1, C2;` $\to$ `mov.[type] %dest, (C1 * C2);`

### Safety Conditions
- Both source operands must be immediate integers (`Operand::Kind::IMMEDIATE_INT`).
- Destination must be a valid register.
- The computed value is truncated using bitmasking corresponding to `DataType`:
  - 8-bit: `uint8_t` / `int8_t`
  - 16-bit: `uint16_t` / `int16_t`
  - 32-bit: `uint32_t` / `int32_t`
  - 64-bit: `uint64_t` / `int64_t`
- Floating-point instructions are skipped and logged in `skipped_reasons`.

---

## 3. Pass 2: Algebraic Simplification (`algebraic-simp`)

### Objective
Apply mathematical identity simplifications to eliminate redundant arithmetic overhead without altering program state.

### Transformation Rules
- **Additive Identity**:
  - `add.[type] %dest, %src, 0;` $\to$ `mov.[type] %dest, %src;`
  - `add.[type] %dest, 0, %src;` $\to$ `mov.[type] %dest, %src;`
- **Subtractive Identity**:
  - `sub.[type] %dest, %src, 0;` $\to$ `mov.[type] %dest, %src;`
- **Multiplicative Identity**:
  - `mul.lo.[type] %dest, %src, 1;` $\to$ `mov.[type] %dest, %src;`
  - `mul.lo.[type] %dest, 1, %src;` $\to$ `mov.[type] %dest, %src;`
- **Zero Multiplication**:
  - `mul.lo.[type] %dest, %src, 0;` $\to$ `mov.[type] %dest, 0;`
  - `mul.lo.[type] %dest, 0, %src;` $\to$ `mov.[type] %dest, 0;`

### Safety Conditions
- Only applied to integer types. Skipped on floating-point types because $x \times 0.0 \ne 0.0$ when $x = \text{NaN}$ or $-\infty$ under IEEE-754.

---

## 4. Pass 3: Redundant Move Elimination (`redundant-mov`)

### Objective
Remove identity moves and duplicate copy assignments.

### Transformation Rules
- **Self-Move Elimination**:
  - `mov.[type] %r1, %r1;` $\to$ Deleted.
- **Duplicate Copy Elimination**:
  - If `%r2` was previously loaded from `%r1` and neither register has been modified, a subsequent `mov %r2, %r1;` is deleted.

### Safety Conditions
- Must not be predicated, or must match identical predicate conditions.
- Invalidation: If either register is redefined by any intervening instruction, the copy mapping is invalidated immediately.

---

## 5. Pass 4: Dead Code Elimination (`dce`)

### Objective
Remove unreachable basic blocks and dead register assignments that consume hardware registers without contributing to any kernel output.

### Transformation Rules
1. **Unreachable Block Pruning**:
   - Computes CFG reachability via breadth-first search from function entry.
   - Any basic block not present in `reachable_blocks` is pruned from the module.
2. **Dead Instruction Elimination**:
   - Computes global register use counts across all blocks.
   - If an instruction defines register `R`, and `use_count[R] == 0`:
     - Checks `!inst->has_side_effects()`.
     - Checks `!inst->predicate.is_predicated`.
     - Removes the instruction and decrements the use counts of the registers it referenced.
     - Runs iteratively until fixed-point convergence.

---

## 6. Pipeline Coordination & Metrics Tracking

The `PassManager` executes passes in logical dependency sequence:
$$\text{const-fold} \to \text{algebraic-simp} \to \text{redundant-mov} \to \text{dce}$$

Every pass returns a structured `PassResult`:
- `pass_name`: string identifier
- `modified`: boolean flag
- `num_transformations`: integer count of changes
- `explanations`: human-readable log of transformations
- `skipped_reasons`: documentation of skipped cases and safety audits
