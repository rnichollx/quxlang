# Privacy

## Syntax

Quxlang declarations are public unless they have a `PRIVATE(...)` prefix. Privacy is a compile-time name-access rule and does not change layout, linkage, symbol mangling, or ABI.

The single-declaration form places privacy before the declaration name:

```quxlang
PRIVATE(CLASS) .x VAR I32;
PRIVATE(MODULE) ::initialize FUNCTION();
```

The block form applies one scope list to several declarations. For example, private class members can be written as:

```quxlang
::string STRUCT
{
  PRIVATE(CLASS)
  {
    .data_val VAR =>>BYTE;
    .size_val VAR SZ;
    .cap_val VAR SZ;
  }
}
```

The block applies to each declaration directly contained by it. Privacy is not propagated into the contents of a declaration: a class or namespace declared in a private block is private, while its members and subsymbols remain public unless they have their own `PRIVATE(...)` annotation.

`PRIVATE(CLASS)` grants access to the nearest enclosing `STRUCT`, `UNION`, `VARIANT`, or `IMPLEMENTATION`, including its nested contexts. `PRIVATE(MODULE)` grants access to the declaration's module. A type-symbol entry grants access to that context and its nested contexts:

```quxlang
PRIVATE(RUNTIME_MODULE) ::runtime_state STRUCT {}
```

`RUNTIME_MODULE` is the source-visible absolute reference to the runtime module and is available from every module. `MODULE(name)` is not accepted in Quxlang source. `MODULE` remains valid by itself inside `PRIVATE(MODULE)`, where it denotes the declaration's current module rather than a type symbol. `RUNTIME` remains reserved for `RUNTIME CONSTEXPR` and `RUNTIME NATIVE` statements.

Several entries are alternatives: access through any listed scope is sufficient. A declaration is always accessible from its own nested contexts.

Direct nesting of `PRIVATE` inside a `PRIVATE` block is rejected. Postfix privacy, such as `::x PRIVATE(MODULE) VAR I32;`, is also rejected. `CLASS` and `MODULE` are built-in scope entries; all other entries use the ordinary Quxlang type-symbol syntax.

## Access rules

An access succeeds when the accessing context is the declaration itself or is nested within any allowed context. Otherwise compilation fails. Privacy applies only to the annotated declaration and is not inherited by its members or subsymbols.

Every lexical prefix in a qualified name must be accessible. Access to `a::b::c` therefore fails when `a::b` is private even if `a::b::c` is public. A private declaration still shadows an outer declaration with the same name; inaccessible lookup does not continue searching for a public declaration.

Reopened non-overload declarations must specify equivalent privacy scopes. Mixing public and private reopenings, or using different scopes, is an error. Duplicate entries within one `PRIVATE(...)` list are also an error.

When overload resolution selects an inaccessible function or template, compilation fails; overload resolution is not rerun to find an accessible fallback. Compiler-provided overloads are public.

Privacy does not prevent public declarations from exposing private types in their signatures.
