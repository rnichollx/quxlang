# VMIR2 instruction semantics

This document describes the intended behavior of each vmir2 instruction as currently reflected by the codegen state engine (`quxlang/include/quxlang/vmir2/state_engine.hpp`) and the IR type declarations (`quxlang/include/quxlang/vmir2/vmir2.hpp`).

## Global notes
- Arithmetic wraps by default. A future field may select non-wrapping arithmetic.
- Division by zero is UB. Modulus with a negative modulus is UB.

## State model quick reference
- Each local slot has `slot_state{ alive, storage_valid, delegate_of?, delegates? }`.
- readonly(i): requires `state[i].alive && state[i].storage_valid`; no state change.
- consume(i): requires `alive && storage_valid`; sets `alive=false`; sets `storage_valid = has delegate_of`.
- output(i): requires `not alive`; sets `alive=true`; requires `storage_valid` if `delegate_of` is set; then `storage_valid=true`.
- Entry state: parameters are `storage_valid=true`; positional/named parameters are alive unless typed as `NEW&&` (`nvalue_slot`) which start dead but valid.
- Normal/exception exit: see `state_engine.apply_normal_exit` / `apply_exception_exit`.

## Slot parameter kinds
`NEW&&` and `DESTROY&&` are slot types (not reference types). They cannot exist as true runtime locals; instead, they control transfer of storage/lifetime between caller and callee without materializing a separate reference value.
- `NEW&& T`: provides S-state storage for `T` at entry and requires it to be A-state on normal return.
- `DESTROY&& T`: provides an A-state value that must be consumed and leaves S-state storage on return.
- Slots can target references too (e.g., `NEW&& &I32` is valid).

## State terminology (A/S/P/DA/DP)
- A-state (Active): local has a valid, initialized value.
- S-state (Storage): storage is allocated/valid, but value is not active.
- P-state (Partial): under construction via delegates.
- DS-state (Delegate Storage): a delegate’s storage exists but is not active.
- DA-state (Delegate Active): a delegate that is active (prior to its own finalization).
- DP-state (Delegate Partial): a delegate that itself has delegates (partial).

For clarity, this document uses both the long and abbreviated forms (e.g., "Partial (P-state)").

---

## Arithmetic and comparisons
### `int_add` (IADD)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of the same type as `a` and `b`.

**Effect:**
- Integer addition of `a` and `b`, producing `result`. Wraps on overflow.


### `int_sub` (ISUB)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of the same type as `a` and `b`.

**Effect:**
- Integer subtraction `a - b`, producing `result`. Wraps on underflow/overflow.

### `int_mul` (IMUL)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of the same type as `a` and `b`.

**Effect:**
- Integer multiplication of `a` and `b`, producing `result`. Wraps on overflow.

### `int_div` (IDIV)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of the same type as `a` and `b`.

**Effect:**
- Integer division of `a` by `b`, producing `result`. Uses the operand type's signedness/width. Arithmetic wraps.
- Division by zero is undefined behavior.

### `int_mod` (IMOD)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of the same type as `a` and `b`.

**Effect:**
- Integer remainder of `a` by `b`, producing `result`. Uses the operand type's signedness/width. Arithmetic wraps.
- Using a negative modulus is undefined behavior.

### `cmp_eq` (CEQ), `cmp_ne` (CNE), `cmp_lt` (CLT), `cmp_ge` (CGE)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be of the same integer type (same width and signedness).
- `result` must be of type `bool`.

**Effect:**
- Performs the indicated integer comparison between `a` and `b`, storing a boolean into `result`.

### `pcmp_eq` (PCEQ), `pcmp_ne` (PCNE), `pcmp_lt` (PCLT), `pcmp_ge` (PCGE)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be pointers of the same type.
- `result` must be of type `bool`.

**Effect:**
- Performs the indicated pointer comparison between `a` and `b`, storing a boolean into `result`.
- Equality and inequality are well-defined even when `a` and `b` refer to different allocations.
- Relational comparisons (`<`, `>=`) between pointers from different allocations are undefined behavior.
- Within the same allocation, ordering follows address order; one-past-the-end participates as usual for contiguous ranges.

### `gcmp_eq` (GCEQ), `gcmp_ne` (GCNE), `gcmp_lt` (GCLT), `gcmp_ge` (GCGE)

**Operands:**
1. `a` (consumed)
2. `b` (consumed)
3. `result` (output)

