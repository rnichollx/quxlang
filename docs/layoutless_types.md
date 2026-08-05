# Layoutless Types

Quxlang supports the concept of a "layoutless type". While machine-native backends like LLVM always produce
layouts for all classes, when using a backend like Cortado that produces Java bytecode, some types may be
"objects" which are dynamically allocated by a JIT where the compiler cannot know the layout of the object
in advance. Such classes are called "layoutless types".

For this reason, Quxlang provides the `TYPE_IS_LAYOUTLESS(type)` expression, which returns true if the given
type is layoutless and false otherwise. On LLVM targets, this expression always returns false.

On the current JVM target, only `I8`, `U8`, `I16`, `U16`, `I32`, `U32`, `I64`, and `U64` have fixed byte sizes.
Other integer widths are layoutless: `U24` may require ABI padding, while wider integers such as `U128` may
require managed object storage. Aggregates and arrays are also layoutless.

`SIZEOF` is available for those eight fixed-width integer types on the JVM target. `SIZEOF` for every other
type, all uses of `ALIGNOF`, and all reached `ALIGNED_STORAGE` types are semantic errors on layoutless targets.
Target-specific code can use `STATIC_IF(!ARCH_IS_LAYOUTLESS)` or `TYPE_IS_LAYOUTLESS(type)` to keep such
operations unreachable.

Sizeless and layoutless are distinct properties; an empty size-zero type is not necessarily layoutless.
