## General

### Compiler Determinism

#### Where determinism is required.

If the compiler produces a compiler-output or compiler-error, the output must be deterministic. If a compiler-exception
occurs, the exception may be non-deterministic. A compiler error, includes, for example, syntax errors and semantic
errors. A compiler exception includes, for example, "out of memory" or "stack overflow" errors, or other
compiler-platform-specific errors. If a maximum template depth is set in any compiler flags, this must trigger a
compiler exception and not a compiler error. If the compiler aborts compilation due to a compiler error encountered
during compilation of a _different_ output, this must be treated as a _compiler exception_ and not a compiler error.

#### Compiler Flags

A compiler flag MUST NOT affect the output of the compiler if it produces a compiler-output or compiler-error. A
compiler flag may affect, for example, the compiler's memory usage, the maximum template depth, the number of threads
the compiler uses, temporary directories, etc. However, it MUST NOT affect the output of the compiler's generated code
or the compiler errors. Changes to e.g. optimization levels MUST be reflected in the Quxbuild file and NOT the compiler
flags. If a compiler flag causes the compiler to fail to compile a program that would not fail with another set of
flags, this MUST be treated as a _compiler exception_ and not a _compiler error_.

#### Errors in some, but not all outputs

If a compiler error occurs in one target, but not others, the compiler MUST only produce a compiler error for the
outputs which trigger such errors. The compiler SHOULD NOT refuse to compile the other outputs unless a flag is set
requesting that the compiler abort compilation of the other outputs upon error; in such a case, the compiler MUST
treat this as a _compiler exception_ regarding the other outputs and not as a _compiler error_ for the other outputs.

#### Exceptions for limited targets

If the build file has multiple targets, it is permitted for the compiler to accept flags or arguments that cause the
compiler to only build a subset of the outputs or targets defined in the quxbuild file. If the compiler is invoked such
that it only builds a subset of the outputs or targets, it MUST only produce compiler reports for those targets it
actually builds.

#### Compiler Output

If the compiler produces an _output_, it should be considered a return value of 0 or an equivalent if using a
non-process mechanism. A non-zero return value or equivalent indicates a _compiler exception_, such as running out of
memory, or the presence of a flag that causes the compiler to abort compilation, such as a maximum template depth, or
for example, the receipt of a signal such as SIGINT or SIGTERM. A compiler error shall be detected by the absence of a
produced output and the presence of an error in the compilation report.

#### Warning promotion

A compiler MUST NOT promote warnings to errors, regardless of the compiler flags. A compiler SHOULD NOT abort
compilation if a warning is encountered, _unless_ this has been requested via a flag. If the compiler aborts due to a
warning, this MUST be treated as a _compiler exception_ and never as a _compiler error_.

#### Version Determinism

The compiler is required to produce a deterministic output for a given source code and compiler version. If a given
version of the compiler is compiled for multiple compiler-platforms, the output for a given version is required to be
the same for all compiler-platforms given the same source code. However, if a compiler exception occurs, the exception
messages need not be in the same format. The interface to the compiler is not required to use the same copyright notice
or flag structure on different compiler-platforms.

#### References to External Content, Prohibition

The compiler MUST NOT reference any system content, such as the system time, or any files outside the source bundle,
when determining the output of the compiler. If any libraries are obtained from a package management system or the
system's installed packages, they MUST be included in the source bundle by a tool other than the compiler which is run
before the compiler to prepare the source bundle. The compiler MUST NOT interact with any package management systems
directly.

#### Caching and Remote Compilation

If a caching or remote compilation system is used, the system MUST ensure that the use of remote compilation or caching
service does not affect the determinism of the compiler output. It is a violation of this specification to cause the
compilation output to be non-deterministic based on the use of a remote compilation service or a caching service. A
compliant compilation or caching service MUST ensure that caches for different compiler versions are kept separate.

#### Debugging Information