**Validity:**
- `a` and `b` must be pointers of the same type.
- `result` must be of type `bool`.

**Effect:**
- Global pointer comparisons that place all pointers into a single total order; the specific order between pointers from different allocations is unspecified but consistent within a single execution.

---

## Boolean conversions and increments
### `to_bool` (TB)
Operands:
- `{from, to}`

Purpose:
- Convert arbitrary value to `bool`.

State:
- `consume(from)`, `output(to)`.

### `to_bool_not` (TBN)
Operands:
- `{from, to}`

Purpose:
- Convert to `bool` then logical not.

State:
- `consume(from)`, `output(to)`.

### `increment` (INC)
Operands:
- `{value, result}`

Purpose:
- Post-increment `value`, producing `result` (the post value) as a separate output.

State:
- `consume(value)`, `output(result)`. Wraps on overflow.

### `decrement` (DEC)
Operands:
- `{value, result}`

Purpose:
- Post-decrement.

State:
- `consume(value)`, `output(result)`. Wraps on underflow.

### `preincrement` (PINC)
Operands:
- `{target, target2}`

Purpose:
- Pre-increment in place; output the updated value/reference in `target2`.

State:
- `consume(target)`, `output(target2)`. Wraps on overflow.

### `predecrement` (PDEC)
Operands:
- `{target, target2}`

State:
- `consume(target)`, `output(target2)`. Wraps on underflow.

---

## Constants and loads/stores
### `load_const_zero` (LCZ)
Operands:
- `{target}`

Purpose:
- Zero-initialize `target`. Applies to trivial structs, integers, and `bool` (false).

State:
- `output(target)`.

### `load_const_bool` (LCB)
Operands:
- `{target, value}`

Purpose:
- Load a constant Boolean.

State:
- `output(target)`.

### `load_const_int` (LCI)
Operands:
- `{target, value, bits, signed?}`

Purpose:
- Load constant integer.

State:
- `output(target)`.

### `load_const_value` (LCV)
Operands:
- `{target, bytes/type}`

Purpose:
- Load arbitrary constant value blob.

State:
- `output(target)`.

---

## References and pointers
### `make_reference` (MKR)
Operands:
- `{value_index, reference_index}`

Purpose:
- Create a reference to an existing value.

State:
- `readonly(value_index)`; `output(reference_index)`.

### `copy_reference` (CPR)
Operands:
- `{from_index, to_index}`

Purpose:
- Copy reference (alias another reference).

State:
- `readonly(from_index)`; `output(to_index)`.

### `cast_reference` (CRF)
Operands:
- `{source_ref_index, target_ref_index}`

Purpose:
- Cast/reference-rebind as permitted by type system.

State:
- `consume(source_ref_index)`; `output(target_ref_index)`.

### `make_pointer_to` (MPT)
Operands:
- `{of_index, pointer_index}`

Purpose:
- Create a pointer to an existing object/value.

State:
- `readonly(of_index)`; `output(pointer_index)`.

### `dereference_pointer` (DRP)
Operands:
- `{from_pointer, to_reference}`

Purpose:
- Form a reference from a pointer.

State:
- `consume(from_pointer)`; `output(to_reference)`.

### `load_from_ref` (LFR)
Operands:
- `{from_reference, to_value}`

Purpose:
- Read value from reference.

State:
- `consume(from_reference)`; `output(to_value)`.

### `store_to_ref` (STR)
Operands:
- `{from_value, to_reference}`

Purpose:
- Store value into reference.

State:
- `consume(from_value)`; `consume(to_reference)`.

### `pointer_arith` (PAR)
Operands:
- `{from, multiplier, offset, result}`

Purpose:
- Compute pointer +/- offset (scaled by element size). `multiplier` is +1 or -1.

State:
- `consume(from)`; `consume(offset)`; `output(result)`.

### `pointer_diff` (PDF)
Operands:
- `{from, to, result}`

Purpose:
- Compute element-difference between two pointers into the same array/object.

State:
- `consume(from)`; `consume(to)`; `output(result)`.

---

## Delegates/struct construction
### `struct_delegate_new` (SDN)
Operands:
- `{on_value, fields}`

Purpose:
- Begin delegated construction for a struct; each field is mapped to a local index.

Transitions:
- `on_value` S -> P; each delegate slot: null -> DS.

