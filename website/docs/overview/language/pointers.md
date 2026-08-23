# Overview of Pointers

Quxlang has separate pointer types for one object and for a position in an
element sequence.

- An **instance pointer**, `->T`, points to one object.
- An **array pointer**, `=>>T`, points to a position in a sequence of objects.

`MUT` and `CONST` qualify access, as in `MUT->I32` and `CONST=>>BYTE`.

## Taking an address and dereferencing

Postfix `<-` takes an object's address. Postfix `->` dereferences an instance
pointer or array pointer:

```quxlang
VAR value I32 := 7;
VAR pointer ->I32 := value<-;

pointer-> := 9;
ASSERT(value == 9);
```

Pointers can be null. Test them before dereferencing:

```quxlang
VAR optional ->I32 := NULL;
ASSERT(optional?!);

optional := value<-;
IF (optional??)
{
  use(optional->);
}
```

## Array pointers

`array[& index]` produces an array pointer. Arithmetic advances in elements:

```quxlang
VAR values [4]I32 :[10, 20, 30, 40];
VAR begin MUT=>>I32 := values[& 0];

ASSERT(begin-> == 10);
ASSERT((begin + (2 AS SZ))-> == 30);
```

Instance pointers do not support sequence arithmetic.

## Reference

See the [Pointers Reference](../../reference/pointers.md) for pointer
qualification, nullability, indexing and arithmetic, `VOID` reinterpretation,
procedure pointers, lifetime, and validity constraints.[^gc-pointer]

[^gc-pointer]: A **GC pointer**, `~>T`, is the pointer type used for
    garbage-collected objects on supported managed-runtime targets. It is not
    interchangeable with an instance pointer or array pointer.
