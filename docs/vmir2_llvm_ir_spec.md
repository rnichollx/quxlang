# VMIR2 To LLVM IR Lowering Specification
(partially ai generated, may have mistakes)
## Purpose

This document is a normative specification for lowering VMIR2 into textual LLVM IR.
It is written as a backend contract for future implementation work.
It does not describe any particular implementation file or current internal structure.

## Scope

This specification covers:

- LLVM-visible ABI rules for VMIR2 routines.
- The slot model used inside lowered functions.
- Runtime type categories and their LLVM storage/ABI representation.
- Lowering rules for every VMIR2 instruction and terminator.

This specification does not define:

- Source-language overload selection.
- VMIR2 construction rules.
- Object emission, relocation models, or target-specific machine code.

## Core Model

### Slot model

Every VMIR2 local is modeled as a slot pointer.

- A slot pointer is the address used for every later read, write, aliasing operation, field access, array element access, and lifetime transition on that slot.
- Once a value enters a slot, all later operations work through the slot pointer, even when the value itself is trivially relocatable.
- A slot may be backed by:
  - a local `alloca`,
  - a caller-provided pointer parameter,
  - a pointer alias into another slot,
  - a pointer alias into global or static storage.

### Trivially relocatable types

For now, the following types are trivially relocatable:

- pointers,
- references,
- `int_type`,
- `byte`,
- `bool`,
- `float_type`.

Procedure-pointer values follow the same rule as pointer values.

Future user-defined types may become trivially relocatable, but this specification does not define that mechanism yet.

### ABI boundary rules

At a function boundary:

- Trivially relocatable input values are passed by value.
- Non-trivially-relocatable input values are passed by pointer to storage.
- `nvalue_slot<T>` and `dvalue_slot<T>` are slot parameters, not value parameters.
- Slot storage for `nvalue_slot<T>`, `dvalue_slot<T>`, and non-trivially-relocatable values is caller-managed.
- A slot parameter arrives as a pointer and the callee uses that pointer directly as the slot pointer.
- Destruction of value arguments is performed by the callee, not the caller.

### Trivially relocatable input parameters

For a trivially relocatable input parameter:

- The caller passes the value itself.
- The callee allocates local slot storage.
- The callee copies the incoming value into that storage exactly once at function entry.
- From that point onward, the lowered body uses only the slot pointer.

### Non-trivially-relocatable input parameters

For a non-trivially-relocatable input parameter:

- The caller allocates and owns the storage.
- The caller passes a pointer to that storage.
- The callee does not create replacement object storage for the parameter.
- The incoming pointer becomes the slot pointer for that slot.

### `nvalue_slot<T>` and `dvalue_slot<T>`

`nvalue_slot<T>` corresponds to `new[T]` output-slot semantics.
`dvalue_slot<T>` corresponds to `destroy[T]` slot semantics.

For slot parameters:

- The caller allocates and owns the backing storage.
- The caller passes a pointer to that storage.
- The callee uses that incoming pointer as the slot pointer directly.
- No copy is introduced merely because the slot target type is trivially relocatable.

This rule has one exception for `nvalue_slot<T>`:

- One trivially relocatable `nvalue_slot<T>` may also be promoted to the ABI return value.

### ABI return selection

Output slot return selection follows this order:

1. If a slot parameter named `RETURN` exists and its target type is trivially relocatable, it is the ABI return value.
2. Otherwise, the first trivially relocatable `nvalue_slot<T>` is the ABI return value.
3. All remaining `nvalue_slot<T>` parameters remain pointer-passed output slots.

If `RETURN` exists but is not trivially relocatable:

- `RETURN` remains a pointer-passed output slot.
- Another trivially relocatable `nvalue_slot<T>` may still be selected as the ABI return value.

When an `nvalue_slot<T>` is selected as the ABI return value:

- The lowered LLVM function uses `T` as its LLVM return type for that slot.
- The slot is not represented as a pointer-passed output parameter at the LLVM boundary.
- This specification does not constrain how an implementation stages or materializes that returned value internally.

### Destruction and transition cleanup

Block edges and returns must preserve VMIR2 lifetime semantics.

