# Overview of Arrays

An array type places the element count before the element type:

```quxlang
VAR values [4]I32;
VAR initialized [4]I32 :[2, 4, 6, 8];
VAR empty [0]BYTE :[];

initialized[1] := 9;
ASSERT(initialized[1] == 9);
```

`[N]T` is one array object containing `N` elements of `T`. A fixed array may be
default-constructed or initialized positionally.

## Initialization forms

```quxlang
VAR default_value widget;
VAR copied_value widget := source;
VAR named_value widget :(@width 20, @height 10);
VAR positional_value widget :[20, 10];

VAR expression_value widget := widget(@width 20, @height 10);
VAR expression_array [3]I32 := [3]I32(% [10, 20, 30]);
```

- No initializer invokes the default constructor.
- `:= expression` initializes from one expression.
- `:(...)` accepts ordinary named arguments and positional groups.
- `:[...]` is positional-only constructor shorthand.
- `:[]` is an empty positional constructor call, not an omitted initializer.

The forms apply to user-defined types as well as arrays. The declared type's
constructor set decides which initialization is valid.

Arrays expose `BEGIN()` and `END()` and support `array[index]` plus
`array[& index]`; see [Pointers](pointers.md).

## Reference

See the [Arrays Reference](../reference/arrays.md) for the complete
language rules, constraints, and technical edge cases.
