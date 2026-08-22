# Conversions

An explicit conversion uses `AS`, optionally followed by a conversion mode:

```quxlang
VAR wide I64 := 300;
VAR narrowed I8 := wide AS PARTIAL I8;
VAR checked I32 := wide AS CHECKED I32;
VAR assumed I32 := wide AS ASSUME I32;
VAR approximate F32 := 0.4 AS APPROXIMATE F32;
```

## Modes

| Form | Contract |
| --- | --- |
| `expression AS Type` | Ordinary explicit conversion with no extra permission |
| `AS EXPLICIT Type` | Selects an explicit user conversion category |
| `AS PARTIAL Type` | Permits a partial representation result, such as discarded high bits |
| `AS CHECKED Type` | Requires validity to be checked and faults on failure |
| `AS ASSUME Type` | Makes validity a program precondition |
| `AS APPROXIMATE Type` | Permits an inexact numeric result |
| `AS REINTERPRET Type` | Selects a representation-level conversion explicitly allowed by the type system |

## Narrowing

Narrowing is not silently accepted. The programmer selects whether truncation,
a dynamic check, an assumption, or approximation matches the intended
semantics.

## Pointer reinterpretation

Current pointer reinterpretation supports explicit paths such as conversion
through `VOID` while preserving pointer category and validity constraints:

```quxlang
VAR pointer CONST->I32 := value<-;
VAR erased CONST->VOID := pointer AS REINTERPRET CONST->VOID;
VAR restored CONST->I32 := erased AS REINTERPRET CONST->I32;
```

`REINTERPRET` is not general permission to access storage as an unrelated live
type. Use [typed storage and explicit lifetime](typed-storage-and-lifetime.md)
for lifetime transitions.

## User-defined conversions

Constructor parameter names select the corresponding conversion categories,
including `@OTHER`, `@EXPLICIT`, `@PARTIAL`, `@CHECKED`, `@ASSUME`,
`@APPROXIMATE`, and `@REINTERPRET`. See
[Constructors and destructors](constructors-and-destructors.md).
