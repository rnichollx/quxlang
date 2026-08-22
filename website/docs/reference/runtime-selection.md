# Runtime Selection

`RUNTIME` selects between function bodies according to the execution mode. It
lets one function provide a compile-time implementation and an ordinary runtime
implementation, or a native implementation and a portable alternative.

Runtime selection does not test a program value. Its condition is one of the
fixed mode names `CONSTEXPR` or `NATIVE`.

## Grammar

```text
RUNTIME CONSTEXPR block [ELSE block]
RUNTIME NATIVE block [ELSE block]
```

Both forms are statements. The blocks use braces, and no semicolon follows the
statement. `ELSE` is optional; when it is absent, the statement does nothing in
the opposite mode.

## `RUNTIME CONSTEXPR`

The first block is selected while the function is executing in compile-time
evaluation. The `ELSE` block is selected during ordinary runtime execution:

```quxlang
RUNTIME CONSTEXPR
{
  RETURN CONSTEXPR_ALLOC#T();
}
ELSE
{
  RETURN native_allocate#T();
}
```

This form is appropriate when an operation needs a compiler-interpreter
implementation during constant evaluation but uses a runtime service in the
finished program.

Without `ELSE`, the block runs only in compile-time execution:

```quxlang
RUNTIME CONSTEXPR
{
  validate_static_state();
}
```

## `RUNTIME NATIVE`

The first block is the native execution path. The optional `ELSE` block is the
non-native alternative:

```quxlang
RUNTIME NATIVE
{
  deallocate_native(@address address);
}
ELSE
{
  deallocate_portable(@address address);
}
```

This form belongs inside code that shares a language-level contract across
backends but requires a backend-specific implementation. Target availability
for the declarations called by each path still follows
[`INCLUDE_IF`](availability-and-targets.md).

## Branch validity and dependency selection

Unlike `STATIC_IF`, `RUNTIME` does not remove an unselected source body during
static source selection. Both bodies must parse, and each selected mode must
have a valid implementation.

The compile-time dependency analysis follows the constexpr-selectable path. A
native-only path is not treated as a constexpr dependency merely because it is
written in the same function. This permits a `RUNTIME NATIVE` body to call
operations that cannot execute in the compile-time interpreter.

Conversely, the native output need not lower operations that exist only in a
`RUNTIME CONSTEXPR` branch when the ordinary runtime path selects `ELSE`.

## State after the statement

Variables declared inside either block are scoped to that block. Values that
remain live after the statement must have a compatible initialized state on
every path that can reach the following code:

```quxlang
VAR result I32;

RUNTIME CONSTEXPR
{
  result := 10;
}
ELSE
{
  result := 20;
}

RETURN result;
```

Normal destruction and control-flow rules apply within each selected block.
A `RETURN` in a selected body leaves the function; fallthrough converges at the
statement's continuation.

## Distinction from related features

- [`STATIC_IF`](compile-time-evaluation.md) evaluates a program expression at
  generation time and generates only one body.
- `IF` evaluates a program expression at runtime every time execution reaches
  it.
- `INCLUDE_IF` controls whether a declaration is available for a target.
- `RUNTIME` chooses an implementation block according to execution mode.

These forms solve different problems and are not interchangeable.
