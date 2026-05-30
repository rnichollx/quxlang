# Value Semantics

Value and data semantics are the core programming model in Quxlang. Various defaults in the language assume things like strong ordering and canonical representations for datatypes. We recognize that the C++ value model is superior to the reference model adopted by languages like Java and Python.

##  Floating Point

One of the core tenents of Quxlang is that _data types_ should be _strongly ordered_. This is true even for data types which are traditionally not strongly ordered, such as floating point numbers.

This means that in Quxlang `NAN == NAN` and there is only 1 NaN value. Traditional floating point comparison functions are still available, but they are moved into keyword functions instead of operators, such as `IEEE_FLOAT_LESS(a, b)`.

The decision to use strong ordering for floating point operators like `<`, `<=` etc. is based on the recognition that requiring special cases for handling ordering of floating point types limits composability, which is an undesirable consequence. Namely, the additional complexity when performing generic computation on data for purposes like caching, lookups, and serialization was considered more harmful than the extra issues with NaN values that could arise when using strong ordering, particularly given such issues are constrained to numeric domain exclusive code. This is particularly the case given that floating point has an easy opt-out through IEEE comparison functions in the few places this matters, but the alternative would require all generic ordering code to special case floating point numbers, which was considered unacceptable.


## Structured Comparison

By default, a struct or class which contains only datatype members is itself a datatype, represented as a tuple of values.

For example, given: 

```quxlang
::foo CLASS
{
  .a VAR I32;
  .b VAR baz;
}
```
 

Then the expression `my_foo1 < my_foo2` is valid if `baz` is itself a comparable datatype.

Formally this iterates the members, and the result of inequality is the first unequal result.