The compiler MUST NOT produce non-deterministic debugging information. The compiler MUST NOT include any information
about which platforms the compiler was executed on or otherwise provide any information that would cause the binary
outputs of the compiler to vary based on the compiler-platform. The compiler outputs may vary based on the target
platform.

#### Cross-Compilation

The compiler MUST support cross-compilation for all platforms it targets equally for all versions of the compiler on
different platforms. If a given version of the compiler can produce binaries for a given target platform, it MUST be
able to produce an identical binary output for that target platform using the compiler for any compiler-platform
supported by a given version of the compiler. The compiler MUST NOT produce different outputs based on the
compiler-platform the compiler is executed on for the same source bundle and compiler version.

#### Legal Restrictions on System SDKs

If an SDK cannot be transferred to another platform due to legal restrictions, this does NOT create an exception to the
cross-compilation requirements. In such cases, a separate tool must be used prior to invoking the compiler to retrieve
restricted SDKs and include them in the source bundle. The compiler MUST NOT integrate with mechanisms to circumvent
the cross-compilation requirements, including but not limited to, the use of DRM to restrict the source code. Nothing in
this section prohibits the use of purely legal restrictions on SDK usage, or the use of precompiled binaries.

#### Binaries

Nothing in this section prohibits the use of precompiled binary libraries, however any such precompiled binary MUST be
included in the source bundle.

#### Stderr

The compiler MAY produce non-deterministic output to stderr, such as progress messages, thread IDs or timestamps.

## Basic

### Basic.Lifetime

#### 1.1 Automatic Storage Duration

An object has automatic storage duration if it is created as a local variable within a function or block scope. Such
objects and all subobjects begin in the storage initialized state immeidately before the constructor is entered. At the
entry of the constructor, the object becomes in the partially constructed state, and any delegates are executed such
that the subobjects are constructed. Upon transitioning from delegate construction to the body of the constructor, all
subobjects are in the fully constructed state. When the constructor returns, the object is in the fully constructed
state. If a delegate initializer throws an exception, any already constructed delegate submembers are destroyed in the
reverse order of their construction during unwinding. Upon entering the constructor body but before returning normally
from it, if an exception is thrown which escapes the constructor body, delegates are automatically destroyed in
submember destruction order, regardless of the order in which the delegates were initialized. When an exception leaves a
constructor, the object is transitioned into a destroyed state. If an exception propagates to unwind a VAR statement,
and the constructor completed, the destructor is executed. Regardless of whether the destructor executes, the storage is
deallocated upon unwinding the VAR statement and pointers to it are invalidated.

Once the constructor associated with an automatic storage node has completed, the behavior of the program is undefined
if the object is destroyed by any method other than by unwinding the VAR statement in which it was created.

If an object exists at a given memory location, and another object is constructed at any overlapping memory location,
the program's behavior is undefined unless the original object is destroyed before the new object is constructed.

If a subobject of a structure is destroyed other than by the destructor of the enclosing object, the behavior of the
program is undefined, this restriction does not apply to the stored subobject of any object of storage-type (e.g.
`STORAGE@footype`).

If a storage object is destroyed while a stored subobject is still alive, the behavior of the program is undefined
_unless_ the stored subobject is trivially-destructible, in which case it is destroyed implicitly.

When an object is destroyed, including an object of trivial type like I32, BYTE or BOOL, its memory representation is
also destroyed and its storage region is _poisoned_.

Contrast: In C++, trivial type destructors are no-ops and do not poison the memory representation. In Quxlang, all
objects, including trivial types, poison their memory representation upon destruction.

Remark: This provision allows the compiler to avoid writing short-lived temporary objects to memory. When the compiler
can observe the entire lifetime of an object, it may optimize away the associated memory store operations of the object
entirely, holding it entirely in registers, even if the storage region is re-used by other parts of the program and the
compiler cannot prove that they do not read subsequently from the storage region.

If the content of an object is read through a pointer or reference of a type that does not match its actual type the
program's behavior is undefined.

