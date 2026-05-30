# Control Flow

Control flow in Quxlang prioritizes implicit control flow over explicit control
flow. This diverges from languages such as Rust and Zig.

The rationale is that forgetting to release a resource is a more common mistake
than an operator performing an unintended side effect. Quxlang therefore keeps
features such as RAII, destructors, exception handling, and operator
overloading.

## Programmer responsibility

Operators are expected to behave reasonably, and Quxlang places responsibility
on programmers to use them reasonably. This is not unique to operator
overloading: a badly named function can also perform surprising side effects.

There are situations where operator overloading is merited and situations where
it is not. A matrix type overloading `*` for matrix multiplication is reasonable.
Using `+` to make a GUI widget appear would not match the ordinary meaning of
addition.

## Why implicit control flow remains

Quxlang places these responsibilities on the programmer, not the language.
Removing implicit control flow would burden skilled programmers and make
resource management harder to express.
