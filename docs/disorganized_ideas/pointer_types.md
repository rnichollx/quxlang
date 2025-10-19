# Pointer Types

## Instance Pointers

### Type

`-> T`

Example:

`-> I32`

### Description

Represents a pointer to an instance of type `T`.

### Operations

Dereference `->`: example: `ptr->` to get the value pointed-by an instance pointer.
Instance address `<-`: example: `obj<-` to get an instance pointer to an object.

### Restrictions

Pointer arithmetic between instance pointers is not allowed.

## Array Pointers

`[->] T`

### Type

`[->] T`

Example:

`[->] I32`

### Description

Represents a pointer to a position in an array of `T`.

### Operations

Dereference `[index]`: example: `ptr[index]` to get the value at the index in the array.

Addition/subtraction of indices is allowed. e.g. `ptr + 5` to get an array pointer advanced by 5 indices.

### Restrictions
Pointer arithmetic between array pointers is allowed, but only within the same array. e.g. `ptr1 + (ptr2 - ptr1)` is valid if `ptr1` and `ptr2` point within the same array.

Offset conversions and other pointer arithmetic is not allowed.

## Wildcard Pointers

### Type

`*T`

Example:

`*I32`

### Description

Wildcard pointers are the most liberal pointer. They can be used in aribtrary pointer arithmetic and perform offsetof conversions.
