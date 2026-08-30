# Overview of MATCH

`MATCH` observes a union or variant and selects an arm by option or type.

When every non-`VOID` variant alternative should compile the same source
region and rely on overload resolution, use [`VISIT`](visit.md) instead.

## Union matching

```quxlang
MATCH failure AS payload
{
  CASE ok
  {
    ASSERT(FALSE);
  }
  CASE error AS code
  {
    ASSERT(code == 5);
  }
}
```

`CASE name` selects a union option. `AS name` on the header binds the payload
for every payload-bearing arm; an arm-local `AS name` replaces that binding for
the arm.

## Variant matching

```quxlang
MATCH number AS payload
{
  TYPE I32 WHERE TRUE
  {
    payload++;
  }
  TYPE I32 OTHERWISE
  {
    payload := 0;
  }
  TYPE VOID
  {
    ASSERT(FALSE);
  }
  DEFAULT FAIL;
}
```

`TYPE T` selects a variant alternative. `WHERE condition` guards an arm.
`OTHERWISE` is the final guarded arm for the same selector. `DEFAULT` handles
unmatched or valueless states; `DEFAULT FAIL;` makes that path an explicit
failure.

## Shadow binding

When the subject is one bare name, `SHADOW` reuses it as the payload binding:

```quxlang
MATCH value SHADOW
{
  TYPE I32 { value++; }
  TYPE VOID { }
}
```

The arm binding is local and does not change the fusion object's type outside
the arm.

An unguarded match must cover the declaration's options or types unless a
`DEFAULT` arm handles what remains. A `DEFAULT` also handles a valueless state;
without one, reaching that state fails. Union arms use `CASE`; variant arms use
`TYPE`.

## Reference

See the [MATCH Reference](../reference/match.md) for the complete
language rules, constraints, and technical edge cases.
