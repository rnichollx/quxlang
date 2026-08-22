# Arrays

A fixed array is one object containing a compile-time number of elements of one
element type. The count is part of the array's type.

## Array types

Write the element count before the element type:

```quxlang
VAR values [4]I32;
VAR matrix [3][2]I32;
VAR empty [0]BYTE;
```

`[4]I32` and `[5]I32` are different types. Nested arrays associate through the
written element type: `[3][2]I32` contains three elements, each of type
`[2]I32`.

The count must resolve to a nonnegative compile-time integer. A zero-length
array is valid and contains no element objects.

## Default construction

Omitting an initializer default-constructs every element in increasing index
order:

```quxlang
VAR numbers [3]I32;
ASSERT(numbers[0] == 0);
ASSERT(numbers[1] == 0);
ASSERT(numbers[2] == 0);
```

Default construction is available only when the element type has an applicable
no-argument constructor. A zero-length array performs no element construction.

## Positional initialization

`:[...]` supplies one initializer for each element:

```quxlang
VAR initialized [4]I32 :[2, 4, 6, 8];
VAR empty [0]BYTE :[];
```

The number of entries must exactly equal the array count. Each entry initializes
the corresponding element using that element type's constructor selection.
Too few or too many entries are compilation errors.

Positional construction works with user-defined element types:

```quxlang
VAR points [2]coordinate :[
  coordinate(@x 1, @y 2),
  coordinate(@x 3, @y 4)
];
```

The array expression form invokes the same constructor surface:

```quxlang
VAR values [3]I32 := [3]I32(% [10, 20, 30]);
```

See [Variables](variables.md) for the distinction among no-argument, `:=`,
`:(...)`, and `:[...]` initialization.

## Copy and move construction

Copy construction copies corresponding elements. Move construction moves
corresponding elements from a temporary-qualified source:

```quxlang
VAR original [3]I32 :[1, 2, 3];
VAR copied [3]I32 := original;
```

The operation is available when the element type supports the required
constructor.

## Indexing

`array[index]` selects an element and preserves access qualification:

```quxlang
initialized[1] := 9;
ASSERT(initialized[1] == 9);
```

A mutable array provides mutable element access; a constant array does not.
The index must use a supported integer form. Access is valid only for indices
from zero through `N - 1`; a zero-length array has no valid element index.

`array[& index]` produces a pointer to an element instead of a reference-like
element access:

```quxlang
VAR first MUT->I32 := initialized[& 0];
```

The pointer remains tied to the array object's lifetime and valid bounds.
Arrays do not require an implicit decay to a pointer; use `[& index]`,
`BEGIN()`, or another explicit pointer-producing operation.

## Iteration

`BEGIN()` returns an array pointer to the first element and `END()` returns the
one-past endpoint:

```quxlang
VAR cursor MUT=>>I32 := initialized.BEGIN();
VAR end MUT=>>I32 := initialized.END();

WHILE (cursor != end)
{
  consume(@value cursor->);
  cursor++;
}
```

For `[N]T`, advancing `BEGIN()` by `N` elements reaches `END()`. For `[0]T`,
`BEGIN()` equals `END()` and neither may be dereferenced.

Array iteration also integrates with [`FOR`](for-loops.md):

```quxlang
FOR ITEM(item) IN(initialized) LOOP
{
  item++;
};
```

`ITEM` binds writable element access for a mutable array; `VALUE` creates the
loop's projected value according to the iterator projection rules.

## Assignment, swap, and comparison

Array assignment applies assignment to corresponding elements. `<->` swaps
corresponding elements. These operations require equal array types and the
corresponding element operation:

```quxlang
VAR left [2]I32 :[1, 2];
VAR right [2]I32 :[7, 8];
left <-> right;
```

Equality compares corresponding elements. Three-way and relative comparison
use the array's generated comparison path when the element type supplies the
required comparisons. Array length does not need runtime comparison because it
is already fixed by the common type.

## Destruction and lifetime

An array owns all of its elements. Destroying the array destroys every live
nontrivial element in reverse construction order. References and pointers to
elements do not extend the array lifetime.

Raw storage for an array is not an array object until `PLACE` or another
constructor begins its lifetime. See [Object Storage and Lifetime](typed-storage-and-lifetime.md)
for explicit lifetime operations and [Pointers](pointers.md) for array-pointer
arithmetic and validity rules.
