# Call Arguments

Quxlang accepts named and positional arguments.

## Named arguments

A named argument uses `@name expression`:

```quxlang
VAR result I32 := ceil_div(@numerator 9, @denominator 2);
```

Where `name` here is the argument interface name or the API-name of the argument. In Quxlang, named arguments do 
not need to passed in declaration order, so the following is also allowed with equivalent behavior:

 ```quxlang
VAR result I32 := ceil_div(@denominator 2, @numerator 9);
```

Functions can also declare a local name using `@api_name:local_name`, separate from the API-name for brevity:

```
::ceil_div FUNCTION(@numerator:n I32, @denominator:d I32) : I32
{
  RETURN (n + d - 1) / d;
}
```

The use of named arguments prevents confusion regarding the order of arguments and reduces programming errors.
Named arguments are a compile time feature and do not reduce the runtime performance of the program compared to traditional
positional arguments.

## Single argument functions

For functions which take only one argument, it is not nessecary to specify the name of the argument. Such functions can declare
an argument named `@ARG`. Here, the keyword-identifier `ARG` is used in place of a normal argument name.

Example:

```quxlang
::twice FUNCTION(@ARG I32): I32
{
  RETURN ARG * 2;
}
```

Can be called with:

```Quxlang
VAR result I32 := twice(x);
```

## Positional Argument

Quxlang also supports positional arguments, though they are discouraged for most uses.

A positional argument is declared using `%local_name` instead of `@`.
To call a function using positional arguments, `%` must be used

The use of positional arguments is recommended only for operations which process sequences of elements.

For example:

```quxlang
sum_all(% [a, c, 9]);
```

## Interleaving and evaluation order

Named arguments and positional groups may be interleaved when an API needs
both. Expressions are evaluated in source order:

```quxlang
VAR combined I32 := combine_interleaved_arguments(
  % [first_expression()],
  @named second_expression(),
  % [third_expression(), fourth_expression()]
);
```

Constructor argument forms are covered on [Arrays and construction](arrays-and-construction.md)
and [Constructors and destructors](constructors-and-destructors.md).
