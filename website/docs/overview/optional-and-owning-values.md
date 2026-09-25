# Optional and Owning Values

The `std` module provides `optional#T` for an inline value that may be absent,
and `unique_ptr#T` for ownership of one allocated object.

## Optional values

```quxlang
IMPORT std;

::optional_example FUNCTION(): I32
{
  VAR number std::optional#I32 := NULL;
  TEST_ASSERT(number!?);
  number.emplace(@OTHER 42);
  TEST_ASSERT(number??);
  VAR result I32 := number->;
  number := NULL;
  RETURN result;
}
```

`has_value()` and postfix `??` test engagement; `!?` negates that test.
`value()` and postfix `->` access the contained value and require engagement.
`reset()` destroys it. `emplace(...)` forwards constructor arguments.
Moving an optional leaves its source engaged with a moved-from value.

## Owning pointers

```quxlang
IMPORT std;

::ownership_example FUNCTION(): I32
{
  VAR first AUTO := std::make_unique#I32(@OTHER 42);
  VAR second std::unique_ptr#I32 := MOVE(first);
  TEST_ASSERT(first!?);
  RETURN second->;
}
```

A move transfers ownership and empties the source. Destruction deletes the
owned object. `get()` borrows the raw pointer; `release()` transfers deletion
responsibility to the caller. Both types support `value->.field` when their
contained object has fields.

See the [reference](../reference/optional-and-owning-values.md) for access,
construction, and ownership requirements.
