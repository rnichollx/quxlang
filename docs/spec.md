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

#### 2.2.3 Function Selection

When a functum is called, the abstract machine selects the function to invoke based on the types of the prearguments of
the call.

### Function Return

If the execution of a function reaches the end of the function body, the function implicitly returns a default
constructed object of the return type, or no object if the return type is void. However; if the return type is not
default constructible, then the function triggers a fallthrough fault (which by default, terminates the program by
calling RUNTIME::terminate()). However, if the function is declared NOFALLTHROUGH, then executing past the end of the
body of the function without an explicit `RETURN` will always trigger a fallthrough fault, even if the return type is
void or default constructible.


## Operators
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




