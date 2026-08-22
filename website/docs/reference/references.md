# References

A reference is a non-owning, non-null association with an existing Quxlang
object. Its qualifier describes the access and value category carried by that
association.

| Form | Contract |
| --- | --- |
| `MUT& T` or `&T` | Read and modify an existing `T`. |
| `CONST& T` | Read an existing `T` without modifying it through this reference. |
| `TEMP& T` | Refer to an expiring `T` that may be consumed or moved from. |
| `WRITE& T` | Initialize or replace a `T` without requiring a readable input value. |
| `AUTO& T` | Deduce the incoming reference qualifier while matching target type `T`. |

References do not own their target and do not extend its lifetime.

## Binding references

A reference initializer must identify compatible live storage:

```quxlang
VAR value I32 := 7;
VAR mutable_reference MUT& I32 := value;
VAR constant_reference CONST& I32 := value;

mutable_reference := 8;
ASSERT(constant_reference == 8);
```

`MUT&` requires mutable access. `CONST&` may bind where a read-only view is
valid. Reference qualification participates in overload resolution, so an API
can distinguish mutable, constant, and expiring arguments.

```quxlang
::inspect FUNCTION(@ARG:value CONST& record)
{
  use(value);
}

::update FUNCTION(@ARG:value MUT& record)
{
  value.revision++;
}
```

## `WRITE&` output references

`WRITE&` is for a destination whose previous value is not part of the
function's input contract:

```quxlang
::divide FUNCTION(@numerator I32, @denominator I32,
                  @quotient WRITE& I32, @remainder WRITE& I32)
{
  quotient := numerator / denominator;
  remainder := numerator % denominator;
}
```

The callee must establish a valid result before an operation that requires the
destination to be readable. `WRITE&` is not a mutable-input reference spelled
differently; use `MUT&` when the old value is an input.

## Temporary references and forwarding

`TEMP&` marks an expiring source and is the reference category used by move
conversion constructors. A forwarding parameter combines reference-category
and type deduction:

```quxlang
::identity FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

`AUTO& AUTO` means: deduce the reference qualifier and deduce the referenced
type. `FORWARD(value)` preserves the deduced category. Returning `value`
directly would treat the named parameter as an ordinary named expression and
would not express the same forwarding contract.

See [Move Semantics](move-semantics.md) and
[Type Queries](type-queries-and-deduction.md).

## Reference deduction patterns

Reference forms may contain type patterns:

- `AUTO& AUTO(t)` deduces the qualifier and the non-reference target as `t`.
- `AUTO& TT(t)` can retain a type pattern that includes reference information
  where the surrounding declaration accepts it.
- `CONST& AUTO(t)` fixes const access while deducing the target type.
- `TEMP& AUTO(t)` fixes the expiring category while deducing the target type.

These are template-matching forms, not runtime reference objects with unknown
types.

## Member and return references

A member function can return a reference to storage owned by its receiver:

```quxlang
::box STRUCT
{
  .value VAR I32;

  .get FUNCTION(): MUT& I32
  {
    RETURN .value;
  }
}
```

The caller must not retain that reference after the `box` object leaves its
lifetime. The same rule applies to array elements, iterator dereference, and
references returned by user-defined operators.

## References and object state

A reference itself is not nullable. Use a [Pointer](pointers.md) when the
association may be absent or must be reseated as a stored value. Creating a
pointer with postfix `<-` preserves a reference to the same object but changes
the type-level contract from non-null reference to pointer:

```quxlang
VAR value I32 := 9;
VAR reference MUT& I32 := value;
VAR pointer MUT->I32 := reference<-;
```

Neither form grants access after the object's lifetime ends.
