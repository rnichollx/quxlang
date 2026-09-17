# Reasonable Safety Mechanisms

Sometimes you need sharp knives and big guns to solve big problems. Where reasonable, we can put safety mechanisms on the guns and knives. Having safety mechanisms doesn't mean we can't use sharp knives and big guns, it means we may sometimes need to use a safety lever or pull the knife out of its sheath to use it; the language isn't going to protect you against every possible hazard, but we can put safeties up around the most hazardous bits.
 
In Quxlang, we guard certain constructs from C++ that are inherently unsafe and error-prone, with an _easy to remove_ safety mechanism.

## General Guiding Principle

The choice on whether to make a behavior safe-by-default or unsafe-by-default is mainly made based upon the performance impact. The general principle here is to measure the impact before making a decision.

Certain behaviors, like having variables uninitalized by default or having signed overflow be undefined behavoid, provides little to no improved performance in practice. Other behaviors, like checking all pointers for null before dereferencing, have severe performance impacts and are therefore not safe-by-default.

Quxlang adopts a safe-by-default policy where the performance impact is usually negligible and an unsafe-by-default policy where safe behavior would introduce significant performance penalties.

As Quxlang prioritzes program performance, safety mechanisms should have opt-out mechanisms.

## Casting

In Quxlang, implicit narrowing casts are not allowed:

```quxlang
VAR x I32 := my_i64; // Compilation error
```

We can however cast explicitly:

```quxlang
VAR x I32 := my_i64 AS PARTIAL I32; // OK: excess bits discarded
VAR y I32 := my_i64 AS CHECKED I32; // OK: guaranteed fault if out of range
VAR z I32 := my_i64 AS ASSUME I32;  // OK: behavior is undefined if out of range
```

This improves performance, because using PARTIAL or ASSUME allows signaling your expectation to the compiler. When using ASSUME, the compiler might be able to make additional optimizations by assuming the value is in-range.

## Variable Initialization

When we initialize variables, we get them default constructed with a sensible default value (usually 0).

```quxlang
VAR x I32; // has default value of '0'
```

In the rare case where variable initialization imposes an unacceptable performance penalty, `TYPED_STORAGE(T)` can be used instead:

`VAR x TYPED_STORAGE(I32);`

The `TYPED_STORAGE(t1, t2...)` keyword-type allows the creation of storage which is of adequte size and alignment for all of the types specified.
It is then the programmer's responsibility to ensure the value is created before being accessed and that the internal object is destroyed before deinitializing the storage.

In the rare case where initalization presents significant overhead, TYPED_STORAGE can be used instead.

## Overflow

Default fixed-width signed arithmetic has defined Quxlang semantics; it does not acquire
C or C++ signed-overflow undefined behavior:

```quxlang
VAR x I32 := a + b; // well defined to wrap around even if signed overflow occurs
```

Signed overflow being undefined creates a number of bugs and rarely improves performance. However, in the rare cases where it is needed for performance reasons, the assume variants can be used instead:

```quxlang
a +! b // The behavior is undefined if overflow would occur
```

In some cases overflow would indicate a program bug or invalid input, in these case, the checked variant can be used instead:

```quxlang
a +? b // Throws exception if overflow would occur
```

The checked variants are guaranteed to throw an exception if an overflow would occur. This can make code much easier to read:

```quxlang
result := checked_add64(a, checked_multiply64(b, c)); // without checked operators
result := a +? (b *? c);                              // with checked operators
```


These operator variants are available for both signed and unsigned variants. This means, for example, that you can even declare unsigned overflow to be undefined behavior using operator `+!`.

## Hardened builds

Quxlang will provide an option for Hardened builds which modify some of the behaviors. Ordinarily, assume operations like `+!` will check that their operands are in-range in Debug configuration and optimize away as undefined behavior in Release mode. Hardened compilation mode allows Quxlang to keep certain "hardness checks" even when compiling in a non-debug format.

