# Variables

`VAR` declares a mutable object or binding. A variable always has a type and is
initialized as part of its declaration.

## Declaration forms

A function-local variable places its name between `VAR` and its type:

```quxlang
VAR count I32 := 4;
```

A namespace-scope variable is a named declaration, so the name precedes
`VAR`:

```quxlang
::request_count VAR U64 := 0;
```

A structure data member uses the same named-declaration order:

```quxlang
::coordinate STRUCT
{
  .x VAR I32;
  .y VAR I32;
}
```

The terminating semicolon is required in every form. Data-member declarations
do not accept an initializer at the point of declaration; constructors
initialize their owning object and may assign the members.

## Types

The type after a local variable name is normally explicit:

```quxlang
VAR enabled BOOL := TRUE;
VAR bytes [4]BYTE;
VAR cursor MUT=>>BYTE;
```

A function-local `AUTO` variable deduces its type from one `:=` initializer.
No other initializer form is valid with `AUTO`:

```quxlang
VAR selected AUTO := callback;
```

`AUTO` removes ordinary reference qualification and constructs an independent
value. Initializing it from a referenced object copies or moves that object as
appropriate; copying a pointer preserves its pointer value. Explicit reference
types create aliases. Attached callable bindings retain their binding behavior.

```quxlang
VAR source I32 := 7;
VAR copy AUTO := source;
source := 11;
TEST_ASSERT(copy == 7);
```

An explicitly typed attached-reference variable also requires exactly one
`:=` initializer:

```quxlang
VAR value I32 := 5;
VAR alias MUT& I32 := value;
alias := 8;
ASSERT(value == 8);
```

See [References](references.md) for the reference qualifiers and binding rules.

## Initialization

The declaration selects one of four initialization forms:

| Form | Meaning |
| --- | --- |
| `VAR value T;` | Invoke `T.CONSTRUCTOR` without user arguments. |
| `VAR value T := expression;` | Invoke `T.CONSTRUCTOR` with the expression as the named `OTHER` argument. |
| `VAR value T :(arguments);` | Pass ordinary positional or named arguments to `T.CONSTRUCTOR`. |
| `VAR value T :[expressions];` | Pass a positional initialization sequence. |

For example:

```quxlang
VAR zero I32;
VAR copied I32 := zero;
VAR boxed lifecycle_boxed_union :(@number (3 AS I32));
VAR digits [4]I32 :[2, 4, 6, 8];
```

Omitting an initializer does not leave an object uninitialized. It selects the
no-argument constructor. Built-in integers therefore start at zero, pointers
start null, and user types must have an applicable default constructor.

`:=` is construction when it occurs in a declaration. A later `:=` expression
is assignment to an already-live object and follows the rules on
[Assignment Operators](assignment-operators.md).

`:[...]` supplies one entry for each element of a fixed-size array. Too many or
too few entries are ill-formed. A user type may also provide a positional
constructor that accepts this form. See [Arrays](arrays.md).

Raw `STORAGE` and `ALIGNED_STORAGE` variables do not accept any direct
initializer. Their declaration establishes storage, not an object of a payload
type; `PLACE` begins a payload lifetime in that storage.

## Local scope and lifetime

A local name becomes visible after its declaration and remains visible to the
end of its enclosing block, subject to ordinary nested-block shadowing rules.
Its object lifetime begins only after its constructor completes.

Leaving the scope destroys a nontrivial local object. This includes fallthrough
and control-flow exits such as `RETURN`, `BREAK`, and `CONTINUE` that cross the
scope boundary. Objects are destroyed in the reverse of their completed
construction order. See [Object Storage and Lifetime](typed-storage-and-lifetime.md)
for explicit `PLACE` and `DESTROY` operations.

## Namespace-scope variables

A namespace-scope `VAR` names one program-wide mutable object. Accessing the
name produces access to that object, rather than a copy of it:

```quxlang
::next_identifier VAR U64 := 1;

::allocate_identifier FUNCTION(): U64
{
  VAR result U64 := next_identifier;
  next_identifier++;
  RETURN result;
}
```

The declaration may use the same no-argument, `:=`, `:(...)`, and `:[...]`
initializer forms as a local variable. Initialization and destruction are
managed for the program object. Code that shares a mutable global between
threads must still provide synchronization; `VAR` does not imply atomic access.

Use [`VAR PER_THREAD`](thread-local-variables.md) when each thread needs its own
instance. Use [`STATIC`](static-compile-time-constants.md) for a compile-time
constant and `STATIC_VAR` for mutable state used during
[Compile-Time Evaluation](compile-time-evaluation.md). Those are different declaration
categories, not storage qualifiers on an ordinary `VAR`.

## Reserved declaration tags

The declaration parser accepts `CONSTEXPR_READABLE` and
`CONSTEXPR_READWRITE` between `VAR` and the type in a named declaration. The
current language semantics do not assign an access contract to either tag.
Programs should treat them as reserved syntax rather than usable variable
qualifiers.

## Global access during constant evaluation

A namespace-scope `VAR` requires `CONSTEXPR_OK` before constant evaluation can
obtain a reference to it, read or modify it, or take its address:

```quxlang
::counter VAR CONSTEXPR_OK I32 := 0;
::thread_counter VAR PER_THREAD CONSTEXPR_OK I32 := 0;
```

An unmarked global remains available at runtime. Accessing it during constant
evaluation fails even if it has a zero or constant initializer. `STATIC`
constants remain available without this modifier. `CONSTEXPR_READABLE` and
`CONSTEXPR_READWRITE` do not grant this permission.
