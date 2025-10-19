# Index Form

This document describes a method for handling ctors/dtors in the VMIR representation.

Supposing that we have a collection of "slots" during execution of a function,

A slot is typed (e.g. `I32`) and can be in a live or dead state.

Slots are created for each argument, local variable, temporary, and return value.

At the start of the function execution, all argument slots are considered "alive" and all other slots are considered "dead".

The only way to manipulate a slot's lifetime is through the INVOKE instruction.

INVOKE takes slots as arguments.

e.g.

INVOKE foo_functanoid@#{...} %0, %1, %3

When a callable is invoked, the following properties are noted:

- If the parameter is any reference type REF& T, the slot must be of a compatible REF& T type and must be alive before and after the call.

- If the parameter is accepted by value, the slot is "alive" before the call and "dead" after the call.

- If the parameter is of type NVALUE<T>, the slot must be of type T, and is "dead" before the call and "alive" after the call if no exception is thrown.

- If the parameter is of type DVALUE<T>, the slot must be of type T, and is "alive" before the call and "dead" after the call regardless of exceptions.

Return values are implemented as NVALUE arguments.



