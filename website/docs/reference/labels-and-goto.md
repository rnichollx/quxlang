# Labels and `GOTO`

Labels name control-flow targets inside a function. Loop labels select a
particular enclosing loop for `BREAK` or `CONTINUE`; `LABEL` statements and
blocks provide explicit targets for `GOTO` or labeled `BREAK`.

## Label names

A label reference is written as a colon followed by a lowercase identifier:

```quxlang
:retry
```

Whitespace and comments may appear after the colon. Label names follow the
ordinary identifier grammar. Labels are function-local control-flow names, not
namespace declarations or runtime values.

## Labeled loops

`WHILE` and `LOOP` accept a label immediately after the loop keyword:

```quxlang
LOOP :outer VALUE(i) FROM(0 AS I32) TO(4) DO
{
  WHILE :inner (condition)
  {
    CONTINUE :outer;
  }
  BREAK :outer;
};
```

`BREAK :name;` exits the matching labeled loop. `CONTINUE :name;` begins its
next iteration using that loop's normal step or condition path. The target must
be an enclosing active loop.

Without a label, `BREAK;` selects the nearest enclosing breakable construct and
`CONTINUE;` selects the nearest enclosing loop. A label that cannot be resolved
is ill-formed.

## Labeled blocks

`LABEL :name` followed by a block creates a breakable block:

```quxlang
LABEL :done
{
  IF (finished)
  {
    BREAK :done;
  }
  perform_more_work();
}
```

`BREAK :done;` transfers to the point after the block. A labeled block is not a
loop, so `CONTINUE :done;` is ill-formed.

## Statement labels and `GOTO`

`LABEL :name;` declares a statement target. `GOTO :name;` transfers control to
it:

```quxlang
VAR attempts I32 := 0;
LABEL :retry;
attempts++;

IF (attempts < 3)
{
  GOTO :retry;
}
```

Both statements require a semicolon. A `GOTO` may refer to a label written
later in the function, and backward jumps are valid when the lifetime state is
compatible. Every referenced label must exist, and a statement label name may
be declared only once in the function.

`GOTO` targets statement labels, not loop labels or the entry of a labeled
block. Use labeled `BREAK` and `CONTINUE` for structured targets.

## Scope and object lifetime

A jump may leave nested blocks. Objects whose live scopes are exited are
destroyed according to ordinary lifetime rules before control reaches the
target:

```quxlang
{
  VAR temporary resource;
  GOTO :after;
}
LABEL :after;
```

A jump cannot enter a scope in a state that assumes skipped construction. For
example, jumping from outside a block to a label after a local variable's
declaration is ill-formed because the target state would contain an object whose
constructor never ran.

The compiler compares the control-flow state at every incoming jump with the
state established at the label. Jumps that would create incompatible live
locals, bindings, or destruction obligations are rejected.

## Choosing a control-flow form

Prefer `IF`, loops, `RETURN`, and labeled `BREAK` for structured control flow.
`GOTO` is appropriate for function-local transfers whose lifetime state is
explicit and which are clearer than duplicating cleanup or nesting many
conditions. It cannot cross a function boundary.
