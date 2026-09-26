# Quxlang Overview

The Overview introduces Quxlang through examples and common use cases. Each
page describes a feature's purpose, typical applications, and relevant
performance or implementation costs. The [Reference](../reference/index.md)
specifies exact syntax, language rules, restrictions, and edge cases.

## Introductory material

[Getting Started](getting-started.md) describes the required tools and initial
project setup. [First Program](first-program.md) presents a complete buildable
program. [Syntax at a Glance](syntax-examples.md) collects short examples of
the principal language constructs.

The first program introduces the ordinary development workflow:

```text
source files -> qxcbuild.yml -> configured output -> executable or library
```

[Source Bundles and Targets](source-bundles.md) explains why one source bundle
can produce several target-specific outputs.

## Topics by application

| Application | Overview pages |
| --- | --- |
| Local state and record modeling | [Variables](variables.md), [Structures](structs-and-members.md), and [Constructors and Destructors](constructors-and-destructors.md) |
| Object borrowing, sharing, and allocation | [References](references.md), [Pointers](pointers.md), [Optional and Owning Values](optional-and-owning-values.md), and [`NEW` and `DELETE`](new-and-delete.md) |
| Function interfaces | [Functions](functions-and-parameters.md), [Call Arguments](call-arguments.md), and [Default Arguments](default-arguments.md) |
| Code reuse across types | [Templates](templates-and-value-parameters.md), [Interfaces](interfaces-and-implementations.md), and [Generics](generics.md) |
| Alternative-value representations | [Enums](enums.md), [Unions](unions.md), [Variants](variants.md), [`VISIT`](visit.md), and [`MATCH`](match.md) |
| Recoverable failure handling | [Exception Handling](exceptions.md), [Failure Statements](diagnostics-and-failure.md), and [Compilation Policies](compilation-policies.md) |
| Compile-time execution | [Compile-Time Evaluation](compile-time-evaluation.md), [`STATIC` Constants](static-compile-time-constants.md), and [Runtime Selection](runtime-selection.md) |
| Operating-system and processor adaptation | [Target Availability](availability-and-targets.md), [External Procedures](external-procedures.md), and [Assembly Procedures](assembly-procedures.md) |
| Data outside the Quxlang object model | [Serialization](serialization.md), [External Types](external-types.md), and [External Memory](external-memory.md) |
| Concurrent coordination | [Atomics](atomics.md) and [Thread-Local Variables](thread-local-variables.md) |

## Performance and implementation considerations

The [Composites](composites.md) page describes temporary heterogeneous records
and argument forwarding. [Move Semantics](move-semantics.md) and
[Swap](swap-operator.md) describe transfers of resource-owning values.
[Inheritance](inheritance.md) describes base-subobject layout and optional
dynamic dispatch. [Compile-Time Allocation](constexpr-allocation.md) and
[Object Storage and Lifetime](typed-storage-and-lifetime.md) describe explicit
storage decisions. [Floating-Point Ordering](floating-point-ordering.md)
compares the total value order with IEEE predicates. [Build Options](build-options.md)
and [Compilation Policies](compilation-policies.md) describe per-output
behavior.

The navigation contains the complete Overview topic list. The
[Reference index](../reference/index.md) provides accepted syntax, overload
selection, failure behavior, target restrictions, and complete operation lists.
