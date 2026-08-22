# Reference Conventions

The Reference division describes contracts, not merely accepted spellings.
This page defines how to read those contracts and how implementation status is
reported.

## Overview versus Reference

| Division | Use it for |
| --- | --- |
| [Overview](../overview/index.md) | Learning a feature, seeing its usual form, and choosing the right language mechanism |
| [Reference](index.md) | Checking accepted forms, semantic distinctions, constraints, target boundaries, and failure behavior |

Overview examples favor one ordinary use. A Reference page may show several
forms because differences in qualification, initialization, ownership, or
target selection are part of the contract.

## Implementation boundary

The Reference division documents the current compiler surface. A source form
belongs in the Reference only when its parser and the relevant semantic or
generation path implement the described behavior. Parser recognition alone is
not a support guarantee.

When a spelling is reserved or parsed but its behavior is unavailable, the
Reference states **feature is not implemented** or identifies the form as
reserved syntax. Forward-looking design proposals and VMIR engineering
instructions are outside the source-language Reference.

## Source fragments

A complete source file begins with:

```quxlang
LANGUAGE QUXLANG EN 0.0;
```

Most feature pages omit that line so an example can focus on the construct at
hand. Lowercase names such as `value`, `source`, and `t` are example
identifiers. Uppercase names are language keywords, reserved interface names,
or compiler-provided symbols.

An isolated fragment may refer to a declaration introduced by nearby prose or
an earlier fragment on the same page. The syntax and semantic relationship are
the subject of the example; a fragment is not necessarily a complete source
bundle by itself.

## Declaration terminology

- A **declaration** introduces a named entity such as a variable, function,
  namespace, type, test, option, or external symbol.
- A **global** declaration belongs directly to a logical module. `::name`
  introduces a module-, namespace-, or type-owned name according to context.
- An **instance member** uses `.name` and belongs to an object. A nested
  `::name` inside a type is not an instance member.
- A **source bundle** is the complete compiler input selected by
  `qxcbuild.yml`, including logical module mappings and target properties.
- An **active source configuration** is the declaration set remaining after
  compile-time inclusion conditions are applied for one configured target.

## Type and value terminology

- A **value type** is the declared object type without a reference qualifier.
- A **reference category** states how an expression refers to storage:
  `MUT&`, `CONST&`, `TEMP&`, `WRITE&`, or a deduced `AUTO&` form.
- An **instance pointer** (`->T`) identifies one object. An **array pointer**
  (`=>>T`) identifies a position in a multi-object allocation or sequence.
- A **managed reference** (`~>T`) is a layoutless-runtime reference and does
  not imply the native pointer representation.
- A **literal type** preserves one exact source literal for matching. A
  compile-time constant category such as `STRING_CONSTANT` preserves computed
  read-only data. Neither is an ordinary mutable runtime container.

## Storage, lifetime, and ownership

Storage and object lifetime are separate technical concepts:

- `TYPED_STORAGE` or `ALIGNED_STORAGE` reserves storage without starting a
  payload lifetime.
- `PLACE AT` starts a payload lifetime and `DESTROY AT` ends it.
- `NEW` and `DELETE` combine the default allocator path with construction and
  destruction for one object.
- `GENERIC_REF` is non-owning. `GENERIC` owns its erased object. An interface
  handle selects function implementations rather than owning an erased user
  object.

The feature pages state where automatic destruction, explicit destruction, or
non-owning lifetime obligations apply.

## Compile-time, runtime, and target distinctions

`STATIC`, `STATIC_IF`, `STATIC_WHILE`, and `STATIC_TEST` operate through
compile-time evaluation. `RUNTIME CONSTEXPR` and `RUNTIME NATIVE` explicitly
select paths by execution mode.

Target predicates such as `ARCH_IS_X64` and `BACKEND_LLVM` are compile-time
values. CPU `HAVE_*` queries are runtime capability queries and are not valid
`STATIC_IF` or `INCLUDE_IF` conditions.

A layoutless target has no source-visible fixed byte layout for types whose
representation belongs to the managed runtime. Layout queries and native
storage forms state when that distinction makes an operation unavailable.

## Diagnostics and unsupported paths

Reference constraints are compile-time errors unless a page explicitly
describes runtime failure, trapping, or an unchecked precondition. In
particular:

- `AS CHECKED` performs validation and faults when the check fails.
- `AS ASSUME` makes validity a program precondition.
- `ASSERT` fails in its current execution mode.
- `COMPILATION_ERROR` rejects a selected source path.
- `UNIMPLEMENTED` follows the target's configured trap-or-error policy.

The [Failure Statements](diagnostics-and-failure.md)
page defines these programmer-visible failure mechanisms in detail.
