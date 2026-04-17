# TODOs

This list tracks high-level project gaps. Design notes may describe intended syntax
or semantics that are not implemented yet.

## Language features

- [ ] Floating point numbers
- [ ] Atomic operations
- [ ] Non-antestatal static constants
- [ ] Function-local static constants
- [ ] Function-local static variables
- [ ] Variadic arguments
- [ ] For loops
- [ ] Compound assignments
- [ ] Exceptions and unwinding
- [ ] Polymorphism / virtual functions
- [ ] Lambda functions / closures
- [ ] Coroutines

## Compiler and runtime

- [ ] Non-constexpr codegen and linking
- [ ] Restore/complete the VMIR2 LLVM backend
- [ ] Complete runtime unwinding support

## Frontend and VMIR correctness

- [ ] Defaulted function arguments
- [ ] AUTO return type inference
- [ ] Static tests inside concrete functions
- [ ] VMIR2 constexpr pointer comparison and pointer difference semantics
- [ ] VMIR2 constexpr lifetime and state-transition checks

