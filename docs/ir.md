# Qux VMIR Specification


# Definitions

# Local State

Local state is the type of state permissible for a local.

* null or discarded state - The local does not currently exist.
* A-state - The local is currently *active* and has a normal value.
* S-state - S-stage designates a local which has *storage* but is not *active*. The contents of memory in an S-state local
  are undefined.
* P-state - Designates a partially constructed local. The contents of memory in a P-state local are defined by the
  delegates bound to the local. Regions bound to P, S, or A state locals defined according to the rules of each type.
* DS-state - Like S-state, but for a value that is a delegate of a P-state.
* DA-state - Designates a delegate that is active and has a normal value prior to delegate finalization.
* DP-state - A value which is both a delegate of another value and itself has delegates.

## Slot types (NEW&& and DESTROY&&)

NEW&& and DESTROY&& are slot types, not reference types. They describe how argument storage and lifetime are transferred between a callsite and a callee, and values of slot type cannot exist as true runtime locals.

- A function may declare parameters of slot type. At runtime, the slot governs a callee-local ordinary local (of the parameter's target type), but there is no distinct "reference object" created to model the transfer.
- Slots behave reference-like in that control of the lifetime/initialization of a callee-local is transferred from the caller to the callee without materializing a separate reference value.
- Slots compose with references. For example, `NEW&& &I32` is a valid parameter type and means the callee receives a slot governing a reference-to-I32 local.

Semantics at function entry/exit (normal):
- NEW&& T: At entry, the callee has storage for a local of type `T` in S-state. On normal return, that local must be fully constructed (A-state). On exception exit, storage remains valid (S-state) but the value need not be constructed.
- DESTROY&& T: At entry, the callee has an A-state local of type `T` that it must consume/destroy. On both normal and exception return, the value is no longer active (discarded) but the storage remains valid (S-state).

These semantics match the codegen state engine’s handling of `nvalue_slot` (NEW&&) and `dvalue_slot` (DESTROY&&) at entry and exit.

# Output Transition

An output transition tranforms a local from a nullstate to A-state, or from DS-state to DA-state.

# Consume Transition

A consume transition transitions a local from A-state to a discarded state.

# Readonly Transition

A readonly does not cause a state change, but the input must usually be in A-state, P-state, or DP-state, or DA-state.

## SDN - Struct Delegate New

### Arguments

  * `local` "target" - The local to operate on.
  * `args`  "delegates" - The list of delegates. 

### Description

The `SDN` instruction is used on a local which is in the storage initialized state (S-state). The `SDN` instruction
transitions the target from S-state to P-state (partial). Each values in the `args` list is a delegate that is 
transitioned from nonexistent to DS-state (delegated storage) and bound as a delegate to the target local.

### SCN - Struct Complete New

The `SCN` instruction completes delegated construction. It supports two cases:

- P -> A: when the target local is not itself a delegate (no delegate_of), SCN transitions the target from P-state to A-state.
- DP -> DA: when the target local is itself a delegate (delegate_of is set) and has delegates, SCN transitions the target from DP-state to DA-state.

In both cases, each bound delegate of the target is transitioned to discarded (no destructors are invoked during SCN), and the target’s delegate list is cleared. It is invalid to execute `SCN` when the target does not have initialized delegates or when any of its delegates are not in DA-state.




