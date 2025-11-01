# Runtime

Example runtime statement:

```quxlang
RUNTIME CONSTEXPR {
  // implementation using constexpr-safe functions only 
} ELSE {
  // fast implementation that runs the calculations on a GPU
}
```

```quxlang
RUNTIME NATIVE {
  // implementation using constexpr-safe functions only 
} ELSE {
  // fast implementation that runs the calculations on a GPU
}
```



## Runtime options

* `CONSTEXPR` Path taken when the compiler is in a constexpr execution context.
* `NATIVE` Path taken when the compiler is in a native execution context.

# For Loop

## Syntax

```quxlang
FOR ... LOOP {
   ...
};
```

## Description

### General For-loop properties

  * `INIT ( ... )` initialization block, run once before the loop.
  * `EVAL { ... }` similar to `INIT`, but allows multiple statements to execute.
  * `TEST(x)`: before each `LOOP` iteration, the condition `x` is checked; if false, the loop exits.
  * `STEP { ... }` block executed after each iteration of the loop. This executes after the `LOOP` body and any 
    `TEST` condition.
  * `POSTTEST(x)`: similar to `TEST`, but the condition is checked after each iteration of the loop body.

Each round of iteration looks like either:

1. Check `TEST` condition; if false, exit loop.
2. Execute `LOOP` block.
3. Execute `POSTTEST` condition; if false, exit loop.
4. Execute `STEP` block if present.
5. Return to step 1.



A loop of the form `WHILE(x) { ... }` is equivalent to `FOR PRETEST(x) LOOP { ... };`. Quxlang does not have a
dedicated "do-while" loop, but the equilvaent is availble using a `POSTTEST` condition in a `FOR` loop e.g.:
 `FOR POSTTEST(x) LOOP { ... };`.
  

Example:

```quxlang
FOR INIT{ VAR i := 0; } TEST(i < 10) STEP{ i++; } LOOP {
   ...
};
```

### Iteration For-Loops
#### Specifiers
 * `ITER(x)` the iterator in a range
 * `VALUE(x)` the value of the iteratation
 * `INDEX(x)` the index of the iteration where applicable,
 * `ITEM(x)` the item of the iteration where applicable, 
   equivalent to C++ `*iter`.
 * `IN(z)` sets `START` and `END` using `z`.
 * `START(x)` sets the start iterator of the loop.
 * `END(x)` sets the end iterator of the loop.
 * `LIMIT(x)` like `END`, but uses `<` instead of `!=`. 
 * `WHILE(x)` sets the end condition of the loop. Exclusive with `END`.
 * `UNTIL(x)` similar to `WHILE`, but with two differences:
   * the condition is negated, i.e. the loop continues while `x` is false.
   * the condition is only checked after each iteration, so the loop always runs at least once.
 * `FILTER(x)` skips iterations where `x` is false, without ending the loop.
 * `BY(x)` the number to add to the iterator during a step. Exclusive with `STEP`.

In C++, some containers store their own indexes, e.g. std::map.

From C++ Examples:

   * `std::map<int, std::string>`: 
     * `INDEX = int`
     * `ITEM = std::pair<int, std::string>`
     * `VALUE = std::string`
   * `std::vector<int>`:
     * `INDEX = std::size_t`
     * `ITEM = int`
     * `VALUE = int`
   * `std::list<int>`:
     * `ITEM = int`
     * `VALUE = int`
   * `std::set<int>`:
     * `ITEM = int`
     * `VALUE = int`

Loops have several allowed iteration methods,

```
FOR ITEM(x) IN(iterable) LOOP {
   ...
}
```

This loop iterates over each item which satisfies the "iterable" interface. This could be done with a range (having
`BEGIN()` and `END()` methods which produce iterators.)

```
FOR INDEX(i) IN(container) LOOP {

}
```

This loop is essentially equivalent to:
```
FOR ITEM(x) IN(container.INDEXES()) LOOP {

}
```

Where `container.INDEXES()` produces an iterable of indexes for the container.

Another form is the `VALUE(x)` iterator,

```
FOR VALUE(v) IN(container) LOOP {

}
```

This provides a loop over `container.VALUES()`, which produces an iterable of values for the container.

If both INDEX and VALUE are specified, `container.IV_PAIRS()` is used to produce an iterable, of which the iterated 
values must have `.INDEX()` and `.VALUE()` methods, which return the values for the loop respectively.

It is a compilation error to specify both `ITEM` and also either `INDEX` or `VALUE` simultaneously.

In each of these cases, `ITER` can be specified to get access to the iterator during the loop.

In each of these cases, the _default_ behavior is currently that `IN(c)` is equivalent to `START(c.BEGIN()) END(c.END())`.

Thus for example `FOR VALUE(v) IN(container) LOOP { ... }` is equivalent to:

```quxlang
FOR ITEM(v) START(container.VALUES().BEGIN()) END(container.VALUES().END()) LOOP {
   ...
}
```

However, if the `BY` or `STEP` clause is used, then `LIMIT` is used instead of `END`.
 
By default, the loop step is equivalent to `++__iter`, but this behavior can be modified with `BY` or `STEP`.

if `BY(n)` is specified, the loop step is equivalent to `__iter += n` instead. If `OPERATOR+=` is not defined for the
iterator type, it is a compilation error.


### Sequence For-Loops

Sequence for loops are similar

### Additional Loop Blocks

 * `FIRST` like `LOOP`, but only for the first iteration.
 * `LAST` like `LOOP`, but only for the last iteration.
 * `BETWEEN` like `LOOP`, but only for iterations which are not `FIRST` or `LAST`.
 * `SEQFIRST` like `FIRST`, but doesn't run if the `FIRST` is also the `LAST`.
 * `SEQLAST` like `LAST`, but doesn't run if the `LAST` is also the `FIRST`.
 * `SINGLE` only runs if the iteration is both `FIRST` and `LAST`.
 * `EMPTY` only runs if the loop is empty.
 * `DO` Similar to `STEP`, but runs the body before each iteration check.



Instead of using `IN` for selecting the iterator, you can use `START` and `END` to specify the range.

```quxlang
FOR START(iter1) END(iter2) ITEM(x) LOOP {

};
```

You can also use `STEP` to modify the iterator,

```quxlang
FOR START(iter1) END(iter2) VALUE(x) ITER(it) STEP {
  it += 2;
} LOOP {
  ...
};
```

Instead of `END` you can also use `TEST` to specify the end condition,

```quxlang
FOR ITER(it) START(iter1) TEST(it > iter2) ITEM(x) LOOP {
   ...
};
```
