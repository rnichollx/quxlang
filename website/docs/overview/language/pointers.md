# Overview of Pointers

Quxlang has separate pointer categories for one object, an element sequence,
and managed-runtime objects.

- `->T` points to one native `T` object.
- `=>>T` points into a native sequence of `T` objects.
- `~>T` is a managed-runtime reference.

`MUT` and `CONST` qualify access, as in `MUT->I32` and `CONST=>>BYTE`.

## Taking an address and dereferencing

Postfix `<-` takes an object's address. Postfix `->` dereferences a native
pointer:

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

Instance pointers do not support sequence arithmetic. Managed `~>` references
are backend-managed handles rather than native addresses.

## Reference

See the [Pointers Reference](../../reference/pointers.md) for pointer
qualification, nullability, indexing and arithmetic, `VOID` reinterpretation,
managed references, procedure pointers, lifetime, and validity constraints.
