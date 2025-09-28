# Quxlang Philosophy

## Overview

Quxlang is intended as a systems programming language with flexibility and performance, with power and expressiveness.

Guiding principles:

  * _Simplicity is a bonus, not a goal_. Problems ought to be solved, not ignored in the name of simplicity.
  * _Complexity for the sake of complexity is bad_, if a problem can be solved in a simple way without downsides, the simple way should be preferred.
  * _The language should prioritize a design that makes it easy to write bug-free code_, even if it means that the language is more complex.
  * _Performance is critical, but not the only concern_. We may choose defaults that are not the most performant if they measurably improve safety or usability. This means for example, we choose to zero-initialize variables by default, but allow an opt-out.
  * _Compile time is a major concern_, so the design should favor a syntax that is semantic context free, even if that makes the langauge syntax less familiar.
  * _Breaking compatibility with old code is a big no-no_. The entire UPPERCASE set is reserved for keywords so that new keywords can be added liberally in new versions without the possibility of breaking old code.
  * _The compiler should be a deterministic and reproducible cross-compiler_. This means that the compiler should produce the same output given the same input, regardless of the environment in which it is run.
  * _Quxlang should try to reproduce the entire feature set of C++_. Some things might take a different form, but the goal is to be a better C++, not a simpler or limited subset of C++. It isn't necessary for every library feature to be implemented, the core language should have every useful feature of C++. There may be instances where things are not quite 1:1, for example how Quxlang has "TEMP & T" and "CONST &" instead of "T&&" and "T const&", as well as no equivalent to "T const&&". But it should preserve the useful spirit of C++.
  * _Quxlang should push innovation in performance_. This means trying new things like letting the compiler optimize structure layouts for cache coherency and generating multiple procedures for the same routine with different sub-architecture levels and optimization tunings.
  * _Quxlang should be the one ~~ring~~ language to rule them all_. It should be possible to write code in Quxlang that runs on CPU, GPU, TPU etc. without the need of particularly specialized tools. Likewise, Quxlang should be suitable not only for userspace programming, but also for kernel, embedded, and firmware development.
  * Convention can be ignored in favor of clarity and expressiveness. Familiar constructs are preferred where possible, but not if the concepts are unclear to new programmers.

# Operators

Quxlang has a much larger number of operators than C++. In addition to usual logical operators like `&&`, it also has logical operators like `&!` and `^^`. The decision was made that single logical operations should be expressed with a single operator, not a combination of operators. A second guiding principle is to generally prefer suffix operators over prefix operators. By using mostly suffix operators, it makes it easier to read code left to right. There are some exceptions, for example `++i` and unary `-i`. In such cases, we should still read left to right, so `--a++` would first decrement, then increment.

In this way, we chose `ptr->` as the method of dereferencing a pointer, similar to `arry[i]`. Both use suffixes. To get the address of an element, we use the `<-` suffix. For example, `a[6]<-` gets the address of the 6id element of `a`. The choice of `<-` was made to mirror the `->` operator.


For array indexing, we also support a syntax for accessing elements, the `[&]` operator, for example `a[& 8]` gets the address of the 9th/8id element of `a`. We permit taking the address of the one-past-the-end element of an array, but not accessing it. So supposing `a` is of type `[8]I32`, then `a[& 8]` is valid, but `a[8]<-` is not. This differs from C++ where undefined behavior only occurs when the invalid reference is _used_.

For logical negation, we use the `!!` suffix operator. For example, in C++ `!a` is equivalent to `a!!` in Quxlang. The choice of `!!` instead of `!` was mainly for stylistic reasons and to allow single `!` to be used for other purposes.


Another major change from C++ is the set of bitwise operations. In general, bitwise operators are the same as logical operators, with a `#` prefix. So bitwise and is `#&&` and bitwise or is `#||`. Likewise, bitwise negation or complement is a suffix operator, `#!!`. 

One surprising element of Quxlang however, may be the choice of bitwise "shift" operations. Quxlang does not have a "shift right" or "shift left" operator. Instead Quxlang has "shift up" and "shfit down" operators. Shift up is `#++` and shift down is `#--`. The C++ "left shift" `<<` is replaced with "up shift" `#++`, and the C++ "right shift" `>>` is replaced with "down shift" `#--`.

 The decision to use "up" and "down" instead of "left" and "right" has been made for several reasons. The first is that for people without prior programming exposure, left and right shifts are conceptually confusing and non-intuitive. The terminology of up and down shifts, on the other hand, are more intuitive due to their immediate similarity to "round up" and "round down" conceptually.
 
The next reason is that when doing arithmetic in computer systems, it's often much more convenient to use the so called "little endian" representation in computer memory, where the least sigificant portions are stored in lower order positions. Our decimal notation we use is in the so-called "big endian" format. For example, two hundred and thirty four is written as "234", which is a big endian representation. In little endian, it would be written as "432" instead.

When visualizing arrays, it's conventional to view array elements as indexing higher orders left to right, for example, in the array `[1, 2, 3, 4]`, the `1` is the lowest order element and the `4` is the highest order element.

However, when looking at a little endian binary representation of an array, suppose we have a 16 bit number in little endian byte order, but representing the individual bytes in big endian, as conventional `B_10000100 B_00000010` (Quxlang uses `B_` instead of `0b` to represent binary literals).

Suppose we then "left shift" this number by 1, the result is `B_00001000 B_00000101`. The leftmost bit of the first byte has been shifted into the rightmost bit of the second byte. This is confusing as we represent the individual bytes in big endian, but the overall number is in little endian. It would be more convenient to visualize this with all the bits in little endian order, so the original number would be `00100001_B 01000000_B`. This has the bizarre consequence that a "left shift" would move the bits to the right, and a "right shift" would move the bits to the left. Such a terminology introduces meaningless confusion when working with little endian numbers, so the decision to "up shift" and "down shift" instead has been made. The effect of an "up shift" is to make the number larger, and the effect of a "down shift" is to make the number smaller. Thus, the terminology of "up" and "down" shifting is endian neutral, unlike traditional "left" and "right" shifting which assume a big endian representation.

 While this may seem like a very specific problem, anyone that has worked on compression and/or huffman coding may find this a welcome change.
 
The conventional use of `0b` to represent binary literals in C++ has been replaced with `B_` prefix in Quxlang for big endian binary literals, and `B` suffix for representing binary literals in little endian. For example, the number two can be represented as `B_10` or `10_B`. This the `B` is "attached" to the least significant digit. Likewise, `X_FA` and `FA_X` represent the same number in hexadecimal.
