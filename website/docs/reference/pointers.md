# Pointers

Quxlang distinguishes instance pointers, array pointers, and managed-runtime
references. These categories have different arithmetic and representation
contracts.

| Form | Category |
| --- | --- |
| `MUT->T`, `CONST->T` | Pointer to one `T` object |
| `MUT=>>T`, `CONST=>>T` | Pointer into a sequence of `T` objects |
| `~>T` | Managed-runtime reference to `T` |

Omitting `MUT` gives the mutable short forms `->T` and `=>>T`. `&T` is a
reference, not a pointer.

## Instance pointers

Postfix `<-` obtains a pointer from an object reference. Postfix `->`
dereferences a native instance or array pointer:

```quxlang
VAR value I32 := 7;
VAR pointer MUT->I32 := value<-;

pointer-> := 9;
ASSERT(value == 9);
```

Pointer layers compose from left to right:

```quxlang
VAR value I32 := 5;
VAR pointer ->I32 := value<-;
VAR pointer_to_pointer -> ->I32 := pointer<-;

pointer_to_pointer-> -> := 10;
ASSERT(value == 10);
```

`CONST->T` does not permit mutation of `T` through that pointer. Qualifiers are
part of the pointer type and participate in conversion and overload matching.

## Null pointers and booliation

`NULL` constructs a compatible non-reference pointer value. `pointer??` tests
for a non-null pointer and `pointer?!` tests for null:

```quxlang
VAR pointer ->I32 := NULL;
ASSERT(pointer?!);

pointer := value<-;
IF (pointer??)
{
  use(pointer->);
}
```

Dereferencing a null pointer is invalid. A booliation test does not extend the
lifetime of the pointed-to object or establish that a non-null pointer is valid
to dereference.

## Array pointers

`=>>T` denotes a position in a multi-object allocation or array. Indexing an
array with `[& index]` produces one:

```quxlang
VAR values [4]I32 :[10, 20, 30, 40];
VAR iterator MUT=>>I32 := values[& 0];

ASSERT(iterator-> == 10);
ASSERT((iterator + (2 AS SZ))-> == 30);
ASSERT(iterator[3] == 40);
```

Array pointers support element-based `+`, `-`, `++`, and `--`. Subtracting two
compatible array pointers produces a signed pointer-sized element distance.
They also support three-way ordering. These operations require pointers into a
compatible valid sequence.

An instance pointer does not become an array pointer merely because `T` has a
known size. Use `->T` for one object and `=>>T` for a sequence position.

## `VOID` pointers

Native instance and array pointers may be explicitly reinterpreted through
`->VOID` or `=>>VOID` when an untyped native boundary requires it:

```quxlang
VAR typed ->I32 := value<-;
VAR erased ->VOID := typed AS REINTERPRET ->VOID;
VAR restored ->I32 := erased AS REINTERPRET ->I32;
```

A `VOID` pointer cannot be dereferenced because it has no element type. Array
arithmetic also requires a non-`VOID` element size. `REINTERPRET` does not
create object lifetime; the restored type must describe an object that is
actually valid at that address. See [Conversions](conversions.md).

## Managed references

`~>T` is a garbage-collected or managed-runtime reference used by layoutless
targets such as the JVM. Its representation and member-call behavior belong to
the selected backend. It is not a native address, does not use native pointer
arithmetic, and is not interchangeable with `->T` or `=>>T` through ordinary
pointer conversion.

## Procedure pointers

A pointer target may be a `PROCEDURE` type:

```quxlang
VAR callback CONST->PROCEDURE(@value I32: I32) := transform<-;
VAR result I32 := callback(@value 4);
```

Procedure signatures, calling conventions, and invocation are documented under
[Procedure Pointers](procedure-pointers-and-function-values.md).

## Lifetime and validity

Pointers are non-owning unless an owning abstraction gives them a separate
ownership contract. A pointer may remain representable after its target's
lifetime ends, but dereferencing it is invalid. `NEW`/`DELETE` and explicit
typed storage impose additional allocation and lifetime rules; pointer syntax
alone does not satisfy them.
