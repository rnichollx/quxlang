# References


In Quxlang, a reference must be attached to valid storage at the entry point of every function
and upon creation.

Therefore, unlike a pointer, a compiler may speculatively load data from a reference without violating the as-if rule
in many situations.

For example, given:

```
FUNCTION(@a &BOOL, @b &BOOL, @c I32) : I32
{
  IF (a!!)
  {
    RETURN 0;
  }
  IF (b!!) {
    RETURN 0;
  }
  ELSE {
    RETURN 0;
  }
}
```

The compiler may assume that `@a` and `@b` are valid references, it does not need to check that a is valid before dereferencing
`b`, because the reference must point to valid storage throughout the function due to lack of sychronization points.

Therefore the compiler can optimize the above to a single branch:

```quxlang
FUNCTION(@a &BOOL, @b &BOOL, @c I32) : I32
{
  IF (a #&& b)
  {
    RETURN c;
  }
  RETURN 0;
}
```

The compiler cannot always assume that `@b` is valid after a synchronization point, such as a mutex lock/unlock or a function
call which the compiler doesn't have visibility into. However, because the reference must be valid upon entry to the function,
the compiler may assume that `@b` is valid given that no synchronization points or unknown side effects occur.