Contrast: In C++, some types are compatible, such as `int` and `unsigned int`. In Quxlang, no such compatibility exists;
the types must match exactly with much more limited exceptions.

## §1 Classes

Classes are declared using `CLASS` keyword.

Classes have special functions:

### §1.1 `.CONSTRUCTOR` Functions.

#### 1.1.1 Constructor Functions

A constructor function is a function declared under the `.CONSTRUCTOR` functum.

#### 1.1.2 Default Constructor

A default constructor is a constructor function that has no parameters or only defaulted parameters.

#### 1.1.3 Conversion Constructor

A conversion constructor is a constructor function that takes a single `@OTHER` non-defaulted parameter.

#### 1.1.4 Copy Constructor

A copy constructor is a conversion constructor which is chosen during function selection in response to a
`CONST& THISTYPE` argument passed as `@OTHER` to the constructor.

#### 1.1.5 Move Constructor

A move constructor is a conversion constructor which is chosen during function selection in response to a
`@OTHER TEMP& THISTYPE` argument passed as `@OTHER` to a conversion constructor.

## §2 Functions / Functums

### 2.1 Definitions

#### 2.1.1 Definition of function

A *function* is a region of code declared with the `FUNCTION` keyword.

#### 2.1.2 Definition of functum

A *functum* is an abstract-machine symbol that is an aggregate of 1 or more included functions declared under the same
name.

#### 2.1.3 Definition of preargument

A preargument is an object constructed during evaulation of a call expression which is the full result of that
expression, and whose type is used for function selection. The preargument may be converted to the argument if the
selected function requires a conversion, or the preargument may also be the argument if the preargument and
corresponding parameter are of the same type.

### 2.2 Function Aggregation

#### 2.2.1 Fuctum Aggregation

A functum contains the functions for which an `INCLUDE_IF` clause is omitted, or the `INCLUDE_IF` clause evaluates
to true. The functum includes all such functions, even if the `ENABLE_IF` clause evaluates to false. Each functum is
a sumsymbol or submember comprising all the functions declared as the same subsymbol or submember of the same
enclosing scope. If there is a set of subsymbols and submembers with the same name, those are two distinct
functums and are not aggregated together.

#### 2.2.2 Call Procedure

A functum call is the full procedure of evaluating prearguments, selecting a function, performing any argument
adaptations required by the selected parameter types, and then invoking the selected functanoid. An invocation is the
exact-passing step of that procedure: it passes the arguments as-is, and therefore requires the argument types to
already match the parameter types exactly, except that a `->T` argument may be passed directly to a `NEW&& T` or
`DESTROY&& T` parameter to designate the storage governed by that slot.

A functum call contains the following steps:

1. Evaluation of each *preargument expression*, in the order they appear from left to right, to construct the  
   preargument objects.
2. Identification of the call invotype based on the types of the preargument objects.
3. All functions within the called function a filtered based upon whether they can be called with the call invotype, and
   the `ENABLE_IF` clause, if present, evaluating to true. The set of such functions are called *candidate functions*.
4. If there are no candidate functions, a compiler error occurs.
5. If a function is declared without a priority, it is treated to have a priority of 0. The functions are ordered in
   priority by highest numerical values. Functions with higher priorities are ordered before functions with lower
   priorities.
6. For functions of the same numerical priority, If there are multiple candidate functions, and a function is a
   *better-fit* than another function, it is ordered-before that function in priority. If a function is a *worse-fit*
   than another function, it is ordered-after that function in priority. This relationship is transitive. If a cycle
   occurs, all functions in the cycle are ordered equally.
7. If this process produces a single candidate function ordered before all other functions, then that function is
   selected for instantiation.
8. If there are multiple candidate functions equally ordered, the compiler may arbitrarily select one candidate
   function from those with the PICKANY attribute. If no such function exists and there are multiple candidate
   functions, then a compiler error occurs.
