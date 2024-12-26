# For Loop

## Syntax

```quxlang
FOR ... LOOP {
   ...
};
```

## Description

### Specifiers
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
 * `FILTER(x)` skips iterations where `x` is false, without ending the loop.
 * `ADVANCE(x)` the number to add to the iterator during a step. Exclusive with `STEP`.

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

### Loop Blocks

 * `LOOP { ... }` the body of the loop.
 * `STEP { ... }` sets the step of the loop. The default step is `<iter>++`.
 * `FIRST` like `LOOP`, but only for the first iteration.
 * `LAST` like `LOOP`, but only for the last iteration.
 * `BETWEEN` like `LOOP`, but only for iterations which are not `FIRST` or `LAST`.
 * `SEQFIRST` like `FIRST`, but doesn't run if the `FIRST` is also the `LAST`.
 * `SEQLAST` like `LAST`, but doesn't run if the `LAST` is also the `FIRST`.
 * `SINGLE` only runs if the iteration is both `FIRST` and `LAST`.
 * `EMPTY` only runs if the loop is empty.
 * `DO` Similar to `STEP`, but runs the body before each iteration check.

This syntax differs a bit from C++, where the x:y syntax iterates equivalent to `FOR ITEM(x) IN(y)`.
There are some differences, for example, `INDEX(x)` on a `vector` would give the index of the vector,

```cpp
for (std::size_t i = 0; i < vec.size(); i++) {
  ...
}
```

```quxlang
FOR INDEX(i) IN(vec) LOOP {
  ...
};
```



Instead of using `IN` for selecting the iterator, you can use `START` and `END` to specify the range.

```quxlang
FOR START(iter1) END(iter2) VALUE(x) LOOP {

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

Instead of `END` you can also use `UNTIL` or `WHILE` to specify the end condition,

```quxlang
FOR ITER(it) START(iter1) UNTIL(it > iter2) VALUE(x) LOOP {
   ...
};
```