- If a live slot does not survive an edge, required destructors must run before control transfers.
- If an initguard lock does not survive an edge, the required release or abort action must run before transfer.
- Slot aliases must not trigger duplicate destruction.
- Value arguments received by the routine are destroyed by the callee as part of its lifetime cleanup responsibilities.
- Once a value is destroyed, the contents of its storage become undefined; later accesses to the same storage location must not treat the old bytes as a still-valid value.
- This rule applies even to trivially relocatable and otherwise trivial scalar types such as `I32`; unlike C++, destroying such a value is still treated as having a side effect on the storage bytes by making their contents undefined.
- For LLVM lowering purposes, destroying a local poisons the storage bytes after any destructor call completes, even when the type is trivially destructible.
- This is modeled in LLVM IR by storing a `poison` value into the destroyed storage region.

## Type Lowering

### Storage representation versus ABI representation

This specification distinguishes:

- slot storage representation: what is stored at a slot pointer,
- ABI representation: what crosses a function boundary.

### Type table

| VMIR2 type/category | Slot storage representation | ABI input/output representation | Notes |
| --- | --- | --- | --- |
| `void_type` | one-byte placeholder storage | never passed as a normal value | used only to give a slot an addressable placeholder |
| `bool_type` | integer boolean storage | passed by value | use integer zero/nonzero representation |
| `byte_type` | `i8` | passed by value | byte-exact |
| `int_type` | integer of declared bit width | passed by value | signedness affects arithmetic and comparison, not storage width |
| `size_type` | pointer-sized integer | passed by value | follows integer ABI rules when present as a concrete runtime scalar |
| `float_type` | IEEE float of declared width | passed by value | widths outside the supported LLVM scalar set are invalid |
| pointer types | opaque pointer storage | passed by value | includes machine pointers and non-reference pointer classes |
| reference types | opaque pointer storage | passed by value | reference value is itself a pointer to another object |
| procedure pointers | opaque pointer storage | passed by value | callable ABI comes from the procedure signature |
| `initguard_type` | target runtime guard scalar | passed by pointer unless made trivially relocatable later | represents the backing guard object |
| `initguard_lock_type` | opaque pointer storage | passed by value | runtime lock handle |
| interface values | opaque pointer storage | passed by value | points to an interface record containing default flag and slot pointers |
| `array_initializer_type` | `{ptr, i64, i64}` | passed by pointer unless later declared trivially relocatable | fields are base pointer, current index, total count |
| `attached_type_reference` | transparent wrapper over carried runtime type | same as carried runtime type | compile-time-only attachment metadata is erased |
| user-defined non-trivial runtime values | byte-addressable storage sized by layout | passed by pointer | includes classes, arrays, string-like aggregates, and other non-trivial values |
| `nvalue_slot<T>` selected as ABI return | slot-local representation of `T` | lowered LLVM function returns `T` and does not take a pointer parameter for that slot | this exception applies only when the selected return type is trivially relocatable |
| `nvalue_slot<T>` not selected as ABI return | caller-provided storage of `T` | caller passes pointer | callee uses pointer as slot pointer |
| `dvalue_slot<T>` | caller-provided storage of `T` | caller passes pointer | callee uses pointer as slot pointer |
| constexpr-only proxy/result carrier types | no native runtime representation | invalid in native lowering | must be rejected from native LLVM lowering |

### Aggregate layout

For non-trivial runtime values:

- The LLVM-visible object layout is byte-addressable storage sized by the VMIR2 type placement.
- Field addressing is computed from the struct layout offsets.
- Arrays use contiguous element storage with no implicit element objects outside the declared placement.

### Interface layout

An interface value points to a private immutable interface record containing:

1. a default-implementation flag,
2. one function pointer field per interface slot.

The slot order must be stable for a given interface type.

### Global and static storage

- Mutable globals lower to mutable LLVM globals.
- Readonly antestatal data lowers to readonly LLVM globals.
- Readonly antestatal data must lower to concrete constant definitions, not external declarations.
- Lowering input must therefore provide enough readonly data to materialize the constant initializer.
- Initguard objects lower to dedicated guard globals separate from the guarded payload.

## Instruction Lowering

