# Reasonable Safety Mechanisms

Sometimes you need sharp knives and big guns to solve big problems. Where reasonable, we can put safety mechanisms on the guns and knives. Having safety mechanisms doesn't mean we can't use sharp knives and big guns, it means we may sometimes need to use a safety lever or pull the knife out of its sheath to use it; the language isn't going to protect you against every possible hazard, but we can put safeties up around the most hazardous bits.
 
In Quxlang, we guard certain constructs from C++ that are inherently unsafe and error-prone, with an _easy to remove_ safety mechanism.

## Casting

In Quxlang, implicit narrowing casts are not allowed:

```quxlang
VAR x I32 := my_i64; // Compilation error
```

We can however cast explicitly:

```quxlang
VAR x I32 := my_i64 AS PARTIAL I32; // OK: excess bits discarded
VAR y I32 := my_i64 AS CHECKED I32; // OK: guaranteed fault if out of range
VAR z I32 := my_i64 AS ASSUME I32; // OK: behavior is undefined if out of range
```

## Variables

When we initialize variables, we get them default constructed with a sensible value.

```quxlang
VAR x I32; // has default value of '0'
```

If we need extra performance in a particular case, we can remove the safety sheath from our knife easily:

```quxlang
VAR x I32 := UNSPECIFIED;
```

## Overflow

Fixed-width signed arithmetic has defined Quxlang semantics; it does not acquire
C or C++ signed-overflow undefined behavior:

```quxlang
VAR x I32 := a + b; // well defined even if signed overflow occurs
```

The current source grammar does not provide arithmetic suffixes for requesting
undefined-on-overflow or checked-overflow behavior. Where overflow is an error,
check operands against the type's bounds before the operation, or use an
appropriate `AS CHECKED` conversion at a narrowing boundary.

Note: Current faults are implemented with traps. Do not assume faults will trap, in particular, a fault might throw an exception in future versions of Quxlang instead of trapping; please write exception-safe code accordingly where possible.