9. The function is instantiated to a functanoid with the corresponding types. If the functanoid does not compile, and
   the declaring function has the SFINAE attribute, then the compiler will ignore the function and repeat the procedure
   from step 7, excluding the ignored function. In doing so, the compiler will not alter the constructed priority graph
   even if the removal of the non-compiling candidate function would break a cycle.
10. The selection shall proceed identically for all other selections of the same functum and same call invotype. That
    is, if the compiler arbitrarily selected a function in step 8, it will always select the same function for the same
    functanoid and call invotype in all expressions in the program where it occurs.
11. The selection of PICKANY functions is required to be deterministic for a given compiler version and whole program
    source. If hash tables or other unordered structures are used, the hash function must be seeded with a value derived
    deterministically from the whole program source, such as a cryptographic hash of the source code. It is not
    permitted for internal concurrency of the compiler to affect the selection of PICKANY functions.

##### Implicit Rebinding Conversions

For the purposes of argument adaptation, an implicit rebinding conversion is any one of the following conversions
between differently bound manifestations of the same unbound type `T`:

1. Reference materialization: `T -> CONST& T`
2. Reference materialization: `T -> TEMP& T`
3. Objectization: `QUAL& T -> T`, where `QUAL&` is any reference qualifier except `WRITE&`
4. Reference requalification: `QUAL1& T -> QUAL2& T`, where `QUAL1& T` is qualification-convertible to `QUAL2& T`

The term "objectization" means constructing an object of type `T` from a reference to `T`.

The term "reference materialization" means constructing a reference to `T` from an object of type `T`.

The term "reference requalification" means converting one reference qualification of `T` to another reference
qualification of `T` without changing the unbound type and without constructing a new object.

##### Allowed Adaptations

Argument adaptation is the process by which a source type is adapted to a destination type in a call or other implicit
conversion context.

An adaptation path consists of the following stages, in order:

1. Zero or one source rebinding conversion.
2. Zero or one class conversion.
3. Zero or one destination reference materialization to `TEMP&` or `CONST&`.

No other chaining is permitted. In particular, the source stage may perform at most one implicit rebinding conversion.

Let `S` be the source type. The effective source forms of `S` are `S` itself together with each type reachable from `S`
by exactly one implicit rebinding conversion.

During template matching and during matching of a destination constructor's `@OTHER` parameter, each effective source
form is considered independently. A candidate is a viable adaptation if it succeeds for any effective source form.

The principal templating typoids used in argument adaptation are as follows:

1. `AUTO(t)` matches the non-reference form of the type presented to it.
2. `TT(t)` matches the type presented to it as-is, including references.
3. `DIRECT(t)` matches the original source type as-is.

`DIRECT(t)` does not consider any effective source form other than the original source type and therefore does not
permit source rebinding during template matching.

Examples:

1. If the source type is `S`, the effective source forms are `S`, `CONST& S`, and `TEMP& S`.
2. If the source type is `MUT& S`, the effective source forms are `MUT& S`, `CONST& S`, and `S`.

A rebinding adaptation is an adaptation in which no class conversion is performed and the source and destination have
the same unbound type.

A class-conversion adaptation is an adaptation in which the destination's unbound class is constructed by invoking a
constructor of that class with one effective source form as `@OTHER`. The class-conversion stage does not itself permit
another class conversion.

If the destination type is `TEMP& U` or `CONST& U`, a class-conversion adaptation first constructs a value of `U`, and
may then perform the final destination reference materialization to produce `TEMP& U` or `CONST& U`. Final
materialization to `MUT& U` or to any other reference type is not permitted.

Adaptation of NEW or DESTROY parameters proceeds only through identity, it cannot be adapted from any other type.

##### 2.2.2.1 Better-Fit Ordering

For overloads of the same numerical priority, a candidate function `F` is a better-fit than a candidate function `G`
if at least one corresponding argument adaptation of `F` is better-ranked than the corresponding argument adaptation of
`G`, and no corresponding argument adaptation of `F` is worse-ranked than the corresponding argument adaptation of `G`.
Here, a lesser rank number is better-ranked and a greater rank number is worse-ranked.

