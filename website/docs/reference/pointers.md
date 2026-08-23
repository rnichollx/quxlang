# Pointers

Quxlang distinguishes instance pointers and array pointers. These pointer types
have different arithmetic and representation contracts.

| Form | Pointer type |
| --- | --- |
| `MUT->T`, `CONST->T` | Instance pointer |
| `MUT=>>T`, `CONST=>>T` | Array pointer |

Omitting `MUT` gives the mutable short forms `->T` and `=>>T`. `&T` is a
reference, not a pointer. Quxlang also has a backend-specific GC pointer
type.[^gc-pointer]

## Instance pointers

Postfix `<-` obtains a pointer from an object reference. Postfix `->`
dereferences an instance pointer or array pointer:

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

`=>>T` denotes an array pointer: a position in a multi-object allocation or
array. Indexing an array with `[& index]` produces one:

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

Instance pointers and array pointers may be explicitly reinterpreted through
`->VOID` or `=>>VOID` when an untyped boundary requires it:

```quxlang
VAR typed ->I32 := value<-;
VAR erased ->VOID := typed AS REINTERPRET ->VOID;
VAR restored ->I32 := erased AS REINTERPRET ->I32;
```

A `VOID` pointer cannot be dereferenced because it has no element type. Array
arithmetic also requires a non-`VOID` element size. `REINTERPRET` does not
create object lifetime; the restored type must describe an object that is
actually valid at that address. See [Conversions](conversions.md).

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

[^gc-pointer]: A **GC pointer**, `~>T`, refers to a garbage-collected object on
    a supported managed-runtime target. Its representation and member-call
    behavior belong to the backend. GC pointers do not use array pointer
    arithmetic and are not interchangeable with instance pointers or array
    pointers through ordinary pointer conversion. The current JVM path permits
    GC pointers to `EXTERN_TYPE` declarations.
