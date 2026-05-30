# Reasonable Safety Mechanisms

 Sometimes you need sharp knives and big guns to solve big problems. Where reasonable, we can put safety mechanisms on the guns and knives. Having safety mechanisms doesn't mean we can't use shrap knives and big guns, it means we may sometimes need to use a safety lever or pull the knife out of it's sheath to use it; the language isn't going to protect you against every possible hazard, but we can put safeties up around the most hazardous bits.
 
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

In Quxlang, signed arithmetic is not undefined unless you request it:

```quxlang
VAR x I32 := a + b; // well defined even if signed overflow occurs
```

But we _can_ specify that signed overflow is illegal and should be optimized as undefined behavior using `!`:

```quxlang
VAR x I32 := a +! b; // If overflow occurs, the program behavior is undefined
```

Likewise, in situations where overflow would indicate a bug we want to detect, we can also _check_ for overflow:


```quxlang
VAR x I32 := a +? b; // Overflow is checked at runtime, fault if overflow occurs.
```

These operators are also available for unsigned integers as well. Meaning you can obtain similar no-wrap optimizations by using `!` suffix operators on unsigned arithmetic operations.

Note: In debug builds, undefined operations like `+!` overflow may fault instead.

Note: Current faults are implemented with traps. Do not assume faults will trap, in particular, a fault might throw an exception in future versions of Quxlang instead of trapping; please write exception-safe code accordingly where possible.