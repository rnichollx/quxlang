# Quxlang Overview

## Introduction

Quxlang is a multi-level compiled programming language, which is most similar to C++. Most of Quxlang's semantics are
similar to C++, although the syntax can be more similar to Go in some cases. Semantically, Quxlang keeps C++'s
focus on value-oriented programming, including destructors, and user-defined customizable types, with compile time 
metaprogramming. Quxlang broadly intends to be an improvement over C++ while breaking backwards compatibility with
exisitng C++ syntax. Quxlang is not intended to be a language which is opposed to core C++ ideology and design, but
rather does what C++ cannot do because of legacy compatibility.

In contrast to Carbon, Quxlang is not intended to be "easy C++". In many ways, many of the complexities of C++ will
remain, and even some new ones introduced. Unlike Rust, Quxlang is not intended to be a "safe C++". Where we can
simplify things without costs, we do so, and where we can gain safety without major costs, we may also do so.

What Quxlang does intend to do is provide fast deterministic builds and cross compiles, in a way that cannot be easily
done with modern C++ tooling.

Quxlang also intends to innovate on performance in some areas, to provide a language that
can beat modern C++ performance. Quxlang should be able to beat C and C++ for practical performance in many cases,
when comparing idomatic code that has not been thoroughly optimized.

Quxlang also intends to improve ergonomics. This doesn't mean a simpler language, but rather a more regular one that
provides power where needed.

Finally, Quxlang intends to provide a more accessible language which provides multiple language implementations. In 
practice, this means that files could begin with e.g. `LANGUAGE QUXLANG 1.0 EN;` or `LANGUAGE QUXLANG 1.0 JP;`,
and the language keywords can be translated to the language of the user. 

## Keywords

In Quxlang, all keywords are written using `UPPERCASE`, in contrast, all identifiers must use `lower_case`. This ensures
that new versions of Quxlang cannot introduce new keywords that conflict with existing identifiers used in code.
Therefore, the goal is that it should always be safe to upgrade to a new version of Quxlang without breaking existing
code. For example, `IF` is a keyword, whereas `if` is a valid identifier that could be used as a variable name if so desired. `MixedCase` is not allowed.

## Syntax Basics: Globals, Types and Functions

In Quxlang, global declarations begin with `::name`, followed by a definition. Type syntax is similar to Go, with instance pointers using `->Type` instead of `*`. The basic types are `BOOL`, `I32`, `U64`, `F32`, etc. rather than `INT` or `LONG` etc. Special types which alias the basic integer types include `SZ` (unsigned), `SIGNED_SZ`, `UINTPTR` and `SINTPTR`. Arrays are declared using (`[` _N_ `]` _Type_), where N is the number of elements in the array, and _Type_ is the type of the elements.

There is no `=` operator in Quxlang. As a new programmer, I often used operator `=` where I should have used `==`. This mistake is more common than you think. To help prevent this mistake, assignment uses `:=` instead of `=`. Thus `=` will never compile, you must use either `:=` (for assignment) or `==` (for equality comparison).

Functions begin with the `FUNCTION` keyword. Quxlang supports two types of arguments, named arguments and positional arguments. Named arguments are specified using `@name Type`, and positional arguments are specified using `%name Type`. Return types are specified after a trailing colon `:`.

Variables are declared using `VAR name Type;`.

For example, here is a function which adds two numbers together:

```quxlang
::add_numbers FUNCTION(%a I32, %b I32): I32 {
   VAR result I32;
   result := a + b;
   RETURN result;
}
```

Here is a function which adds values to an array:

```quxlang

::add4 FUNCTION(@array ->[4]I32, @value I32) {
  VAR i I32;
  WHILE (i < 4) {
    array->[i] += value;
    i := i + 1;
  } 
}
::other_function FUNCTION() {
  VAR my_array [4]I32;
  
  add4(@value 10, @array my_array<-);
}
  
```

A few points are illurstrated in the above code:

* Primitive types are read from left to right, so `->[4]I32` is a pointer-to(`->`) an array of 4 (`[4]`) 32-bit integers (`I32`).
* Accessing an instance pointer is done using a suffix `->` instead of a prefix `*`.
* To get the address of a variable, you use the suffix `<-` instead of the prefix `&` as in C++.

To understand why this syntax was chosen, consider in C/C++: `&a[0]`. Is this equal to `(&a)[0]` or `&(a[0])`? If you are not familiar with the order of operations, it is not immediately obvious. In Quxlang by contrast, the equivalent to `(&a)[0]` would be `a<-[0]`, and the equivalent to `&(a[0])` would be `a[0]<-`. Thus we can immediately read the code from left to right without confusion. `a[0]<-` means: 

  * Get `a`,
  * Access the 0id element
  * Take its address.

`a<-[0]` means: 

  * Get `a`,
  * Take its address.
  * Access the 0id element

And for the logical not (`!!`), lets suppose we have an expression like `a<-[0]!!`, this then means:

  * Get `a`,
  * Take its address,
  * Access the 0id element,
  * Apply logical inversion.

Thus most suffix expressions in Quxlang can be read from left to right without confusion. This is a deliberate design choice to make the language more readable and less error-prone.

