# Quxlang Atomic Optimizations


The Quxlang compiler is to fuse sequences of atomic operations under an as-if rule if the atomics are defined on
ATOMIC(T) types. It cannot do so for atomic-volatile operations on external memory.

Atomic fusion is an allowed optimization that allows multiple subsequent atomic operations to have their side effects
combined under the as-if rule. Essentially, if a legal scheduling of operations that respects the atomic semantics can
be found that produces the same observable behavior, then the optimization is valid.

For example, given `VAR a ATOMIC(I32)` then the following code:

```quxlang
a.fetch_add(4);
a.fetch_add(4);
```

May be optimized to:

```quxlang
a.fetch_add(8);
```

There is only one constraint on the optimization, which is that modifications made by other threads become visible
"eventually" under some definition of eventually. This means that values checked in possibly infinite loops must be
re-checked at some point, even when performing unrolling. This optimization is also not a license to violate the
memory ordering model.

Example transformation where this optimization is valid:

```quxlang
VAR i I64 := 0;

VAR counters = =>ATOMIC(I32);

WHILE (stop.load(RELAXED) == 0 && i < n) {
    counters[i++].fetch_add(1, RELEASE);
}
```

To:

```quxlang
VAR i I64 := 0;

VAR counters = =>ATOMIC(I32);

WHILE (stop.load(RELAXED) == 0 && i+8 < n) {
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
    counters[i++].fetch_add(1, RELEASE);
}
WHILE (stop.load(RELAXED) == 0 && i < n) {
    counters[i++].fetch_add(1, RELEASE);
}
```

This is allowed because the fetch_add operation on counters are performed in release ordering, which does not guarantee
that a load from the `stop` variable will be visible, which is in relaxed ordering. On the other hand, if the
fetch_add operation occured in ACQUIRE_RELEASE ordering, then this optimization would not be valid.

