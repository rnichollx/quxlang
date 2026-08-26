# Evolution from C++

This section describes how Quxlang evolves from C++, introducing important changes and semantic differences.


## Syntax

Quxlang has a highly regular syntax. This is focused on regularity, predictability, and fast parsing.

The name of the thing being declared is the first token, with `::name` used for subsymbols and
`.name` used for members. This allows the programmer to quickly scan the source for the name
the programmer is looking for, without mentally parsing other information.

This approach also improves the speed of the parser, as it doesn't have to do complex
pattern matching to identify the name of the thing being declared, it first finds the name,
constructs a map entry for it, then parses the rest of the declaration and stores it.

As a result, the parser has no "extraction" step to extract the name from the declared symbols, 
which improves the speed of the parser.

The most obvious syntax change, aside from this ordering, is the use of `UPPERCASE` keywords
and `lowercase` identifiers. Quxlang uses this naming syntax because it allows new keywords to
be added without breaking any existing code, which is useful for future language evolution
which can add new keywords without breaking existing code.

The second reason for uppercase keywords is to support multiple languages, it may be possible
in the future to support other keyword sets in other languages. Even if someone memorized all
the english keywords, it's unlikely they could memorize all the keywords in other languages.

The use of lower case ensures that keywords and identifiers do not conflict, and also makes the
language more readable, because keywords can be immediately identified.

## Compile time evaluation and serialoids

Quxlang can execute most code at compile time, including complex code like `std::set` 
and `std::map`. In Quxlang, no special `constexpr` declaration is required to execute code
at compile time, code is automatically allowed during constexpr unless it does something
which is illegal in a constexpr context, such as calling an assembly routine or bit 
manipulatinting pointers.

Unlike in C++, in Quxlang, complex types like `std::set` can be used as compile time STATIC 
declarations. Most `static constexpr` declarations in C++ would be classified as "antestatal statics"
in Quxlang. However, Quxlang supports a second type of static, called a "serialiod static".

Serialoid statics allow any regular type which can be serialized and deserialized to be declared
as a compile time static. During constexpr evaluation, the object is serialized into the serialized
format.

When the object is accessed for the first time, it is deserialized from the precalculated serialized
representation. This means that the object still has non-trivial intialization and must potentially
allocate memory, but the data contents of the object is available immediately without recalculation.

Such serialoid objects can also be accessed from other constexpr evaluations, and deserialize normally
in the constexpr execution context on first access using the normal constexpr memory allocator and
ordinary deserialization mechanism.

## Steppings

Comapred to C++, Quxlang can achieve higher performance in practice due to the use of program
steppings. Program steppings allow the compiler to generate multiple procedure versions of every
routine in the program, and then select the best one for the current CPU at program startup.

Conceptually, you can imagine program steppings like compiling a program multiple times, with
options like `-march=x86-64-v1`, `-march=x86-64-v2`, `-march=x86-64-v3`, `-march=x86-64-v4` etc.
Then, you create a thin wrapper program that detects the highest version of the generic x86-64
instruction ladder available on the current CPU and runs the corresponding program.

As a result, programs compiled by `qxc` tend to perform close to `-march=native` performance,
as these extension versions capture the majority of performance increases available in successive
CPU instruction sets.

While this can be done in C++, it's mostly a manual affair and tends to have drawbacks. In Quxlang,
this feature is enabled by default in Release optimization configurations.

The main downside of program steppings is that it can increase the amount of executable code used by
the program substantially. However, most OS can recognize that parts of the program data are not 
being used.

## Safety

Quxlang does not aspire to be a "safe" language where there are no undefined behaviors. However, it
has been observed that certain undefined behaviors lead to almost no performance benefits and thus
can be removed as a default behavior, with opt-ins to use other behaviors for cases where the 
alternative behaviors are desired.

Quxlang does this in two main places, default initialization and overflow behavior. In Quxlang, all
types other than storage types are initialized to zero by default.

For example:

```quxlang
VAR x I32;
// x is guaranteed to equal 0
```

In practice, this fixes many bugs, and has a negligible performance impact, so it has been chosen as
the default behavior. In the rare circumstance where early intialization has a meaningful performance
impact, typed storage can be used instead:

```quxlang
VAR x TYPED_STORAGE(I32);
// x is not initialized with any object
...
PLACE AT(x) I32 := 9;
// Construct I32 in the storage

...
foofunc(PUN x AS I32);
// acccess an object in storage, behavior is undefined if the object doesn't exist
...
DESTROY AT(x) I32;
// Destroy the object that was in typed storage when done using it.
```

## Trivial Destructors

In Quxlang, trivial type destructors are not "no-ops" and memory cannot usually be reinterpreted to create objects where none existed.

This means that even trivial destructors have a side effect, namely, they destroy the object, erasing its data,
and causing an attempt to access the object after destruction to be undefined behavior. This is an inherent property
of destructors in Quxlang, object destruction "erases" the content of an object from the perspective of the optimizer.

In C++, trivial destructors are "no-ops" and memory can be reinterpreted to create objects where none existed.

This trades a small set of cases where code would be legal for additional optimiation opportunities. The 
optimization opportunites of discarding destroyed object content in the optimizer is considered far
more valuable than preserving defined behavior in the set of cases where programmers did not follow
lifetime behaviors strictly.

!!! note "Secure Object Destruction"
    In future versions of Quxlang, it will be possible to declare "secure" objects which guarantee all data is zero'd out
    when destroyed, even if such zeroing appears to have no observable side-effects. This is intended to guarantee that cryptographically
    sensitive data is not retained past the lifetime of any object which contains it if the program memory is examined by
    an attacker. This behavior is not yet implemented.
    
## Interfaces and Generics

Unlike C++, Quxlang implements "interfaces" and "generics" as first class language features. Interfaces allow the programmer
to implement vtable-like dispatch without template and constexpr metaprogramming. Generics provide type-erasure in an
easy to use way.