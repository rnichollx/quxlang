# References and Pointers

Reference and pointer forms read from left to right:

```quxlang
VAR value I32 := 7;

VAR mutable_reference MUT& I32 := value;
VAR constant_reference CONST& I32 := value;
VAR instance_pointer MUT->I32 := value<-;

VAR values [4]I32 :[10, 20, 30, 40];
VAR iterator MUT=>>I32 := values[& 0];

mutable_reference := 8;
instance_pointer-> := 9;
(iterator + 2)-> := 31;
```

## Reference qualifiers

- `MUT& T` is a mutable reference.
- `CONST& T` is a constant reference.
- `TEMP& T` denotes an expiring value category.
- `WRITE& T` grants write access without promising readable input state.
- `AUTO& T` deduces the incoming reference qualifier.

A reference must attach to valid storage; a pointer may be null.

## Pointer categories

- `MUT->T` and `CONST->T` are instance pointers.
- `MUT=>>T` and `CONST=>>T` are array or multi-object pointers.
- `~>T` is a garbage-collected reference used by layoutless runtimes such as
  the JVM.
- An omitted pointer qualifier is mutable, so `->T` and `=>>T` are accepted
  short forms.

## Pointer expressions

- `value<-` takes an object's address.
- `pointer->` dereferences an instance or array pointer.
- `array[& index]` obtains an array pointer at an element.
- `array[index]` accesses an element.
- Arithmetic, `++`, and `--` advance an array pointer.
- `pointer??` tests for a value and `pointer?!` tests for null.

Pointer layers compose directly:

```quxlang
VAR value I32 := 5;
VAR pointer ->I32 := value<-;
VAR pointer_to_pointer -> ->I32 := pointer<-;

pointer_to_pointer-> -> := 10;
ASSERT(value == 10);
```

## Forwarding references

`AUTO& AUTO` deduces both the reference qualifier and target type. `FORWARD`
preserves that category:

```quxlang
::forwarded FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

See [Conversions](conversions.md), [Typed storage](typed-storage-and-lifetime.md),
and [`NEW` and `DELETE`](new-and-delete.md).