State effects:
- `state[on_value]`: `storage_valid=true`, `dtor_enabled=false`, `alive=true`.
- For each field index `i`: `state[i]`: `delegate_of=on_value`, `storage_valid=true`, `dtor_enabled=false`, `alive=false`.

### `struct_complete_new` (SCN)
Operands:
- `{on_value}`

Purpose:
- Finish construction; field delegate slots are removed; `on_value` becomes fully constructed.

Semantics:
- `SCN` performs one of two transitions depending on the role of `on_value`:
  - P -> A when the target is not itself a delegate (non-delegate target).
  - DP -> DA when the target is itself a delegate and also has delegates.

State effects:
- Erase delegate child indices from state; clear `state[on_value].delegates`.
- Then set `state[on_value]`: `alive=true`, `storage_valid=true`, `dtor_enabled=true`.

### `defer_nontrivial_dtor` (DNTD)
Operands:
- `{func, on_value, args}`

Purpose:
- Defer a destructor call for a non-trivial object; recorded on `on_value`.

State:
- `readonly(on_value)`.

### `end_lifetime` (ELT)
Operands:
- `{of}`

Purpose:
- Consume/destroy value (end lifetime of an object/value).

State:
- `consume(of)`.

---

## Field/array access
### `access_field` (ACF)
Operands:
- `{base_index, store_index, field_name}`

Purpose:
- Produce a reference/pointer to a subfield of a base object.

State:
- `consume(base_index)`; `output(store_index)`.

### `access_array` (ACA)
Operands:
- `{base_index, index_index, store_index}`

Purpose:
- Compute element reference/pointer from base and integer index.

State:
- `consume(base_index)`; `consume(index_index)`; `output(store_index)`.

---

## Invocation and returns
### `invoke` (IVK)
Operands:
- `{what, args}`

Purpose:
- Call a routine or function instantiation with arguments.

State rules:
- For each argument (positional or named): look up the instantiated parameter type. If it is `NEW&&` (`nvalue_slot`), `output(arg)`; otherwise `consume(arg)`.
- There is no special behavior for a named argument `RETURN` at the IR/routine level; if present, it is just another parameter whose instantiated type is determined by the frontend.

### `ret` (RET)
Purpose:
- Return from current function. No explicit value; returning a value is achieved by the caller/callee contract established by the frontend (e.g., providing a `NEW&&` named argument like `RETURN` when applicable).

State:
- No per-slot mutations at instruction time in the state engine; overall entry/exit is handled separately by `apply_normal_exit` / `apply_exception_exit`.

---

## Assertions
### `assert_instr` (ASRT)
Operands:
- `{condition, message, location?}`

Purpose:
- Runtime assert (constexpr-capable); if condition is false during constexpr, evaluation fails.

State:
- `consume(condition)`.

---

## Swapping
### `swap` (SWP)
Operands:
- `{a, b}`

Purpose:
- Swap two values via their references.

State:
- `consume(a)`; `consume(b)`. These are references to storage being swapped; locals themselves are not destroyed.

---

## Terminators and control flow
### `jump` (JUMP)
Operands:
- `{target_block}`

Purpose:
- Unconditional branch to block.

State:
- Terminator; no per-slot state change.

### `branch` (BRANCH)
Operands:
- `{cond, true_block, false_block}`

Purpose:
- Conditional branch.

State:
- Terminator; consumes condition value.

---

## Pointer/integer conversions and fences
### `ptr_to_i` (PTI), `i_to_ptr` (ITP)
Purpose:
- Convert between pointers and integers.

Constexpr:
- Not allowed during constexpr execution.

Behavior:
- Implementation-defined; no slot-state specifics beyond typical consume/output (TBD if/when enabled here).

### `fence_byte_acquire` (FBA), `fence_byte_release` (FBR)
Purpose:
- Interact with non-quxlang code; memory fence semantics.

State:
- Underspecified currently; assumed no slot state changes.

---

## Constexpr notes
- The constexpr interpreter executes most of the above and enforces the same state rules as `codegen_state_engine`.
- Illegal-in-constexpr instructions include at least `ptr_to_i` and `i_to_ptr` as noted above.

## Appendix: declarations not present in `vm_instruction`
- Some forward declarations (e.g., `ptr_offset`, `ptr_comp`, `move_value`) are not part of `vm_instruction` and thus omitted here. If/when they are activated, this document should be extended accordingly.