### References, pointers, fields, and arrays

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `make_reference` | produce a reference value whose pointer payload is the slot pointer of the source value |
| `copy_reference` | produce a reference to the same underlying object named by the source slot without touching pointee storage |
| `cast_ptrref` | reinterpret the pointer payload into the destination pointer/reference category without changing the address |
| `make_pointer_to` | produce a non-reference pointer value whose payload is the slot pointer of the source value |
| `dereference_pointer` | reinterpret a pointer value as a reference value to the same pointee address |
| `access_field` | compute field address from the base slot pointer plus field offset; destination becomes a slot alias to that subobject |
| `access_array` | load the base reference pointer, compute `base + index * element_size`, and return a reference alias to that element |
| `access_pointer` | load the base pointer value, compute `base + index * element_size`, and return a reference alias to that element |
| `pointer_arith` | load the base pointer value, compute `from + multiplier * offset * element_size`, and store the resulting pointer value |
| `pointer_diff` | require both pointers to refer into the same array-like allocation, compute `(from - to) / element_size` in element units, then convert the signed result into the destination integer type |

### Loads, stores, constants, and byte access

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `load_const_int` | store the parsed integer constant into the destination slot storage |
| `load_const_float` | store the parsed floating constant into the destination slot storage |
| `load_const_bool` | store integer zero or one into the destination slot storage |
| `load_const_zero` | zero-initialize the destination slot storage |
| `load_const_value` | materialize the provided constant payload into destination storage; byte-span carriers use readonly constant byte data with start/end pointers, and other supported constant-carrier types lower to their corresponding immutable constant representation |
| `load_from_ref` | load from the reference pointee using the requested atomic mode and store the loaded value into the destination slot |
| `store_to_ref` | store the source value into the referenced pointee using the requested atomic mode |
| `get_value_byte` | byte-address from the reference pointee plus constant offset, load one byte, and store it into the destination slot |
| `set_value_byte` | byte-address from the reference pointee plus constant offset and store the source byte |
| `swap` | exchange the full runtime contents of the two referenced objects, including scalar data and pointer-like payloads |
| `compare_exchange` | perform a compare-and-exchange on the referenced location; write the success flag to the boolean result slot, replace the target with the desired value on success, and overwrite the expected object with the observed target value on failure; this operation is not semantically required to be atomic, though a lowering may choose an atomic implementation when appropriate |

### Calls and callable values

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `get_procedure_ptr` | materialize a function pointer constant for the selected routine and store it as a procedure pointer value |
| `invoke` | emit a direct call using the ABI derived from the callee signature and instantiated argument types |
| `invoke_indirect` | load the procedure pointer value, cast it to the callable LLVM type, and emit an indirect call |
| `interface_init` | materialize a private immutable interface record and store its address as the interface handle |
| `interface_is_default` | load the interface default flag and store the resulting boolean |
| `interface_invoke` | inspect the interface value; if it is a default interface value, call the default function and bind `THIS` to the interface value, otherwise dispatch through the function stored for the requested interface slot |

### Integer arithmetic and conversion

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `int_add` | integer add |
| `int_sub` | integer subtract |
| `int_mul` | integer multiply |
| `int_div` | signed or unsigned integer divide according to the operand type |
| `int_mod` | signed or unsigned integer remainder according to the operand type |
| `mut_int_add` | load referenced integer, add RHS, store back |
| `mut_int_sub` | load referenced integer, subtract RHS, store back |
| `mut_int_mul` | load referenced integer, multiply by RHS, store back |
| `mut_int_div` | load referenced integer, divide by RHS, store back |
| `mut_int_mod` | load referenced integer, remainder by RHS, store back |
| `iconv` | integer width/sign conversion between integer-like runtime types |
| `increment` | treat the operand as a reference to an integer or pointer object, increment the referenced object in place, and write the pre-update value into the result slot |
| `decrement` | treat the operand as a reference to an integer or pointer object, decrement the referenced object in place, and write the pre-update value into the result slot |
| `preincrement` | treat the operand as a reference to an integer or pointer object, increment the referenced object in place, and make the result slot a reference alias to the updated object |
| `predecrement` | treat the operand as a reference to an integer or pointer object, decrement the referenced object in place, and make the result slot a reference alias to the updated object |

