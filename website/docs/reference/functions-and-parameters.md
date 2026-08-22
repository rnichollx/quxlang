# Functions

A function declaration defines a callable symbol, its parameter interface, an
optional return type, and an executable body.

```quxlang
::clamp FUNCTION(@value I32, @minimum I32, @maximum I32): I32
{
  IF (value < minimum) { RETURN minimum; }
  IF (value > maximum) { RETURN maximum; }
  RETURN value;
}
```

An ordinary function declaration always has a body. Signature-only functions
occur in interfaces and generic surfaces, while external declarations use
`EXTERN_PROCEDURE`.

## Named parameters

`@api_name Type` declares a named parameter. The API name is written explicitly
at multi-argument call sites and is also the default local name:

```quxlang
::divide FUNCTION(@numerator I32, @denominator I32): I32
{
  RETURN numerator / denominator;
}

VAR result I32 := divide(@numerator 20, @denominator 4);
```

`@api_name:local_name` gives the body a different local spelling without
changing the call interface:

```quxlang
::offset FUNCTION(@value:v I32, @amount:d I32): I32
{
  RETURN v + d;
}
```

`@ARG` is the conventional name for a one-argument interface. A call with one
bare argument binds it to `@ARG`:

```quxlang
::twice FUNCTION(@ARG I32): I32 { RETURN ARG * 2; }
ASSERT(twice(6) == 12);
```

The bare form is limited to exactly one argument. Other calls write every named
argument as `@name expression`.

## Positional parameters

`%local_name Type` declares a positional parameter:

```quxlang
::sum_pair FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

ASSERT(sum_pair(% [4, 5]) == 9);
```

`%IGNORED Type` consumes one positional argument without introducing a local
name. Positional arguments are passed in `% [...]` groups. A signature may mix
named and positional parameters; argument mapping uses the named API names and
the source order of positional formals independently.

## Variadic positional parameters

`%...name Type` consumes the remaining positional values as a pack.
`%...IGNORED Type` accepts the pack without naming it.

Only one positional pack is permitted, and no ordinary positional parameter
may follow it. Named variadic packs (`@...`) are not supported. Pack element
types are checked or deduced as described in [Variadic Packs](variadic-packs.md).

## Parameter types and access

Value parameters initialize a local parameter object. Reference types express
access to the caller's object:

- `CONST& T` permits reads;
- `MUT& T` permits reads and writes;
- `WRITE& T` is a pure output destination that the callee must initialize
  before reading;
- `TEMP& T` binds the temporary-value category;
- `AUTO& AUTO` deduces a reference and its qualifier.

Pointer and procedure types are ordinary parameter types. `AUTO`, `AUTO(name)`,
and composite patterns make the function deduced for the concrete call; see
[Type Queries and Deduction](type-queries-and-deduction.md) and
[Overload Resolution](overload-resolution.md).

## Default arguments

`DEFAULT(expression)` follows the parameter type:

```quxlang
::increase FUNCTION(@value I32, @amount I32 DEFAULT(1)): I32
{
  RETURN value + amount;
}
```

Omitted parameters with defaults are initialized at the call. Missing required
parameters make the function non-viable. Full declaration-context and overload
rules are on [Default Arguments](default-arguments.md).

## Return type

The return type follows `:` after the parameter list and any `ENABLE_IF`:

```quxlang
::absolute FUNCTION(@ARG:value I32): I32
{
  IF (value < 0) { RETURN 0 - value; }
  RETURN value;
}
```

Omitting `: Type` declares a `VOID` function. `AUTO` and other type patterns can
deduce the result from value-returning paths. Every reachable exit must satisfy
the selected result contract. See [`RETURN` Statements](return-statements.md).

## Overloads and `ENABLE_IF`

Several declarations may share a name when their callable signatures differ:

```quxlang
::width FUNCTION(@ARG I32): I32 { RETURN 32; }
::width FUNCTION(@ARG I64): I32 { RETURN 64; }
```

Parameter binding, type adaptation, template deduction, candidate priorities,
and `ENABLE_IF` determine the selected declaration. `ENABLE_IF(condition)` is
written after the parameter list and before the return type:

```quxlang
::small_width FUNCTION(@ARG AUTO(value_type))
  ENABLE_IF(BITS(value_type) < 32 AS I32): I32
{
  RETURN BITS(value_type) AS I32;
}
```

Its condition is evaluated for the concrete candidate instantiation. See
[Overload Resolution](overload-resolution.md) for the complete ranking rules.

## Member and nested functions

Within a structure, `.` declares an instance member and `::` declares a nested
function that receives no object:

```quxlang
::counter STRUCT
{
  .value VAR I32;

  .read FUNCTION() CONST: I32 { RETURN .value; }
  .set FUNCTION(@ARG I32) MUT { .value := ARG; }

  ::zero FUNCTION(): counter { RETURN counter(); }
}
```

The suffix `MUT`, `CONST`, `WRITE`, or `TEMP` declares the receiver qualifier
and is equivalent to an explicit `@THIS <qualifier>& THISTYPE` parameter.
`THIS` names the current object; `.member` is shorthand for its member.
Receiver and field rules are detailed on [Structures](structs-and-members.md).

## Calls and function values

Named and positional call arguments are evaluated in source order before the
selected function body begins. Calls may target a function symbol, member,
procedure pointer, or callable function value. See [Call Arguments](call-arguments.md)
and [Procedure Pointers and Function Values](procedure-pointers-and-function-values.md).
