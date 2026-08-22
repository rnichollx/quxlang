# Compile-Time Evaluation

Quxlang can execute selected expressions and statements while generating a
function. Compile-time evaluation supports mutable generation state, branch
selection, repeated generation, and capturing the resulting state for ordinary
code.

This page covers function-local `STATIC`, `STATIC_VAR`, `STATIC_EVAL`,
`STATIC_IF`, `STATIC_WHILE`, `STATIC_CHOOSE`, and `SNAPSHOT`. Namespace-scope
constants and their supported representations are specified on
[`STATIC` Compile-Time Constants](static-compile-time-constants.md).

## Function-local static declarations

Two declaration forms create generation-time bindings:

```quxlang
STATIC limit U64 := 4;
STATIC_VAR index U64 := 0;
```

`STATIC` is constant after initialization. `STATIC_VAR` may be changed by
compile-time evaluation. Both are initialized by the compile-time evaluator,
and both accept the no-argument, `:=`, `:(...)`, and `:[...]` constructor forms.

Their names follow lexical block scope. A visible static name cannot be
redeclared by a nested static declaration, and a static name cannot conflict
with a visible runtime local. `STATIC_VAR` is valid only inside a function body;
namespace-scope mutable storage uses `VAR`.

## `STATIC_EVAL`

`STATIC_EVAL expression;` evaluates its expression during function generation
and discards the expression result:

```quxlang
STATIC_VAR count U64 := 0;
STATIC_EVAL count++;
STATIC_EVAL count := count + 2;
```

The semicolon is required. The expression receives mutable access to visible
`STATIC_VAR` state. Attempting to mutate a `STATIC` binding is ill-formed.

Generation-time execution is different from runtime control flow. A normal
runtime `IF` or `WHILE` has its body generated regardless of how often that
body later executes. Consequently, a `STATIC_EVAL` encountered while generating
a normal loop body runs once, not once per runtime iteration. Both bodies of a
normal `IF` are generated, so a `STATIC_EVAL` in each body is encountered once.
Use `STATIC_IF` and `STATIC_WHILE` when compile-time state should control which
source is generated or how many times it is generated.

## `STATIC_IF`

`STATIC_IF` evaluates a condition as `BOOL` during generation and generates
only the selected body:

```quxlang
STATIC_IF(ARCH_IS_X64)
{
  result := 64;
}
STATIC_ELSE
{
  result := 32;
}
```

The alternative is introduced by `STATIC_ELSE`, not `ELSE`. It is optional.
Chains use `STATIC_ELSE STATIC_IF`:

```quxlang
STATIC_IF(ARCH_IS_X64)
{
  result := 64;
}
STATIC_ELSE STATIC_IF(ARCH_IS_X86)
{
  result := 32;
}
STATIC_ELSE
{
  result := 0;
}
```

The unselected body is not generated. Names and operations that exist only for
the selected configuration can therefore appear in a selected branch without
requiring the other branch to be a valid runtime path. The entire statement
must still be syntactically valid because parsing precedes selection.

The condition is evaluated once. It may read compile-time constants and target
predicates and may participate in the current mutable static evaluation state.

## `STATIC_CHOOSE`

`STATIC_CHOOSE(condition, true_expression, false_expression)` is the expression
form of static selection:

```quxlang
VAR width I32 := STATIC_CHOOSE(ARCH_IS_X64, 64, 32);
```

The condition is evaluated as a compile-time `BOOL`. Only the selected
expression is generated and determines the resulting value and type. The
unselected expression is not resolved as an ordinary expression; this permits
target-specific names in the appropriate arm.

`STATIC_CHOOSE` is not a runtime conditional expression. There is no implemented
runtime `CHOOSE` language form; use `IF` when runtime control flow is required.

## `STATIC_WHILE`

`STATIC_WHILE` repeatedly evaluates its condition and generates its body while
the result is true:

```quxlang
VAR result I32 := 0;
STATIC_VAR index U64 := 0;

STATIC_WHILE(index < 3)
{
  result := result + 2;
  STATIC_EVAL index++;
}
```

This produces three copies of the ordinary assignment in the generated
function. The condition is reevaluated before every generation iteration. If it
is false initially, the body is not generated.

The loop must make its condition false through compile-time state or otherwise
terminate by a compile-time failure. `STATIC_WHILE` has no label syntax and is
not an ordinary runtime `BREAK` or `CONTINUE` target.

## `SNAPSHOT`

Ordinary runtime code cannot directly read mutable function-local static state.
`SNAPSHOT(name)` captures the named binding's current expanded value and makes
that captured value available to ordinary code:

```quxlang
STATIC_VAR count I32 := 1;
VAR before I32 := SNAPSHOT(count);
STATIC_EVAL count++;
VAR after I32 := SNAPSHOT(count);

ASSERT(before == 1);
ASSERT(after == 2);
```

The argument is a visible function-local static identifier, not an arbitrary
expression. Each occurrence captures the state at that generation point;
subsequent mutation does not change an earlier captured object.

When the captured value contains pointers into other captured static state, the
captured graph preserves those relationships for that snapshot. A later
snapshot reflects later mutations and receives its own captured state.

Directly using a `STATIC_VAR` in runtime code without `SNAPSHOT` is ill-formed.
Evaluating `SNAPSHOT(x)` once does not authorize later direct accesses to `x`.

## Relationship to runtime selection

Compile-time selection decides what code is generated. [`RUNTIME CONSTEXPR`
and `RUNTIME NATIVE`](runtime-selection.md) instead describe code paths selected
by the execution mode. Use the distinction deliberately: target predicates and
static generation belong here; alternate constexpr/native implementations
belong in runtime selection.
