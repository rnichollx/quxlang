# Privacy

## Design overview

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

The parser expands the block into ordinary declarations. There is no privacy-block AST entity. Each declaration directly contained by the block carries the same source `privacy_scope`; interface-function blocks are likewise flattened, with privacy stored on each directly contained interface function declaration. Privacy is not propagated into the contents of a declaration: a class or namespace declared in a private block is private, while its members and subsymbols remain public unless they have their own `PRIVATE(...)` annotation.

`PRIVATE(CLASS)` grants access to the nearest enclosing `STRUCT`, `UNION`, `VARIANT`, or `IMPLEMENTATION`, including its nested contexts. `PRIVATE(MODULE)` grants access to the declaration's module. A type-symbol entry grants access to that canonical context and its nested contexts:

```quxlang
PRIVATE(RUNTIME_MODULE) ::runtime_state STRUCT {}
```

`RUNTIME_MODULE` is the source-visible absolute reference to the runtime module and is available from every module. `MODULE(name)` is an internal canonical-symbol spelling and is not accepted in Quxlang source. `MODULE` remains valid by itself inside `PRIVATE(MODULE)`, where it denotes the declaration's current module rather than a type symbol. `RUNTIME` remains reserved for `RUNTIME CONSTEXPR` and `RUNTIME NATIVE` statements.

Several entries are alternatives: access through any listed scope is sufficient. A declaration is always accessible from its own nested contexts.

## Detailed design

### Parsing and AST

`try_parse_privacy_scope(parsing_context&)` recognizes `PRIVATE(...)` before a declaration name. `parse_subdeclaroids` keeps an `std::optional<privacy_scope>` while parsing a privacy block and passes it to `try_parse_subdeclaroid`. The block's declarations are appended directly to the containing declaration vector.

Direct nesting of `PRIVATE` inside a `PRIVATE` block is rejected. Postfix privacy, such as `::x PRIVATE(MODULE) VAR I32;`, is also rejected. `CLASS` and `MODULE` are built-in scope entries; all other entries are parsed using the ordinary Quxlang `type_symbol` grammar.

The source structures are:

- `privacy_scope_entry`: a `privacy_scope_kind` plus an optional named `type_symbol` context.
- `privacy_scope`: the source-ordered vector of entries.
- `member_subdeclaroid::privacy` and `global_subdeclaroid::privacy`: optional source privacy attached to a declaration.
- `ast2_interface_function_declaration::privacy`: the same optional source privacy for interface members, whose existing AST representation is separate from subdeclaroids.
- `resolved_privacy_scope`: a canonical `std::set<type_symbol>` of allowed contexts plus the declaration location.

The source vector preserves syntax and source locations. The resolved set provides canonical identity, duplicate detection, and order-independent comparison for reopened declarations.

### Resolution and access

Privacy is resolved relative to the declaration's parent scope. `CLASS` walks outward to the nearest supported class boundary. `MODULE` selects the root module. Named entries use access-neutral canonical lookup so resolving privacy cannot recursively require the privacy being resolved.

An access succeeds when the accessing context is the declaration itself or is nested within any resolved allowed context. Otherwise compilation fails. Selected-declaration checks, including field lowering and overload selection, check only that declaration's own privacy; privacy is not inherited by its members or subsymbols. Qualified lookup additionally requires every canonical lexical prefix to be accessible, so lookup of `a::b::c` fails when `a::b` is private even if `a::b::c` is public. This is evaluated within one `lookup_query` and one path-aware accessibility request rather than recursively issuing lookup queries for each prefix. A private declaration still shadows an outer declaration with the same name; lookup reports the privacy error and does not continue searching.

Reopened non-overload declarations must resolve to identical privacy sets. Mixing public and private reopenings, or using different resolved sets, is an error. Duplicate entries within one `PRIVATE(...)` list are also an error.

Function and template groups are not filtered during overload resolution. The selected user overload is checked afterward. An inaccessible selected overload is an error; overload resolution is not rerun to find an accessible fallback. Compiler-provided overloads have no source privacy and remain public.

Field lowering performs the same selected-declaration access check before emitting access to a matching struct field. This covers field paths that do not pass through ordinary symbol lookup.

Public declarations may expose private types in their signatures. This design does not perform a private-type leak audit.

### Queries

The following queries are added:

- `active_subdeclaroids_query`
  - Input: the canonical `type_symbol` of a containing scope.
  - Output: `std::vector<subdeclaroid>` containing only declarations whose `INCLUDE_IF` evaluates true. Privacy remains attached to each declaration wrapper.
- `canonical_lookup_query`
  - Input: `contextual_type_reference { context, type }`.
  - Output: `std::optional<type_symbol>` containing the canonical symbol, without source access checking.
  - This owns the former recursive canonicalization behavior of `lookup_query` and is used by compiler-internal resolution, including named privacy scopes.
- `declaration_privacy_query`
  - Input: a canonical declaration `type_symbol`, a selected `temploid_reference`, or an `instanciation_reference`.
  - Output: `std::optional<resolved_privacy_scope>`. `std::nullopt` means public or a still-unselected overload group.
- `declaration_is_accessible_query`
  - Input: `declaration_access_request { accessor_context, selected_declaration, kind }`, where `kind` is `selected_declaration` or `lookup_path`.
  - Output: `bool`. `selected_declaration` checks only the selected declaration. `lookup_path` also checks each canonical lexical parent. Referenced signature types are not checked.

The following queries are modified:

- `declaroids_query`
  - Input remains a canonical declaration `type_symbol`.
  - Output remains `std::vector<declaroid>`.
  - It now consumes `active_subdeclaroids_query`, centralizing `INCLUDE_IF` evaluation while retaining privacy metadata for access queries.
- `lookup_query`
  - Input remains `contextual_type_reference { context, type }`.
  - Output remains `std::optional<type_symbol>`.
  - It now delegates canonicalization to `canonical_lookup_query`, then checks the canonical result with `declaration_is_accessible_query`.
- `instanciation_query`
  - Input remains `initialization_reference`; its existing optional `context` identifies the accessor for source-level selection.
  - Output remains `std::optional<instanciation_reference>`.
  - After overload selection, it checks the selected instantiation when an accessor context is present. An inaccessible selection raises a semantic compilation error without retrying selection.
