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

## Logical Operators

### Operator `&&`

Logical and.

### Operator `||`

Logical or.

### Operator `^^`

Logical xor.

### Operator `!^`

Logical nxor.

### Operator `!&`

Logical nand.

### Operator `!|`

Logical nor.

### Operator `?>`

Logical implication.

### Operator `<?`

Logical reverse implication.

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


## Tail Calls

A tail call can be performed using `TAILRET` instead of `RETURN`. This triggers a compiler error if it cannot
be made into a tail call. The compiler might also optimize normal calls into tail calls, but this is not guaranteed.

In order for a call to be converted into a tail call, all the arguments must satisfy `TrviallyRelocatable`. The 
following types will be trivially relocatable:

* Built-in primitives, like I32, F32, etc.
* Pointers, references.
* Classes declared `TRIVIALLY_RELOCATABLE` or `PRIMITIVE` (which implies `TRIVIALLY_RELOCATABLE`).
* Classes declared `POSTHOC_RELOCATABLE`.

## Call Relocation

Types which are trivially relocatable can be passed in function calls more efficiently in many ABI targets. 
Trivial relocation means that a value can be passed into a register or set of registers and then stored in a different
memory location. In this case, the address of the object when the constructor is called and when the destructor is called
may not match. 

For example, supposing you have a function `::foo FUNCTION(%a myclass)`, and you pass an instance of `myclass` as 
an argument by variable name, `foo(myclassobj)`, in this case, a copy of the object is made and passed to the function.

It is possible to *pass* objects directly into a function call, via *pass*, e.g., `foo(PASS(myclassobj))`. In this case,
myclassobj is passed directly to the function and not copied, therefore it is no longer valid after the function call.

However, merely using `PASS` by itself does not remove the pointer indirection, because the callee needs to call the
object's destructor. In general, we assume that objects need to reside in the same memory location at the constructor
and destructor unless the object is `RELOCATABLE`. There are 3 categories of relocatable objects:

* `TRIVIALLY_RELOCATABLE`: Can be relocated using a bit copy.
* `POSTHOC_RELOCATABLE`: Can be relocated using a bit copy followed by `.POSTHOC_RELOCATE`
* `RELOCATABLE`: Can be relocated using `.RELOCATE`.

Of these, `TRIVIALLY_RELOCATABLE` and `POSTHOC_RELOCATABLE` allow bypassing pointer indirection in function call ABIs.


