# Overview of Unions

A union holds one of several named options. The option names identify the
possible states.

## Option declarations

```quxlang
::status INLINE_UNION
{
  .ok OPTION DEFAULT VOID;
  .error OPTION I32;
}
```

`ok` is a payload-free state because its type is `VOID`. `error` carries an
`I32`. `DEFAULT` makes `ok` the state selected by the no-argument constructor.

## Union construction

Default construction selects the default option:

```quxlang
VAR success status;
ASSERT(success IS ok);
```

To select another option, pass its name as a constructor argument:

```quxlang
VAR failure status :(@error 5);
ASSERT(failure IS error);
```

`IS` tests the active option and returns `BOOL`.

## Payload access

The `MATCH` statement selects an option and binds the selected payload:

```quxlang
::error_code FUNCTION(@ARG:value CONST& status): I32
{
  MATCH value AS payload
  {
    CASE ok { RETURN 0; }
    CASE error { RETURN payload; }
  }
}
```

The `payload` name is available only for options that carry a value. Matching
every option makes it clear how each state is handled.

## Inline and boxed unions

`INLINE_UNION` stores the payload inside the object. `UNION` stores it in boxed
storage, which permits direct recursive shapes:

```quxlang
::node UNION
{
  .end OPTION DEFAULT VOID;
  .next OPTION node;
}
```

The type declaration determines the representation. Construction and matching
use the same syntax for both representations.

## Valueless unions

Some lifecycle operations can leave an ordinary union with no active option.
`value??` reports an active option and `value!?` reports a valueless state.

The `NEVER_VALUELESS` policy requires the type to retain an active option. The
`VALUELESS_DEFAULT` policy makes default construction create no active option.
Both policies affect default construction and moved-from state.

## Reference

For option constraints, lifecycle modifiers, generated operations, and
valueless rules, see the [Unions Reference](../reference/unions.md).