Explicit conversions are not permitted during argument adaptation and are therefore not considered in better-fit
ordering.

The conversion ranks are as follows:

From value:

1. Identity (no conversion): `t -> t`
2. Temporary materialization: `t -> TEMP& t`
3. Direct templating match: `t -> [template match]`
4. Temporary-materialization templating match: `t -> TEMP& T -> [template match]`
5. Const materialization: `t -> CONST& t`
6. Const-materialization templating match: `t -> CONST& t -> [template match]`
7. User-declared binding conversions, if supported
8. Implicit non-binding conversions

From reference:

1. Identity (no conversion): `QUAL& t -> QUAL& t`
2. Direct templating match: `QUAL& t -> [template match]`
3. Allowed reference requalification: `QUAL1& t -> QUAL2& t`, if `QUAL1` is qualification-convertible to `QUAL2`
4. Reference-requalification templating match: `QUAL1& t -> QUAL2& t -> [template match]`, if `QUAL1` is
   qualification-convertible to `QUAL2`
5. Direct reference objectization from the exact source reference type: `QUAL& t -> t`
6. Direct reference objectization from the exact source reference type to template: `QUAL& t -> t -> [template match]`
7. User-declared binding conversions, if supported
8. Implicit non-binding conversions

Reference objectization from `QUAL& t` to `t` is permitted if `t::.CONSTRUCTOR` is invokable with `@OTHER` of type
`QUAL& t`.

When template matching or constructor matching considers effective source forms, each such form is tested separately as
an alternative one-step source rebinding. These alternatives do not compose with one another. For example, if the
source type is `MUT& t`, the forms `MUT& t`, `CONST& t`, and `t` may each be considered, but the path
`MUT& t -> CONST& t -> t` is not permitted.



#### 2.2.3 Function Selection

When a functum is called, the abstract machine selects the function to invoke based on the types of the prearguments of
the call. After selection, the call procedure performs any required argument adaptations before the selected function is
invoked. If no argument adaptations are required, then the call's invocation passes the arguments as-is. The only
sanctioned invocation-time type mismatch is the slot-pointer rule for `NEW&& T` and `DESTROY&& T`, where an argument of
type `->T` may be passed directly to the slot parameter without conversion. This is the only case in which the runtime
argument type supplied to a functanoid invocation may differ from the functanoid's parameter type.

### Function Return

If the execution of a function reaches the end of the function body, the function implicitly returns a default
constructed object of the return type, or no object if the return type is void. However; if the return type is not
default constructible, then the function triggers a fallthrough fault (which by default, terminates the program by
calling RUNTIME::terminate()). However, if the function is declared NOFALLTHROUGH, then executing past the end of the
body of the function without an explicit `RETURN` will always trigger a fallthrough fault, even if the return type is
void or default constructible.

## Expressions

### Type Query Expressions

The following expressions query properties of a type and produce ordinary values:

* `SIZEOF(T)` produces the size of `T` in bytes.
* `BITS(T)` produces the bit width of an integral type `T`.
* `IS_SIGNED(T)` produces a `BOOL` indicating whether `T` is a signed integral type.
* `IS_INTEGRAL(T)` produces a `BOOL` indicating whether `T` is an integral type.

For these expressions, `T` is required to denote a type and not an object value.

### Static Choice Expression

`STATIC_CHOOSE(cond, true_expr, false_expr)` evaluates `cond` during compilation as a constexpr boolean and yields the
selected branch expression. Only the selected branch is required to participate in the resulting expression.

## Statements

### Assert Statement

An assert statement has the form:

```quxlang
ASSERT(condition);
ASSERT(condition, "tag");
```

The statement evaluates `condition` as a boolean. If the condition is false, execution triggers an assertion failure.
The optional string argument supplies an implementation-defined diagnostic tag.

### Runtime Statement

A runtime statement has one of the following forms:

```quxlang
RUNTIME CONSTEXPR {
    ...
}
```

