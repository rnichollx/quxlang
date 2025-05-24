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