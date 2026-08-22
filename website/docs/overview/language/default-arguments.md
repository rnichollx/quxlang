# Overview of Default Arguments

Default arguments let callers omit values that have a common choice:

```quxlang
::add FUNCTION(
  @ARG:value I32,
  @amount:delta I32 DEFAULT(1)
): I32
{
  RETURN value + delta;
}

ASSERT(add(4) == 5);
ASSERT(add(@ARG 4, @amount 3) == 7);
```

The expression after `DEFAULT` is used only when the argument is omitted.

Named arguments can skip an earlier default while supplying a later parameter:

```quxlang
::area FUNCTION(@width I32 DEFAULT(10), @height I32): I32
{
  RETURN width * height;
}

ASSERT(area(@height 4) == 40);
```

The default expression is resolved where the function is declared, so a local
name at the call site does not replace a declaration-level name used by the
default.

For evaluation, overload, template, and viability rules, see the
[Default Arguments Reference](../../reference/default-arguments.md).
