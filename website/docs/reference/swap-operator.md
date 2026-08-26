# Swap Operator

`<->` exchanges the values of two mutable objects:

```quxlang
left <-> right;
```

The expression dispatches to `OPERATOR<->` for the operand type with mutable
`THIS` and `OTHER` references. The built-in operator returns `VOID`.

## Built-in and generated swap

Primitive values and pointers have direct swap behavior. Arrays swap
corresponding elements using each element type's swap operator. Structures can
receive generated memberwise swap when every stored member supports it.

Polymorphic structs do not receive generated swap, because swapping only a
static base portion would not define a complete-object operation. They require
an explicit user-defined contract when swap is meaningful. See
[Inheritance](inheritance.md).

```quxlang
VAR left [3]I32 :[1, 2, 3];
VAR right [3]I32 :[4, 5, 6];
left <-> right;
ASSERT(left[0] == 4);
ASSERT(right[0] == 1);
```

Unions and variants exchange their active alternatives, payloads, and valueless
states according to their declared representation. Self-swap preserves the
object.

## User-defined swap

A structure can declare `.OPERATOR<->` when memberwise generation is not its
desired contract. Declaring a user swap suppresses the generated structure
swap. `MOVE_ONLY` also suppresses ordinary generated swap for structures;
fusion types use `NO_DEFAULT_SWAP`.

The implementation must preserve both objects as live values even when their
state is exchanged. Operand aliasing follows the declared parameter contract;
a robust swap should account for self-swap if the call surface permits it.

Swap does not construct a third language-visible object and does not mean move
construction. It is a distinct operator selected directly by `<->`.
