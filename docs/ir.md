# Qx VMIR Spefication


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
transitioned from nonexistent to DS-state (delegated storage) and bound as a delegate-of to the target local.

### SCN - Struct Complete New

The `SCN` instruction is used to transition a local from P-state to A-state. Each delegate bound to the target is 
transitioned from D-state to a discarded state, without invoking any destructors. It is an invalid instruction to 
execute `SCN` on a local that is not in P-state, or where any of the delegates are not in DA-state.