### Floating arithmetic and conversion

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `float_add` | floating add |
| `float_sub` | floating subtract |
| `float_mul` | floating multiply |
| `float_div` | floating divide |
| `mut_float_add` | load referenced float, add RHS, store back |
| `mut_float_sub` | load referenced float, subtract RHS, store back |
| `mut_float_mul` | load referenced float, multiply by RHS, store back |
| `mut_float_div` | load referenced float, divide by RHS, store back |
| `float_from_int` | signed or unsigned integer-to-float conversion according to the source integer type |
| `canonicalize_float` | canonicalize the source floating value using the target’s IEEE canonicalization rule and store the canonical result |

### Bitwise operations

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `bitwise_and` | integer bitwise and |
| `bitwise_or` | integer bitwise or |
| `bitwise_xor` | integer bitwise xor |
| `bitwise_nand` | bitwise not of and |
| `bitwise_nor` | bitwise not of or |
| `bitwise_nxor` | bitwise not of xor |
| `bitwise_implies` | `(~lhs) \| rhs` |
| `bitwise_implied` | `lhs \| (~rhs)` |
| `bitwise_shift_up` | logical shift toward more-significant bit positions |
| `bitwise_shift_down` | logical shift toward less-significant bit positions |
| `bitwise_rotate_up` | rotate toward more-significant bit positions |
| `bitwise_rotate_down` | rotate toward less-significant bit positions |
| `bitwise_inverse` | bitwise not |
| `mut_bitwise_and` | load referenced integer, apply op, store back |
| `mut_bitwise_or` | load referenced integer, apply op, store back |
| `mut_bitwise_xor` | load referenced integer, apply op, store back |
| `mut_bitwise_nand` | load referenced integer, apply op, store back |
| `mut_bitwise_nor` | load referenced integer, apply op, store back |
| `mut_bitwise_nxor` | load referenced integer, apply op, store back |
| `mut_bitwise_implies` | load referenced integer, apply op, store back |
| `mut_bitwise_implied` | load referenced integer, apply op, store back |
| `mut_bitwise_shift_up` | load referenced integer, shift, store back |
| `mut_bitwise_shift_down` | load referenced integer, shift, store back |
| `mut_bitwise_rotate_up` | load referenced integer, rotate, store back |
| `mut_bitwise_rotate_down` | load referenced integer, rotate, store back |

### Comparisons

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `cmp_eq` | integer equality compare |
| `cmp_ne` | integer inequality compare |
| `cmp_lt` | signed or unsigned integer less-than compare according to operand type |
| `cmp_ge` | signed or unsigned integer greater-or-equal compare according to operand type |
| `pcmp_eq` | pointer equality compare |
| `pcmp_ne` | pointer inequality compare |
| `pcmp_lt` | pointer less-than compare; if the two pointers are unordered in the VMIR2 pointer domain, behavior is undefined |
| `pcmp_ge` | pointer greater-or-equal compare; if the two pointers are unordered in the VMIR2 pointer domain, behavior is undefined |
| `gcmp_eq` | global pointer total-order equality compare |
| `gcmp_ne` | global pointer total-order inequality compare |
| `gcmp_lt` | global pointer total-order less-than compare |
| `gcmp_ge` | global pointer total-order greater-or-equal compare |
| `float_ieee_eq` | IEEE ordered equality compare |
| `float_ieee_ne` | IEEE unordered-or-not-equal compare |
| `float_ieee_lt` | IEEE ordered less-than compare |
| `float_ieee_gt` | IEEE ordered greater-than compare |
| `to_bool` | map the input value to boolean truth |
| `to_bool_not` | map the input value to boolean truth and negate it |

