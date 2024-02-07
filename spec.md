## Function
### Function Return

If the execution of a function reaches the end of the function body, the function implicitly returns a default constructed object of the return type, or no object if the return type is void. However; if the return type is not default constructible, then the function triggers a fallthrough fault (which by default, terminates the program by calling RUNTIME::terminate()). However, if the function is declared NOFALLTHROUGH, then executing past the end of the body of the function without an explicit `RETURN` will always trigger a fallthrough fault, even if the return type is void or default constructible.


## Binary Operators

### Operator `+`

Adds two numbers.

### Operator `-`

Subtract two numbers.

### Operator `*`

Multiply two numbers.

### Operator `/`

Divine LHS by RHS.

## Prefix operators

### Operator `.~`

Bitwise inversion.

### Operator `.&&`

Bitwise and.

### Operator `.||`

Bitwise or.

## Operator Sythesis

Operators may be synthesized. 

If an operator, such as `==` is used in an expression, then the system looks for a function that provides that operator,
 such as lhs::.OPERATOR==(CONST &rhs) or rhs::.OPERATOR==(CONST & lhs)RHS. However, if no operator is found, it will
 attempt to synthesize operators according to the following rules:
 
# Operator `==` and `!=`:

First, these operators attempt to synthesize themselves using the logical inversion of the alternative operator. E.g.,

`a != b`

Can be synthesized as:

`!a.OPERATOR==(b)`

If this fails, it will attempt to synthsize it using `<`, `>`, `<=` or `>=`, in that other. The first operator that is
 successfully found is used.
 
## Operator `<`

`a < b`

First it attempts with the opposite operator of reverse ordering, e.g.

`a < b`

is synthesized as:

`b.OPERATOR>(a)`

If this fails, it will look for `<=` or `>=`. The order of preference is to first look for the opposite corresponding 
operator. So `a < b` will prefer to synthsize as `!(a >= b)`. The WEAK_ORDERING or PARTIAL_ORDERING keywords will
prevent this synthesis, if present in either class. With differing types, it is possible for a synthesis like:

`a < b` : `a <= b && !(a >= b)`

But these are not well defined in terms of ordering at the moment.
