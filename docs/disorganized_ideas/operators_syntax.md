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
* `&!` Logical and-inverse (nand).
* `|!` Logical or-inverse (nor).
* `^^` Logical exclusive or.
* `^->` Logical A implies B,
* `^<-` Logical B implies A.
* `!!` Logical inverse (suffix)

## Bitwise operators

### Binary bitwise operators

* `#&&` bitwise and.
* `#||` bitwise or.
* `#&!` bitwise and-inverse (nand).
* `#|!` bitwise or-inverse (nor).
* `#^->` bitwise a implies b.
* `#^<-` bitwise b implies a.
* `#^^` bitwise exclusive or.
* `#^!` bitwise not exclusive-or.
* `#++` bitwise up-shift ("left shift").
* `#--` bitwise down-shift ("right shift").
* `#+%` bitwise up-rotate
* `#-%` bitwise down-rotate

### Unary bitwise operators

* `#!!` bitwise inverse/complement (suffix)

## Object and pointer operators

* `.` Field get operator.
* `->` Pointer access operator.
* `<-` Get instance-address operator.
* `[&]` Get array-address operator.
* `*<-` Get wildcard-pointer operator.
* `?-` Get field address if non-null operator

```
::foo CLASS
{
  .b VAR bar;
}

::func FUNCTION(@f_ptr ->foo): ->bar
{
  RETURN f ?-b;
}
```

* `?>` Get value-or-default operator
* `?=` Get value-or evalulate alternative expression operator


* `??` Booliate operator
* `?!` Anti-booliate operator (same as `?? !!`)

  For example, `5 ??` is `true` whereas `0 ??` is `false`.
  Integers convert to `true` if they are non-zero and `false` if they are zero.
  Pointers convert to `true` if they are non-null and `false` if they are null.
  e.g. `IF (ptr??) { ... }` checks if `ptr` is non-null.
  Note that in Quxlang integers and pointers cannot be used in an IF statement directly without an explicit conversion.
  