```quxlang
RUNTIME CONSTEXPR {
    ...
} ELSE {
    ...
}
```

```quxlang
RUNTIME NATIVE {
    ...
}
```

```quxlang
RUNTIME NATIVE {
    ...
} ELSE {
    ...
}
```

`RUNTIME CONSTEXPR` selects the first block when execution is occurring in a constexpr execution context.
`RUNTIME NATIVE`
selects the first block when execution is occurring in a native execution context. If an `ELSE` block is present, it is
selected when the primary condition is false.

## Operators

## Arithmetic Operators

### Operator `+`

Adds two numbers.

### Operator `-`

Subtract two numbers.

### Operator `*`

Multiply two numbers.

### Operator `/`

Divine LHS by RHS.

### Operator `%`

Computes the remainder of division.

## Comparison Operators

### Operator `==`

Checks whether two values compare equal.

### Operator `!=`

Checks whether two values compare unequal.

### Operator `<`

Checks whether the left operand compares less than the right operand.

### Operator `>`

Checks whether the left operand compares greater than the right operand.

### Operator `<=`

Checks whether the left operand compares less-than-or-equal to the right operand.

### Operator `>=`

Checks whether the left operand compares greater-than-or-equal to the right operand.

## Assignment And Swap Operators

### Operator `:=`

Assigns the value of the right operand into the object designated by the left operand.

### Operator `<->`

Swaps the values of the two operands.

### Compound Assignment Operators

Arithmetic compound assignments `+=`, `-=`, `*=`, `/=`, and `%=` evaluate the left operand once and invoke the matching mutating operator overload, such as `OPERATOR+=`, on that target.

Bitwise compound assignments `#&&=`, `#||=`, `#^^=`, `#&!=`, `#|!=`, `#^!=`, `#^>=`, `#^<=`, `#++=`, `#--=`, `#+%=`, and `#-%=` follow the same rule using their matching mutating bitwise operator overload.

## Bitwise Operators

### Operator `#&&`

Bitwise and.

### Operator `#||`

Bitwise or.

### Operator `#^^`

Bitwise xor.

### Operator `#&!`

Bitwise nand.

### Operator `#|!`

Bitwise nor.

### Operator `#^>`

Bitwise implication.

### Operator `#^<`

Bitwise reverse implication.

### Operator `#^!`

Bitwise nxor.

### Operator `#++`

Bitwise shift-up.

### Operator `#--`

Bitwise shift-down.

### Operator `#+%`

Bitwise rotate-up.

### Operator `#-%`

Bitwise rotate-down.

### Operator `#!!`

Bitwise inversion.

## Logical Operators

### Operator `&&`

Logical and.

### Operator `||`

Logical or.

### Operator `^^`

Logical xor.

### Operator `^!`

Logical nxor.

### Operator `&!`

Logical nand.

### Operator `|!`

Logical nor.

### Operator `^>`

Logical implication.

### Operator `^<`

Logical reverse implication.

### Operator `??`

Converts a value to `BOOL`.

For built-in integer and pointer-like types, the result is false when the operand is zero or null respectively, and
true otherwise.

## Access Operators

### Operator `->`

Dereferences a pointer-like value and yields a reference to the pointed-to object.

### Operator `<-`

Produces a pointer to the designated object.

### Operator `[]`

Indexes into an array-like object and yields a reference to the indexed element.

### Operator `[&]`

Indexes into an array-like object and yields a pointer to the indexed element.

## Increment And Decrement Operators

### Operator `++`

Increments the operand.

### Operator `--`

Decrements the operand.

## Builtin Operator Coverage

The abstract machine may provide built-in overloads for primitive integers, bytes, booleans, pointers, references, and
arrays. In particular, built-in implementations are permitted for arithmetic, comparison, assignment, swapping,
array-indexing, booliation, pointer dereference, pointer arithmetic, and the bitwise operators described above.

## Operator Synthesis

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
memory location. In this case, the address of the object when the constructor is called and when the destructor is
called
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