### Storage, struct, and array construction

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `storage_init` | establish the target storage slot in the empty state with no active object resident in it |
| `storage_init_start` | begin constructing a value inside the referenced storage region and bind the destination slot to that in-storage object under construction |
| `storage_deinit_start` | bind the destination slot to the active object currently resident in the referenced storage region so that destruction or deinitialization can proceed through that slot |
| `storage_pun` | reinterpret the referenced storage address as the destination reference type |
| `struct_init_start` | bind field slots to subobject addresses inside the destination aggregate storage |
| `struct_init_finish` | no direct code required beyond optional lifetime markers |
| `array_init_start` | initialize the array-initializer state record `{base, index, count}` |
| `array_init_more` | compare current index against element count and return a boolean |
| `array_init_element` | compute the current element address from the initializer state and alias the destination slot to it |
| `array_init_index` | expose the current element index without advancing initialization state |
| `array_init_finish` | no direct code required beyond optional lifetime markers |

### Allocation and deallocation

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `constexpr_alloc` | allocate one object worth of raw storage using the native fallback allocation runtime |
| `constexpr_alloc_multiple` | allocate `count * element_size` bytes using the native fallback allocation runtime |
| `constexpr_dealloc` | deallocate one previously allocated object using the native fallback deallocation runtime |
| `constexpr_dealloc_multiple` | deallocate a previously allocated contiguous allocation using the native fallback deallocation runtime |

### Globals, statics, and initguards

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `get_global_storage` | materialize the address of mutable global storage |
| `get_antestatal_ref` | materialize the address of readonly antestatal storage |
| `initguard_global_get_ref` | materialize the address of the initguard backing object |
| `initguard_complete` | call the initguard completion runtime helper |
| `initguard_abort` | call the initguard abort runtime helper |

### Destruction, lifetime, and assertions

| VMIR2 instruction | LLVM lowering |
| --- | --- |
| `destroy` | destroy the live slot immediately; if the slot type has a non-trivial destructor, emit that destructor call first, then poison the storage bytes; otherwise poison the storage bytes and end the slot lifetime directly |
| `end_lifetime` | end the slot lifetime without invoking a destructor; if the slot is storage-backed, the operation is valid only when no active object remains in the storage, and the ended storage is thereafter treated as poisoned/undefined |
| `defer_nontrivial_dtor` | attach the specified destructor invocation to the live slot so that later lifetime transition cleanup emits it exactly once |
| `assert_instr` | branch on truth value; on failure trap or otherwise terminate the current execution |
| `unimplemented` | emit an unconditional trap representing an executed `UNIMPLEMENTED` statement |

### Constexpr-result instructions

These instructions are VM/constexpr result materialization operations and are not part of native LLVM lowering.
If they appear in a routine selected for native lowering, lowering must fail.

| VMIR2 instruction | Native lowering rule |
| --- | --- |
| `constexpr_set_result` | invalid in native LLVM lowering |
| `constexpr_set_result2` | invalid in native LLVM lowering |
| `constexpr_make_proxy` | invalid in native LLVM lowering |
| `constexpr_output_byte` | invalid in native LLVM lowering |

## Terminator Lowering

| VMIR2 terminator | LLVM lowering |
| --- | --- |
| `jump` | unconditional branch to the target block after required edge cleanup |
| `branch` | conditional branch to the true or false target after required edge cleanup on each outgoing edge |
| `ret` | emit required return cleanup, then return the ABI result if one exists, otherwise `ret void` |
| `runtime_constexpr` | branch to the native target after required edge cleanup |
| `initguard_try_acquire` | call the initguard acquire runtime helper, branch to acquired or already-initialized successor, materialize the acquired lock slot on the success edge |

## Reserved VMIR2 operations not currently in the executable instruction variant

The following operations exist as VMIR2 instruction forms but are not currently part of the executable instruction variant.
When activated in the future, they should lower as follows:

| VMIR2 instruction | Intended LLVM lowering |
| --- | --- |
| `fence_byte_acquire` | acquire fence or equivalent visibility barrier for the affected byte domain |
| `fence_byte_release` | release fence or equivalent visibility barrier for the affected byte domain |

## Validation Rules

Native lowering must reject a routine when any of the following is true:

- a slot type has no runtime lowering category,
- a required aggregate placement or struct layout is missing,
- an interface record layout is missing,
- a readonly antestatal reference does not have enough initializer information to materialize its required constant definition,
- a constexpr-result instruction appears in native lowering,
- an instruction requires a runtime helper or ABI contract that is not available,
- a pointer-difference or pointer-arithmetic operation cannot determine its element type.
