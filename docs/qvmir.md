Qux VM Intermediate Representation (QVMIR)

## Local Storage

Each instruction operates on a local storage.

Storage can be initialized either by an argument or by an instruction.

### Storage Initialization

Storage can be initialized by:

* Arguments to the IR routine, when the routine is called.
* By the IVK instruction (sort-of).
* By the LCI instruction (load const int, for primitives).
* By the LCZ instruction (load const zero, for primitives).
* By the LFR instruction (load from reference).
* By the DRP instruction (dereference pointer), which ironically creates a reference from a pointer.
* By the DLG instruction, which delegates struct fields to locals.

Note that storage initialization is different from the lifetime start.

A storage may be in 3 states:

* Dead/Uninitialized: The storage is not initialized.
* Initialized/Partially constructed:
* Alive/Fully constructed: The storage is fully initialized.

All slots may be seen as references, even slots that are not "references".

IVK is the `invoke` instruction, which is used to invoke a routine.

Note: In Quxlang terminology, the difference between a "routine" and a "procedure" is that a *routine* always refers to the QVMIR representation, whereas a *procedure* always refers to generated machine code.

#### IVK

When a function is invoked, the function has the following properties:

Input argument values become dead in the caller after the function returns,
unless they are NEW& T arguments.



