# Operators

## Arithmetic Operators

* `+` Adds two values.
* `-` Subtracts two values.
* `*` Multiplies two values.
* `/` Divides two values.
* `%` Modulus operator.

## Comparison Operators

* `==` Checks if two values are equal.
* `!=` Checks if two values are not equal.
* `<` Checks if the left value is less than the right value.
* `>` Checks if the left value is greater than the right value.
* `<=` Checks if the left value is less than or equal to the right value.
* `>=` Checks if the left value is greater than or equal to the right value.

## Assignment Operators

* `:=` Copy-assigns a value.
* `:<` Move-assigns a value.

## Logical Operators

* `&&` Logical and.
* `||` Logical or.
* `!&` Logical not-and.
* `!|` Logical not-or.
* `^^` Logical exclusive or.
* `^->` Logical A implies B,
* `^<-` Logical B implies A.
* `!!` Logical negation (suffix)

## Bitwise operators

* `.&&` bitwise and.
* `.||` bitwise or.
* `.!&` bitwise not-and.
* `.!|` bitwise not-or.
* `.^^` bitwise exclusive or.
* `.<<` bitwise left shift.
* `.>>` bitwise right shift
* `.<|` bitwise left rotate
* `.|>` bitwise right rotate
* `.+>` bitwise arithmetic shift
* `.?<` bitwise first set index
* `.?>` bitwise last set index
* `.!<` bitwise first unset index
* `.!>` bitwise last unset index

## Object and pointer operators

* `.` Field get operator
* `->` Pointer access operator
* `<-` Get address-of operator
* `?-` Get field address if non-null operator
* `?>` Get value-or-default operator
* `??` Booliate operator
* `?!` Anti-booliate operator (same as `?? !!`)

  For example, `5 ??` is `true` whereas `0 ??` is `false`.
  Integers convert to `true` if they are non-zero and `false` if they are zero.
  Pointers convert to `true` if they are non-null and `false` if they are null.
  e.g. `IF (ptr??) { ... }` checks if `ptr` is non-null.
  Note that in Quxlang integers and pointers cannot be used in an IF statement directly without an explicit conversion.
  