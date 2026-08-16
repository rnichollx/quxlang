# Inhertiance Plan (draft)

# Objectives & Design Considerations




This document proposes C++-style object-oriented inheritance for Quxlang. The feature is struct-based: only `STRUCT` declarations participate in inheritance. Quxlang's broader internal `class` terminology also includes enums, unions, variants, interfaces, generics, and other types, but those types are not inheritance nodes.

The scope includes:

- single, multiple, and virtual inheritance;
- inherited data members and member functions;
- virtual functions, pure virtual functions, final overrides, and abstract structs;
- phase-correct virtual dispatch during construction and destruction;
- RTTI sufficient for safe downcasts and cross-casts;
- pointer-only `AS DYNAMIC` casts;
- virtual destruction through a polymorphic base;
- defaulted and user-written construction, copy, move, assignment, swap, and destruction across base subobjects;
- construction failure cleanup and virtual-base ownership.

The design intentionally does not provide Itanium C++ ABI or Microsoft C++ ABI compatibility. Object layout, name mangling, runtime tables, and adjustment thunks are compiler-private Quxlang ABI details. This permits a smaller whole-program representation and avoids exposing target-specific C++ rules as language semantics.

The design priorities are, in order:

1. **Correct object and lifetime semantics.** Every nonvirtual base is a distinct subobject, every virtual base is shared once per complete object, and cleanup follows the actual construction order.
2. **Zero-overhead abstraction.** A struct that does not opt into polymorphism gets no RTTI pointer, dispatch table, dynamic lookup, or hidden polymorphic call argument. Static nonvirtual base conversions reduce to constant pointer adjustment. Virtual inheritance requires the explicit `VIRTUAL_POLYMORPHIC` object category and therefore may use its vtable pointer for navigation. Metadata constants are emitted only when a reached operation requires them.
3. **One-word pointers and references.** Inheritance must not turn ordinary `-> T` or `& T` values into fat pointers. Required dynamic context is stored in affected object subobjects.
4. **Efficient virtual operations.** A virtual call is a runtime-table load, slot load, and indirect call through a precomputed adjustment thunk. A virtual-base conversion is a table-offset load plus pointer adjustment. No runtime names, hashes, or graph traversal are required on the hot path.
5. **A Quxlang-native model.** Syntax follows existing member declarations, function suffixes, constructor delegates, and `AS` casts. The compiler extends generic AST, query, VMIR2, lifetime, constexpr, and backend paths rather than adding a separate inheritance-only pipeline.
6. **Determinism.** Base declaration ordinals, virtual-base order, virtual slot order, layout, RTTI records, and thunk emission must be stable for identical normalized input.
7. **Clean diagnostics.** Cycles, ambiguity, invalid overrides, abstract construction, duplicate virtual-base initializers, and unsafe destruction are diagnosed semantically before backend lowering.

"C++-style" in this plan refers to the object semantics, not every C++ surface feature. The initial feature has topology-only inheritance: there are no `public`, `protected`, or `private` inheritance modes. Existing Quxlang declaration privacy remains in force, and deriving from a struct does not grant access to its private members.

Polymorphism is an explicit struct property. A struct must contain either the `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` keyword before it can declare or override a virtual function, participate as the dynamic type in RTTI, or provide a polymorphic destruction target. Only `VIRTUAL_POLYMORPHIC` permits virtual bases and selects the split full-object/subobject construction ABI. This makes both the vtable pointer and the more complex virtual-inheritance lifetime contract visible at the type declaration.

The initial implementation also excludes covariant virtual returns, virtual function templates, virtual variadic packs, dynamic reference casts, `dynamic_cast<void *>`-style conversion, and a public RTTI reflection API. These can be added without changing the core object representation.

# Syntax

## Base declarations

A named direct nonvirtual base is declared as a member-like entry:

```quxlang
::widget STRUCT
{
  .control BASE control;
  .caption VAR string;
}
```

The grammar is:

```text
named-base-declaration := "." member-name "BASE" type ";"
virtual-base-declaration := "." member-name "VIRTUAL_BASE" type ";"
anonymous-base-declaration := ".BASE" type ";"
```

`.BASE BaseType;` is shorthand for one anonymous direct nonvirtual base:

```quxlang
::circle STRUCT POLYMORPHIC
{
  .BASE shape;
  .radius VAR I64;
}
```

The anonymous form is permitted only when the struct has exactly one direct base declaration. It cannot be virtual. Multiple inheritance requires names so that every repeated or ambiguous base subobject has a stable source selector.

A direct virtual base is always named:

```quxlang
::scrollable_control STRUCT VIRTUAL_POLYMORPHIC
{
  .control VIRTUAL_BASE control;
  .scroll_offset VAR I64;
}
```

The name selects the base subobject in expressions. It does not affect virtual-base identity: virtual bases with the same canonical `type_symbol` are coalesced even when different paths give them different names.

The following rules apply:

- a base type must resolve to a concrete `STRUCT` instantiation;
- an inheritance cycle is ill-formed;
- a struct marked `FINAL` cannot be used as a base;
- direct base selector names cannot collide with direct fields, functions, or other base selectors;
- repeated nonvirtual base types are allowed when every repeated direct edge is named;
- redeclaring the same virtual base type twice directly in one struct is an error;
- a struct that directly or indirectly derives from a polymorphic base must declare a compatible polymorphic category;
- a struct whose inheritance closure contains a virtual base, or that derives from a `VIRTUAL_POLYMORPHIC` base, must be declared `VIRTUAL_POLYMORPHIC`;
- `IPC_STRUCT` cannot declare a base or be used as a base in the initial implementation;
- base declarations cannot carry declaration privacy because inheritance topology is public.

## Polymorphic structs

`POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` are added to the existing struct keyword list. They are mutually exclusive object categories.

```quxlang
::shape STRUCT POLYMORPHIC
{
  .area FUNCTION() CONST VIRTUAL: I64
  {
    RETURN 0;
  }
}
```

`POLYMORPHIC` authorizes and requires polymorphic runtime identity but prohibits virtual bases anywhere in the inheritance closure. A reached object layout contains a vtable pointer even when the struct currently has no virtual slots; this permits RTTI and lets a derived polymorphic struct introduce slots without changing the base's declared object category. Whole-program reachability still controls whether associated table constants and unused thunks are emitted.

`VIRTUAL_POLYMORPHIC` includes all `POLYMORPHIC` capabilities and additionally permits virtual bases:

```quxlang
::shared_widget STRUCT VIRTUAL_POLYMORPHIC
{
  .control VIRTUAL_BASE control;
}
```

A struct that declares `VIRTUAL`, inherits a polymorphic base, is the source of `AS DYNAMIC`, or is destroyed polymorphically must declare one of these categories. A struct that declares a `VIRTUAL_BASE`, inherits any type whose closure contains a virtual base, or derives from a `VIRTUAL_POLYMORPHIC` base must declare `VIRTUAL_POLYMORPHIC`. A `POLYMORPHIC` derived type may remain `POLYMORPHIC` only when every base is plain or `POLYMORPHIC` and every base closure is free of virtual inheritance. Declaring a virtual function on an unmarked struct or declaring a virtual base on anything other than `VIRTUAL_POLYMORPHIC` is a semantic error.

The declared category selects the ABI even when a conditional virtual base is inactive or the struct currently declares no virtual base. A `VIRTUAL_POLYMORPHIC` struct therefore always has distinct full-object and subobject constructor functions. This is permitted so generic and conditionally assembled types do not change constructor ABI when their active base set changes.

## Final structs

`FINAL` is added to the existing struct keyword list:

```quxlang
::leaf_widget STRUCT FINAL
{
  .BASE widget;
}
```

Attempting to derive from `leaf_widget` is a semantic error. `FINAL` does not otherwise change layout.

## Inherited member access

Unqualified member access searches the derived struct and then its base graph:

```quxlang
VAR item circle;
item.radius := 8;
VAR center point := item.origin;
VAR area I64 := item.area();
```

Named bases are projection members and can disambiguate a repeated base:

```quxlang
::split_button STRUCT
{
  .primary BASE button;
  .secondary BASE button;
}

::read_primary FUNCTION(@value CONST& split_button): I32
{
  RETURN value.primary.command_id;
}
```

The projection preserves the base subobject's address and static type. A virtual call made on the projected value still dispatches virtually. To call a selected implementation nonvirtually, use the existing fully qualified submember form and supply `@THIS` explicitly. For example, inside a struct that declares `.shape_base BASE shape;`:

```quxlang
RETURN shape::.area(@THIS .shape_base);
```

Here `shape::.area` names the implementation declared by `shape`; it does not perform virtual dispatch.

## Virtual function modifiers

Virtual behavior is expressed by one suffix construct after a function's parameter list and before `ENABLE_IF` or the return type:

```text
virtual-specifier := "VIRTUAL" ["(" virtual-option-list ")"]
virtual-option-list := virtual-option {"," virtual-option}
virtual-option := "OVERRIDE" | "FINAL" | "PURE"
```

`VIRTUAL` shares the suffix position already used by `CONST`, `MUT`, `WRITE`, and `TEMP`. The options inside `VIRTUAL(...)` are unordered and cannot be repeated. `VIRTUAL()` is invalid. A function may contain at most one `THIS` qualifier and one virtual specifier.

```quxlang
::shape STRUCT POLYMORPHIC
{
  .area FUNCTION() CONST VIRTUAL: I64
  {
    RETURN 0;
  }

  .describe FUNCTION() CONST VIRTUAL(PURE): string;

  .DESTRUCTOR FUNCTION() VIRTUAL
  {
  }
}

::circle STRUCT POLYMORPHIC FINAL
{
  .BASE shape;
  .radius VAR I64;

  .area FUNCTION() CONST VIRTUAL(OVERRIDE, FINAL): I64
  {
    RETURN .radius * .radius * 3;
  }

  .describe FUNCTION() CONST VIRTUAL(OVERRIDE): string
  {
    RETURN "circle";
  }

  .DESTRUCTOR FUNCTION() VIRTUAL(OVERRIDE)
  {
  }
}
```

The forms mean:

- bare `VIRTUAL` introduces a new virtual slot. It is an error if the declaration instead matches an inherited slot; that case must use `VIRTUAL(OVERRIDE)`.
- `VIRTUAL(OVERRIDE)` requires the declaration to match at least one inherited virtual slot.
- `VIRTUAL(FINAL)` introduces a new final virtual slot.
- `VIRTUAL(OVERRIDE, FINAL)` overrides inherited slots and prevents a further override.
- `VIRTUAL(PURE)` introduces a pure slot without a body. A pure override uses `VIRTUAL(OVERRIDE, PURE)`. A pure declaration ends in `;`.

`FINAL` and `PURE` cannot be combined. Constructors cannot use a virtual specifier. A destructor may use `VIRTUAL`, `VIRTUAL(OVERRIDE)`, or either form with `FINAL`, but cannot be `PURE` in the initial implementation.

Virtual function templates, function packs, `ENABLE_IF`, and priority overloads are rejected initially. Their open-ended overload sets are incompatible with a closed runtime slot until a separate instantiation and reachability model is specified.

## Constructors and base delegates

Plain structs and `POLYMORPHIC` structs use `.CONSTRUCTOR`. Named direct bases use their selector in the delegate list; the sole anonymous base uses its type:

```quxlang
::labeled_button STRUCT
{
  .button BASE button;
  .label VAR string;

  .CONSTRUCTOR FUNCTION(%command I32, %text STRING_CONSTANT)
    :> .button:[command], .label:[text]
  {
  }
}

::circle STRUCT POLYMORPHIC
{
  .BASE shape;
  .radius VAR I64;

  .CONSTRUCTOR FUNCTION(%radius I64)
    :> shape(), .radius:[radius]
  {
  }
}
```

`VIRTUAL_POLYMORPHIC` structs have two user-declarable constructor forms:

- `.FULLOBJECT_CONSTRUCTOR` constructs a complete object and owns every reachable virtual base;
- `.SUBOBJECT_CONSTRUCTOR` constructs the type as a base subobject and never constructs a virtual base.

Every explicitly declared overload signature in one form must have a matching signature in the other form. The two declarations may have different delegate arguments and bodies. A full-object declaration may contain `VIRTUAL` delegates; a subobject declaration may not. Both declarations independently initialize the type's direct nonvirtual bases and fields, using default initialization for omitted delegates.

Ordinary source construction and `PLACE` select `.FULLOBJECT_CONSTRUCTOR`. A direct-base delegate selects `.SUBOBJECT_CONSTRUCTOR` when the selected base is `VIRTUAL_POLYMORPHIC`. Plain and `POLYMORPHIC` types use `.CONSTRUCTOR`; declaring either split form on those types is an error.

A `VIRTUAL_POLYMORPHIC` struct may declare `.CONSTRUCTOR` as shorthand for an explicit pair. For each shorthand overload, the compiler synthesizes:

- a full-object form containing all declared delegates and the declared body;
- a subobject form containing the same direct-base and field delegates and body, with every virtual-base delegate and its argument expressions removed before expression generation.

The shorthand creates two distinct semantic and ABI functions; it does not pass a runtime `most_derived` flag. A shorthand overload cannot coexist with an explicit full/subobject pair having the same normalized signature, although different signatures in one struct may choose different declaration styles. Compiler-generated implicit default/copy/move constructors on a `VIRTUAL_POLYMORPHIC` struct always synthesize the required pair.

A virtual-base delegate is introduced by `VIRTUAL` followed by the canonical base type:

```quxlang
::root STRUCT
{
  .value VAR I32;

  .CONSTRUCTOR FUNCTION(%value I32) :> .value:[value]
  {
  }
}

::left_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .root VIRTUAL_BASE root;

  .FULLOBJECT_CONSTRUCTOR FUNCTION(%value I32)
    :> VIRTUAL root:[value]
  {
  }

  .SUBOBJECT_CONSTRUCTOR FUNCTION(%value I32)
  {
  }
}

::right_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared_root VIRTUAL_BASE root;

  .FULLOBJECT_CONSTRUCTOR FUNCTION(%value I32)
    :> VIRTUAL root:[value]
  {
  }

  .SUBOBJECT_CONSTRUCTOR FUNCTION(%value I32)
  {
  }
}

::diamond STRUCT VIRTUAL_POLYMORPHIC
{
  .left BASE left_branch;
  .right BASE right_branch;

  .FULLOBJECT_CONSTRUCTOR FUNCTION(%value I32)
    :> VIRTUAL root:[value], .left:[value], .right:[value]
  {
  }

  .SUBOBJECT_CONSTRUCTOR FUNCTION(%value I32)
    :> .left:[value], .right:[value]
  {
  }
}
```

`diamond` contains one `root` subobject. Its full-object constructor evaluates its `VIRTUAL root` arguments and initializes that subobject once. Constructing `left_branch` or `right_branch` inside `diamond` selects its declared subobject constructor, which contains no virtual-base delegate. When either branch is constructed as a complete object, its full-object entry uses its own delegate.

The shorthand form is useful when both bodies and all nonvirtual delegates are intentionally identical:

```quxlang
::polymorphic_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .root VIRTUAL_BASE root;

  .CONSTRUCTOR FUNCTION(%value I32)
    :> VIRTUAL root:[value]
  {
  }
}
```

Here the synthesized full-object entry evaluates `value` for `root`. The synthesized subobject entry does not generate or evaluate that delegate expression.

Omitting a direct base, virtual base, or field delegate requests default initialization. A missing default constructor is a semantic error. Written delegate order does not change construction order.

## Dynamic casts

`DYNAMIC` is added to the existing `AS` modifier set:

```quxlang
VAR base_pointer -> shape := circle_pointer;
VAR circle_pointer_2 -> circle := base_pointer AS DYNAMIC -> circle;
```

The initial form accepts only instance pointers (`-> T` and their qualified forms):

- a null source produces null;
- a successful downcast or cross-cast produces a pointer to the unique target subobject;
- a failed or ambiguous cast produces null;
- the source's pointee type must be declared `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`;
- pointer class and qualifiers are preserved, and qualifiers cannot be discarded;
- `PARTIAL` remains an integer conversion modifier and is not used for RTTI casts.

Derived-to-base pointer and reference conversions are implicit when the destination base subobject is unique. They do not use `AS DYNAMIC`. No implicit object slicing conversion is added.

# Semantics

## Terminology and identity

A **complete object** is the object whose storage was supplied by `NEW&& T`, a local declaration, or `PLACE`. A **subobject** is a direct or indirect base or field within that complete object. The **most-derived type** is the complete object's struct type.

Subobject identity is not just a nominal type:

- a nonvirtual base is identified by its source-ordered direct-base ordinal path from its containing root;
- repeated nonvirtual bases therefore remain distinct even when their types match;
- a virtual base root is canonicalized by nominal `type_symbol` within the complete object;
- a nonvirtual descendant inside a virtual base is identified by the canonical virtual root plus its ordinal path.

These identities are used by layout, casts, RTTI, constructor delegates, dispatch thunks, diagnostics, and constexpr evaluation. Source member names are selectors, not identities.

## Hierarchy validity

Hierarchy normalization resolves all active base declarations before layout. It rejects:

- direct or indirect cycles;
- non-struct base types;
- derivation from `FINAL`;
- a virtual declaration in a struct not marked `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`;
- an unmarked struct that derives from a polymorphic base;
- a `POLYMORPHIC` struct that declares or inherits a virtual base, or derives from a `VIRTUAL_POLYMORPHIC` base, instead of using `VIRTUAL_POLYMORPHIC`;
- anonymous-base use outside the single-direct-base case;
- selector collisions;
- unsupported `IPC_STRUCT` participation;
- direct duplicate virtual edges.

Virtual-base canonicalization uses the fully canonical, instantiated base type. Two different template instantiations are different virtual bases.

`INCLUDE_IF` on a base declaration uses the existing active-declaration query path. If resolving a condition recursively depends on the hierarchy being computed, QueryGraph terminates the query with its existing `recursive_dependency_error`. No inheritance-specific detection or prerequisite work is required. A later diagnostics change may translate that exception into a more source-oriented hierarchy message without changing inheritance semantics.

## Member lookup

Lookup follows these rules:

1. Direct declarations and direct base selector names are searched first.
2. If the derived struct directly declares the requested member name, all inherited declarations of that name are hidden.
3. Otherwise, lookup recursively searches every direct base.
4. Reaching the same declaration through the same canonical virtual subobject is deduplicated.
5. Candidates reached through distinct nonvirtual subobjects remain distinct. More than one surviving declaration set is ambiguous, even if later overload resolution could choose between their signatures.
6. An explicit named base projection begins a new lookup from that selected subobject and resolves the ambiguity.

The lookup result contains the declaring symbol and an exact subobject path. Inherited declarations are not copied into the derived struct's declaration list. Field access first applies the path and then emits the existing direct-field operation. A member function binds `THIS` to the selected base projection.

Existing declaration privacy is checked on the final selected declaration using the original accessor context. Derivation grants no new privilege.

## Static conversions

An implicit derived-to-base pointer or reference conversion is valid when hierarchy normalization finds exactly one target subobject. A repeated nonvirtual base is ambiguous unless the source expression has already selected a named base. Conversion to a virtual base is valid and uses the complete object's context to obtain its runtime offset.

Value conversions do not slice. A distinct construction or conversion feature may add slicing later.

Pointer conversions preserve null. Reference conversions require a valid reference. Qualifier rules are the same as other pointer/reference conversions.

`CAST_PTRREF` remains an address-preserving representation conversion. It must never be used for an inheritance conversion because secondary and virtual bases can have different addresses.

## Virtual slots and overrides

A virtual slot key contains:

- the introducing virtual declaration identity;
- the function name;
- normalized non-`THIS` parameter types and passing modes;
- the `THIS` qualifier and reference category.

The formal signature keeps `THISTYPE`; an instantiated function uses its concrete owner type. The return type is validated separately and must match exactly in the initial implementation.

A `VIRTUAL(OVERRIDE)` declaration may override multiple compatible inherited slots, as happens when unrelated bases introduce the same virtual signature. The compiler emits one adjustment thunk per affected source subobject. If two branches provide different final overriders for the same inherited slot and the derived struct does not provide a unique override, the derived struct is ill-formed.

A final overrider marked with the `PURE` option leaves the slot unimplemented. A struct with any unimplemented final slot is abstract. Abstract structs can be base subobjects but cannot be complete objects, fields, values returned by value, or direct construction targets.

Generated destructors automatically override an inherited virtual destructor. A user-written destructor matching one must spell `VIRTUAL(OVERRIDE)`, like every other user-written override.

## Runtime object model

Pointers and references remain one machine word. Each `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` source subobject carries one hidden vtable pointer. Subobjects outside those categories carry no runtime header. Because every virtual-base owner is `VIRTUAL_POLYMORPHIC`, virtual-base navigation uses its existing vtable pointer rather than a separate navigation-only object category.

The vtable pointer addresses a compiler-private phase descriptor. Its fixed prefix identifies:

- the complete dynamic type, represented by the existing linked type-index ordinal;
- the current source subobject identity;
- a signed offset from that subobject to the complete object;
- the active construction or destruction phase;
- the applicable cast and virtual-base navigation records.

The descriptor continues with RTTI and virtual slot entries. A `VIRTUAL_POLYMORPHIC` descriptor additionally contains the virtual-base navigation records required by that source subobject. Slot entries are function pointers to adjustment thunks, not necessarily direct user routine addresses.

Descriptors are immutable private constants. Construction and destruction change the hidden vtable pointers in the object; they do not mutate global tables.

For layout efficiency, one eligible direct nonvirtual polymorphic base is selected as the primary base and reuses the derived root vtable slot at offset zero. Selection is deterministic: choose the first eligible base in declaration order whose header can represent the derived category's descriptor. Physical placement may put that primary base first while semantic construction order remains declaration order. If no base can supply the required root header, the derived struct gets its own.

## Object layout

`struct_layout` becomes the authoritative inheritance layout. `class_placement_info` remains the broad placement interface for all Quxlang classes and reads the complete size from `struct_layout` for structs.

Layout distinguishes:

- **nonvirtual extent**: the bytes required when the type is embedded as a nonvirtual base, excluding all of its virtual bases;
- **nonvirtual alignment**: the alignment required by that embedded nonvirtual extent;
- **complete size**: the bytes required when the type is a complete object, including each canonical virtual base once;
- **complete alignment**: the maximum required by the nonvirtual extent and all canonical virtual bases.

Physical layout proceeds as follows:

1. place the selected primary base at offset zero, or place the root runtime header if one is required;
2. place remaining direct nonvirtual bases, preserving declaration order among them;
3. place direct fields using the existing non-IPC field-ordering policy;
4. finalize the nonvirtual extent and nonvirtual alignment;
5. for a complete object, place canonical virtual bases in their semantic virtual-base order;
6. round the complete size to the complete alignment.

Direct base construction order is never inferred from physical offsets.

Empty nonvirtual bases use empty-base optimization where doing so preserves distinct addresses for distinct same-type subobjects. The first implementation does not reuse nonempty base tail padding; that optimization can be added without changing source semantics.

`ACCESS_FIELD` continues to address only a field declared directly by its static base type. Inherited field access is `STRUCT_CAST` followed by `ACCESS_FIELD`. This prevents field lists from being flattened and preserves source subobject identity.

## Construction

Construction extends the existing `NEW&& T`, `STRUCT_INIT_START`, delegate constructor, and `STRUCT_INIT_FINISH` lifecycle. It does not create a parallel object-state system.

For a complete object, semantic construction order is:

1. all reachable virtual bases, once, in depth-first left-to-right base-declaration order;
2. direct nonvirtual bases in declaration order;
3. direct fields in declaration order;
4. the constructor body.

Each base recursively constructs its own direct nonvirtual bases and fields. Written delegate order does not affect this sequence. A delegate argument expression is evaluated immediately before its selected subobject is initialized.

The full-object constructor receives or materializes the root complete-object context. Its virtual-base delegates select canonical complete-object virtual-base ordinals. Its direct-base delegates select direct-base declaration ordinals and receive exact child contexts.

The subobject constructor receives a compiler-private context for its exact subobject and never owns a virtual-base selector. An explicitly declared `.SUBOBJECT_CONSTRUCTOR` generates its own delegates and body. A shorthand or implicit constructor uses the synthesized subobject form, which removes virtual-base delegates before expression generation and shares the declared or generated body. In both cases it constructs direct nonvirtual bases and fields only.

Before executing a base constructor body, virtual calls and dynamic RTTI operations observe that base's construction phase. After a base constructor returns, the caller explicitly restores its own phase. The most-derived phase becomes active before its direct fields are initialized and remains active for its constructor body.

After a complete constructor body returns successfully, its caller installs the steady phase before publishing the constructed value. After a subobject constructor returns, its caller instead restores the enclosing constructor phase. Keeping this decision at the caller allows one ordinary `.CONSTRUCTOR` entry to serve both complete and base use when no virtual-base ownership split is required.

A virtual call to a pure slot in the active phase is a defined runtime failure. A dynamic cast during construction sees the active phase type, not a future most-derived phase.

Implicit default, copy, and move constructors synthesize the same split:

- a complete copy/move initializes every canonical virtual base once from the corresponding source subobject;
- a subobject copy/move skips virtual bases;
- direct nonvirtual bases are processed before fields;
- generated constructor selection uses `.FULLOBJECT_CONSTRUCTOR` or `.SUBOBJECT_CONSTRUCTOR` according to the requested role.

## Destruction and failure cleanup

Destruction is the exact reverse of successful construction. A complete-object destructor:

1. enters the most-derived destruction phase;
2. executes the most-derived destructor body;
3. destroys fields in reverse declaration order;
4. destroys direct nonvirtual bases in reverse declaration order, entering each base's phase;
5. destroys owned virtual bases once in reverse virtual-base construction order.

A subobject destructor performs steps 1 through 4 for its own subobject and never destroys a virtual base. The compiler generates distinct full-object and subobject destructor entries when virtual-base ownership requires them; the source continues to declare `.DESTRUCTOR` once.

If construction fails before `STRUCT_INIT_FINISH`, only delegates that reached active state are destroyed, in reverse semantic delegate order. If the constructor body fails after `STRUCT_INIT_FINISH`, completed fields and bases are destroyed, but the enclosing destructor body is not invoked because construction of that enclosing object never completed.

Virtual destruction through a base dispatches to a thunk that adjusts the base pointer to the complete object and invokes the most-derived full-object destructor. Destruction through a base whose destructor is not virtual is rejected when the compiler cannot prove that the base is the complete object. Destruction and deallocation remain separate operations; the destructor thunk does not select an allocator or release storage.

## RTTI and dynamic casts

RTTI is emitted only for reached `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` hierarchies and reached dynamic cast operations. It contains ordinals and signed offsets, not source names. Virtual-base navigation records are emitted only for reached `VIRTUAL_POLYMORPHIC` hierarchies.

For `source AS DYNAMIC -> Target`, runtime evaluation is:

1. return null if `source` is null;
2. load the source subobject's phase descriptor;
3. recover the active complete object and source subobject identity;
4. look up target subobjects for the canonical target type;
5. require one target that is valid from the source within the active hierarchy;
6. return the adjusted target address, or null if no unique target exists.

All inheritance edges are public, so access control does not alter this search. During ordinary object lifetime the active hierarchy is the complete most-derived hierarchy. During construction or destruction it is the current phase hierarchy.

# VMIR

VMIR2 gains semantic inheritance operations. Backends must not rediscover source hierarchy rules from byte offsets. Instruction operands use canonical types, subobject identities, paths, and slot keys produced by queries.

## Instructions added

### `STRUCT_CONTEXT_ROOT`

Proposed data:

```text
struct_context_root {
    complete_type: type_symbol,
    result: local_index
}
```

`result` has a compiler-private pointer-sized `struct_context_type`. The instruction materializes the root context descriptor for a complete object of `complete_type`. It outputs an active trivial value and does not access object storage. It is valid only in a full-object constructor, full-object destructor, or compiler-generated complete-object operation.

The context identifies the descriptor family from which exact direct-base and canonical virtual-base contexts are selected. There is deliberately no general `STRUCT_CONTEXT_CHILD` instruction; child context outputs are tied to verified `STRUCT_INIT_START` delegate selectors.

### `STRUCT_CAST`

Proposed data:

```text
struct_cast {
    source: local_index,
    result: local_index,
    path: struct_subobject_path
}
```

The source and result must be pointer or reference types. `path` identifies one statically proven target subobject. The instruction consumes `source` and outputs `result`; a caller that must retain a reference emits the existing reference-copy operation first.

For a nonvirtual path, lowering applies a constant signed offset. For a path containing a virtual edge, lowering loads the canonical virtual-base offset through the source runtime header. Pointer casts preserve null; reference casts require a valid reference.

### `STRUCT_DYNAMIC_CAST`

Proposed data:

```text
struct_dynamic_cast {
    source: local_index,
    target_type: type_symbol,
    result: local_index
}
```

The source and result must be instance pointers with compatible qualifiers. The source static pointee type must be declared `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`. The instruction consumes the source pointer and outputs a nullable pointer. Its semantic result follows the RTTI algorithm in the Semantics section.

### `STRUCT_TYPE_IS`

Proposed data:

```text
struct_type_is {
    source: local_index,
    target_type: type_symbol,
    result: local_index
}
```

This tests whether a nonnull pointer to a `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` struct has active dynamic type exactly equal to `target_type`; null produces false. It consumes `source` and outputs the `BOOL` local `result`. It supports compiler-generated RTTI fast paths and constexpr evaluation. No new source expression exposes it in the initial inheritance feature.

### `INVOKE_VIRTUAL`

Proposed data:

```text
invoke_virtual {
    slot: struct_virtual_slot_key,
    args: invocation_args
}
```

`args.named["THIS"]` is required and identifies the dispatching subobject. The state engine applies parameter and return lifetime effects from the slot's concrete signature, matching ordinary `INVOKE` behavior. Lowering loads the receiver descriptor, selects the slot, and calls its adjustment thunk through the existing indirect-call ABI.

The generator emits ordinary `INVOKE` when the selected call is explicitly qualified, the final overrider is statically known, or devirtualization proves one target. Otherwise it emits `INVOKE_VIRTUAL`.

### `STRUCT_PHASE_ENTER`

Proposed data:

```text
struct_phase_enter {
    object: local_index,
    context: local_index,
    phase_type: type_symbol,
    phase_kind: construction | destruction | steady
}
```

`object` is the current struct subobject and `context` is its exact compiler-private context. The instruction is readonly with respect to VMIR value lifetime. It installs the phase descriptors required by every initialized runtime-header-bearing subobject in the selected phase.

The generator emits it at constructor and destructor phase boundaries and emits a second instruction to restore the caller's phase after a base call. Cleanup blocks emit the phase transitions needed by the destructors they invoke.

## Instructions modified

### `STRUCT_INIT_START`

The current `invocation_args fields` operand is replaced by an ordered delegate list:

```text
struct_init_delegate {
    selector: field_ordinal | direct_base_ordinal | virtual_base_ordinal,
    value: local_index,
    context: optional<local_index>
}

struct_init_start {
    on_value: local_index,
    owner_context: optional<local_index>,
    delegates: vector<struct_init_delegate>
}
```

Field and direct-base ordinals are declaration ordinals for the current static struct. Virtual-base ordinals index the canonical list for the complete-object type and are legal only with a root owner context in a full-object constructor.

The instruction:

- transitions `on_value` from storage state to partial state;
- creates exact storage aliases for fields and base subobjects;
- outputs each requested base context slot;
- records delegates in semantic construction order;
- associates every delegate with `on_value` for reverse cleanup;
- validates that selector types match the declared field or base type.

Names no longer carry lifetime identity. This also removes the current restriction that constructor delegates are named fields only.

### `STRUCT_INIT_FINISH`

The operand remains `on_value`. Its semantics are extended to validate and retire ordered field/base delegate aliases, preserve the completed runtime headers, clear partial-construction ownership, and transition the owner to full state.

The cleanup engine retains enough compiled state to distinguish failure before finish from failure in the constructor body. Finishing does not authorize the enclosing destructor body after a constructor-body failure.

## Instructions retained unchanged

- `ACCESS_FIELD` remains a direct-field projection by the static struct layout.
- `CAST_PTRREF` remains address-preserving and is never an inheritance cast.
- `INVOKE` remains the direct-call operation.
- `INVOKE_INDIRECT` and the existing callable representation are reused by virtual-call lowering after slot selection.
- `MAKE_REFERENCE`, `COPY_REFERENCE`, `DESTROY`, and the `NEW&&`/`DESTROY&&` slot conventions keep their existing meanings.

## Instructions removed

No VMIR2 instruction needs to be removed. The `STRUCT_INIT_START` operand replacement is an intentional IR schema change, not a compatibility layer; all producers and consumers are updated together.

# Queries

Inheritance semantics are normalized by queries before VMIR generation. Query inputs use canonical semantic keys rather than parser nodes or source locations.

## Queries added

### `struct_direct_bases_query(type_symbol)`

Returns source-ordered active direct base declarations with canonical base types, selector names, declaration ordinals, and virtual/nonvirtual kind. It validates the local declaration rules but does not compute layout. Conditional declarations use the existing active-declaration queries, and any recursive condition dependency propagates QueryGraph's existing `recursive_dependency_error`.

### `struct_inheritance_info_query(type_symbol)`

Returns the canonical hierarchy graph:

- all source subobject identities and their types;
- direct edges and exact paths;
- canonical virtual-base roots and depth-first left-to-right order;
- uniqueness and ambiguity information for base conversions;
- whether the closure contains virtual inheritance;
- declared polymorphic category and category-compatible base validation;
- cycle and `FINAL` validation.

This is the shared semantic source for lookup, constructors, layout, RTTI, and diagnostics.

### `struct_member_lookup_query(struct_member_lookup_input)`

The input contains a canonical static struct type and member name. The result contains zero or more declaration symbols paired with exact receiver paths, plus a diagnosed ambiguity state. Base selector projections are represented distinctly from fields.

Dot access, bound member calls, and unqualified member access inside a member function use this query. Ordinary symbol identity queries continue to describe declarations actually owned by a type.

### `struct_conversion_query(struct_conversion_input)`

The input contains canonical source and destination pointee types and the requested static conversion category. The result either contains one `struct_subobject_path` or a reason the conversion is unavailable or ambiguous. It does not answer runtime downcast questions.

### `struct_virtual_slots_query(type_symbol)`

Returns virtual slots in deterministic order, including each introducing declaration, compatible inherited slots, final overrider, pure/final state, concrete callable signature, and required `THIS` adjustments. It diagnoses virtual declarations outside the two polymorphic categories, missing `VIRTUAL(OVERRIDE)`, illegal overrides of `VIRTUAL(FINAL)`, incompatible return types, and non-unique final overriders.

### `struct_constructor_forms_query(type_symbol)`

For a `VIRTUAL_POLYMORPHIC` struct, returns each normalized constructor signature with its full-object and subobject entries and an origin of `explicit_pair`, `constructor_shorthand`, or `compiler_generated`. It validates explicit pairing, prevents a shorthand/explicit collision for the same signature, rejects virtual-base delegates in a subobject declaration, and synthesizes the two semantic declarations for shorthand and implicit constructors. Other struct categories continue through the ordinary single-constructor queries.

### `struct_runtime_requirements_query(type_symbol)`

Returns layout-independent runtime requirements: which source subobjects need vtable headers, the struct's plain/`POLYMORPHIC`/`VIRTUAL_POLYMORPHIC` category, whether RTTI and virtual-base navigation records are needed, and which direct base is the primary-base candidate. Keeping this separate from final offsets prevents a query cycle between virtual-slot analysis and layout.

### `struct_runtime_info_query(type_symbol)`

Combines inheritance, virtual slots, concrete layout, and reached routines into backend data for one complete type. It contains phase descriptor groups, virtual-base offsets, cast records keyed by canonical type symbols, slot ordinals, and adjustment-thunk descriptions. Linked type-index ordinals are assigned later by output collection so this query does not depend on link-wide numbering.

### `struct_default_subobject_ctor_query(type_symbol)` and `struct_subobject_dtor_query(type_symbol)`

These parallel the existing complete-object `class_default_ctor_query` and `class_default_dtor_query` when a base delegate or cleanup edge needs the subobject entry. They return no entry for types that do not need a split, allowing the caller to use the ordinary constructor or destructor.

## Queries modified

- `struct_layout_query` consumes direct-base, inheritance, and runtime-requirement data and produces direct-base offsets, virtual-base offsets, nonvirtual extent/alignment, complete size/alignment, fields, and runtime-header placement.
- `class_placement_info_query` uses `struct_layout.complete_size` and `struct_layout.complete_align` for a complete struct. Direct-base placement uses `nonvirtual_size` and `nonvirtual_align` instead.
- `class_default_ctor_query` selects the full entry returned by `struct_constructor_forms_query` for `VIRTUAL_POLYMORPHIC` and `.CONSTRUCTOR` otherwise.
- `list_builtin_constructors_query` and constructor-call initialization expose only the full-object side to ordinary complete-object construction while preserving existing type-construction syntax.
- `class_default_dtor_query` selects the complete-object destructor entry when destruction ownership is split.
- `class_requires_gen_default_ctor_query`, copy/move constructor queries, assignment/swap queries, and default-destructor queries include base subobjects in addition to fields.
- `have_nontrivial_member_ctor_query`, `have_nontrivial_member_dtor_query`, triviality queries, and relocatability queries account for base operations and runtime-header initialization. A runtime header makes construction nontrivial but does not by itself make byte relocation unsafe.
- `functum_list_user_overload_declarations_query`, function declaration normalization, and instantiation preserve the grouped virtual specifier, retain both user-declared constructor forms, and expose synthesized constructor forms under their exact semantic functum names.
- expression member resolution and `co_generate_dot_access` consume `struct_member_lookup_query` results and emit receiver projections before direct field or function handling.
- `declaration_is_accessible_query` checks inherited declarations using the original access context after member lookup has selected the declaring symbol.
- `output_llvm_input_query`, `vmir_dependencies_query`, `functanoid_required_struct_layouts_query`, and `routine_requirements` collect runtime info, descriptors, thunks, and every type/routine reached through a virtual slot or RTTI cast.
- `constexpr_eval_query` and `run_static_test_query` request hierarchy and runtime information needed by inheritance instructions.
- generated serialization, comparison, assignment, swap, and antestatal checks traverse direct nonvirtual bases before direct fields and treat virtual bases according to complete-object ownership.

`struct_field_list_query` remains deliberately direct-field-only. `active_subdeclaroids_query`, `exists_query`, and `symbol_type_query` do not pretend that an inherited declaration is owned by the derived type; inherited access goes through the new member-resolution result.

## Queries removed

No existing query is removed. Existing queries retain their conceptual ownership boundaries, while their dependency lists and implementations are extended where complete-object behavior changes.

# Codegen

## Whole-program ABI data

`llvm_compilable_unit` gains a map of canonical complete struct types to `struct_runtime_info`. Reachability determines which descriptors and thunks are emitted. All symbols use private linkage unless a future explicit Quxlang ABI export feature says otherwise.

For each reached runtime-bearing complete type, LLVM codegen may emit:

- signed virtual-base offsets and navigation records for `VIRTUAL_POLYMORPHIC` types;
- for either polymorphic category, a compact subobject/cast table sorted by target type ordinal and subobject identity;
- one descriptor group for steady state and each observable construction/destruction phase;
- for either polymorphic category, virtual dispatch arrays whose fixed prefix references the descriptor data, including an empty slot array when the type has no virtual functions;
- signed offsets for direct adjustments, virtual bases, and complete-object recovery;
- one `THIS` adjustment thunk per distinct source-slot/final-overrider adjustment;
- a pure-slot failure target where a phase has no implementation.

No descriptor contains a source spelling. Existing linked type-index ordinals provide runtime type identity to both polymorphic categories. Only `VIRTUAL_POLYMORPHIC` descriptors contain virtual-base navigation records.

## Static base conversion

A nonvirtual `STRUCT_CAST` lowers to a byte-address GEP using the queried signed offset, followed by a cast back to the destination pointer representation. Pointer casts use a null-preserving select so a nonzero base offset cannot turn null into a nonnull address.

A virtual-base `STRUCT_CAST` loads the source runtime header, reads the canonical virtual-base offset, recovers the complete-object address, and computes the target. The result remains one word.

## Virtual calls

`INVOKE_VIRTUAL` lowering:

1. loads the dispatch descriptor from the receiver subobject;
2. loads the function pointer at the query-assigned slot ordinal;
3. passes the original receiver and ordinary runtime arguments through the existing callable ABI;
4. calls the thunk indirectly;
5. lets the thunk adjust `THIS` to the final overrider's declaring subobject and tail-call or directly call the instantiated function.

The optimizer may replace this with `INVOKE` when the receiver's dynamic type is proven, the function or struct is `FINAL`, or whole-program reachability yields one target. The semantic VMIR operation remains virtual until that proof is available.

## Dynamic casts and RTTI tests

`STRUCT_TYPE_IS` is an ordinal comparison after loading the phase descriptor. `STRUCT_DYNAMIC_CAST` uses the descriptor's sorted cast records. Small fixed hierarchies may lower to direct comparisons; larger sets may use binary search over private constants. Runtime strings and allocation are forbidden.

The cast record maps a source subobject identity and target type ordinal to zero, one, or ambiguous candidate offsets. Codegen returns null for zero or ambiguous results.

## Construction and destruction phases

`STRUCT_CONTEXT_ROOT` usually lowers to the address of a private descriptor family. `STRUCT_INIT_START` computes field/base storage aliases from `struct_layout` and materializes exact child context constants selected by its ordinals.

`STRUCT_PHASE_ENTER` emits hidden header stores. It may be removed when the queried phase has no observable header change. Phase descriptors are static constants, so phase entry performs no allocation.

Constructor and destructor routines receive an extra compiler-private context argument only when their static type's runtime requirements need it. Ordinary structs retain their current callable ABI. Full-object entries materialize the root context; subobject entries receive it from the caller. Explicit constructor pairs lower independently. A `.CONSTRUCTOR` shorthand is cloned into full/subobject semantic routines before VMIR generation, so backend code never branches on the construction role.

Virtual destructor slots point to thunks that recover the complete object and call the full-object destructor entry. They do not deallocate storage.

## Constexpr

The constexpr interpreter represents a struct as a canonical subobject graph instead of a flat direct-field map when inheritance is present. It tracks:

- distinct nonvirtual identities;
- canonical virtual-base identities;
- direct fields for each subobject;
- the active construction/destruction phase;
- the complete-object context.

Static casts, virtual calls, exact type tests, and dynamic casts are constexpr-capable when all reached functions are constexpr-evaluable. No host address or native byte offset is exposed; the interpreter follows semantic subobject identities. Cleanup uses the same ordered VMIR delegate records as native codegen.


# Detailed implementation

## Public compiler data model

The following data changes are required. Names are proposed C++ interface names and should receive Doxygen comments when implemented.

1. Extend `subdeclaroid` with `ast2_base_declaration`, containing `optional<string> selector_name`, `type_symbol base_type`, `inheritance_kind kind`, `include_if`, documentation, and source location. Keeping bases in `ast2_struct_declaration.declarations` preserves source order and conditional-declaration behavior.
2. Add `inheritance_kind { nonvirtual, virtual_ }`.
3. Add `optional<ast2_virtual_specifier> virtual_specifier` to `ast2_function_header`. Presence represents `VIRTUAL`; the record contains `is_override`, `is_final`, and `is_pure` for the parenthesized options.
4. Add a constructor-delegate kind to `ast2_function_delegate` and normalized `function_delegate`, distinguishing ordinary member/direct-base selection from a nominal virtual-base target.
5. Add `struct_polymorphism_kind { none, polymorphic, virtual_polymorphic }` and canonical semantic data types: `struct_base_declaration`, `struct_subobject_id`, `struct_subobject_path`, `struct_inheritance_info`, `struct_member_lookup_result`, `struct_virtual_slot_key`, `struct_virtual_slot`, `struct_runtime_requirements`, and `struct_runtime_info`.
6. Add `constructor_form_origin { explicit_pair, constructor_shorthand, compiler_generated }` and `struct_constructor_form` with normalized signature, full-entry symbol, subobject-entry symbol, and origin. `struct_constructor_forms` maps normalized signatures to those records.
7. Add VMIR's compiler-private `struct_context_type` and ordered `struct_init_delegate` selector variant.
8. Add `struct_base_layout_info` with subobject identity, selector, canonical type, declaration ordinal, and signed offset; add `struct_virtual_base_layout_info` with canonical identity, type, virtual ordinal, and signed offset; and add `struct_runtime_header_layout` with offset, polymorphism category, and optional primary-base identity.
9. Expand `struct_layout` with `direct_bases`, `virtual_bases`, optional runtime-header placement, `nonvirtual_size`, `nonvirtual_align`, `complete_size`, and `complete_align`. Remove the old ambiguous `size` and `align` fields. `fields` remains the list of direct fields.
10. Extend `llvm_compilable_unit` and dependency records with reached `struct_runtime_info` and adjustment-thunk requirements.

These are deliberate schema changes. Every producer and consumer should move in one change series; no old-field compatibility alias should remain.

## Parser and AST normalization

1. Add `BASE`, `VIRTUAL_BASE`, `SUBOBJECT_CONSTRUCTOR`, and `FULLOBJECT_CONSTRUCTOR` to the appropriate keyword sets, and add `POLYMORPHIC`, `VIRTUAL_POLYMORPHIC`, and `FINAL` to struct keywords.
2. Extend struct-body declaration parsing before the generic `.name` path so `.BASE Type;` is recognized without treating `BASE` as a member name.
3. Parse `.name BASE Type;` and `.name VIRTUAL_BASE Type;` into ordered base declarations. Reuse `INCLUDE_IF` handling so inactive edges do not enter the hierarchy.
4. Replace the one-shot `THIS` suffix parser in `try_parse_function_declaration.hpp` with a suffix loop that accepts one `THIS` qualifier and one `VIRTUAL` specifier. Parse its optional comma-separated `OVERRIDE`, `FINAL`, and `PURE` options as a group. Validate duplicates, empty parentheses, and `FINAL`/`PURE` conflict there; hierarchy-dependent checks belong in queries.
5. Allow a `VIRTUAL(PURE)` function to end in `;` and store an empty body that code generation never instantiates.
6. Extend `try_parse_function_delegates.hpp` to recognize `VIRTUAL type` before the existing callsite argument parser.
7. Add `DYNAMIC` to the `AS` modifier parser and intercept it in expression generation before constructor-based conversion selection.
8. Update AST metadata, comparison, hashing, serialization, debug printing, and parser tests for every new field and variant alternative.
9. Audit every `subdeclaroid` visitor. Base entries are consumed by hierarchy queries, while ordinary declaration, overload, symbol-kind, and field queries skip them rather than treating them as global declarations.
10. Keep `.CONSTRUCTOR`, `.FULLOBJECT_CONSTRUCTOR`, and `.SUBOBJECT_CONSTRUCTOR` as distinct parsed member names. Constructor-form synthesis occurs in queries, not in the parser.
11. Update constructor classification in formal-signature normalization, instantiation, and generation so all three names receive the `NEW&& THISTYPE` `THIS` convention and existing constructor-specific validation.

## Hierarchy, lookup, and override analysis

1. Implement direct-base normalization and recursive hierarchy construction before touching layout.
2. Read the polymorphism category through the existing struct keyword/tag path. Reject virtual declarations on an unmarked struct, reject `VIRTUAL_BASE` outside `VIRTUAL_POLYMORPHIC`, require every struct deriving from a polymorphic base to declare a compatible category, and require `VIRTUAL_POLYMORPHIC` whenever a base closure contains virtual inheritance.
3. Assign declaration ordinals before filtering physical layout; diagnostics and semantic order must not depend on alignment sorting.
4. Build canonical virtual roots and stable subobject identities. Cache conversion paths and ambiguity results in `struct_inheritance_info` rather than repeatedly walking the AST.
5. Route dot access and bound member calls through `struct_member_lookup_query`. The returned receiver path is carried through overload selection so the chosen callable and adjusted `THIS` cannot become separated.
6. Build virtual slots from normalized concrete signatures. Preserve an introduced slot across descendants, compute final overriders, and generate one thunk requirement for each source-subobject adjustment.
7. Normalize constructor forms per signature. Retain explicit full/subobject declarations, synthesize both entries from an allowed `.CONSTRUCTOR`, and reject missing pairs or signature collisions.
8. Keep constructor and destructor names out of ordinary inherited member lookup. They are selected only by lifecycle role.
9. Diagnose abstract complete-object use at the query that chooses construction or by-value placement, not only during LLVM emission.

## Layout and runtime descriptors

1. Refactor `struct_layout.cpp` into semantic placement of headers/bases/fields/virtual bases. Continue using `class_placement_info_query` for field types, but use base layouts' nonvirtual extents and alignments for base placement.
2. Select the primary base from runtime requirements before assigning offsets, preserving compatibility between the root polymorphism category and the candidate base's vtable header.
3. Implement empty-base optimization with an occupied-address set keyed by canonical subobject type and offset.
4. Produce complete size and nonvirtual extent in one query result so consumers cannot accidentally use complete size for a base.
5. Build runtime descriptor records only after offsets and final overriders are known. Deduplicate identical phase tables and adjustment thunks.
6. Reject inherited types at external ABI boundaries until an explicit external layout convention is selected.

## VMIR generation and lifetime state

1. Add the new instructions and metadata variants in `vmir2.hpp`, the textual assembler/parser, debug rendering, and equality metadata.
2. Replace `slot_state.delegates`' name-based `invocation_args` with ordered semantic delegate records. Record selector, role, activation stage, cleanup entry, and owning object.
3. Extend `state_engine.hpp` so base and field aliases use the same partial/full/dead state transitions. Do not create a separate base-lifetime stack.
4. Refactor `co_generate_struct_ctor_delegates` to request inheritance info, create all delegate slots in semantic order, emit one `STRUCT_INIT_START`, and then generate each selected initializer in that order.
5. Generate each explicitly declared full/subobject constructor from its own normalized declaration. For shorthand and implicit forms, generate both routines from the synthesized declarations and suppress virtual delegate expression generation entirely in the synthesized subobject routine.
6. Extend generated default/copy/move/assignment/swap routines to process bases. Assignment and swap operate on each canonical virtual base once for complete objects and then on direct nonvirtual bases and fields.
7. Generate full/subobject destructor entries and phase transitions. Teach all cleanup exits to select the correct entry from the delegate role.
8. Generate `STRUCT_CAST` for inherited field access, inherited receiver binding, implicit base conversions, and exact base selection.
9. Generate `INVOKE_VIRTUAL` only after ordinary overload resolution chooses a virtual slot. Explicit `Type::.member` calls remain direct.
10. Generate dynamic cast and exact type-test instructions with canonical target types.

## VMIR consumers and dependency collection

Every VMIR visitor must handle the new instruction variants and changed init record:

- `vmir2/state_engine.hpp` validates lifetimes and cleanup;
- `vmir2/assembler.hpp` and `sources/vmir2/assembler.cpp` provide stable textual forms;
- `sources/vmir2/routine_requirements.cpp` collects descriptor, layout, type-index, thunk, and final-overrider dependencies;
- `sources/queries/vmir_dependencies.cpp` propagates those dependencies;
- `sources/ir2_constexpr_interpreter.cpp` implements semantic subobject behavior;
- `sources/llvm-backend.cpp` implements native layout operations and descriptors;
- `sources/cortado-backend.cpp` emits the initial capability diagnostic;
- `sources/queries/output_llvm_input.cpp` closes descriptor and thunk reachability.

Query specs, dependency typelists, query registration shards, and generator dependency lists must be updated in the same series. A new query is not complete until its spec dependencies and compiler querygraph registration are present.

## Generated operations and type traits

Audit every operation that currently iterates only `struct_field_list`:

- default/copy/move construction;
- destruction and failure cleanup;
- copy/move assignment;
- swap;
- equality and three-way comparison generation;
- serialization and deserialization;
- trivial construction/destruction/relocation queries;
- `ANTESTATAL`, `SERIALOID`, `STRINGLIKE`, and related struct-tag validation;
- dependency discovery for every generated routine.

For operations on a complete object, canonical virtual bases are processed once before direct nonvirtual bases and fields. For a base-subobject operation, virtual bases are skipped. If an existing generated operation has no notion of complete versus subobject ownership, add that role explicitly at its highest caller rather than branching on a type name inside field iteration.

## Diagnostics

Diagnostics should name source selector paths rather than byte offsets. At minimum, report:

- the cycle path for inheritance cycles;
- every candidate path for an ambiguous inherited member or base conversion;
- the introducing declaration and attempted declaration for override failures;
- competing final overriders;
- the unimplemented slots that make a struct abstract;
- a `VIRTUAL` declaration outside `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`;
- a missing or incompatible polymorphism category on a struct derived from a polymorphic base;
- a `VIRTUAL_BASE` declaration or inherited virtual base without `VIRTUAL_POLYMORPHIC`;
- the missing default constructor for an omitted base delegate;
- duplicate, unreachable, or non-owned virtual-base delegates;
- an unpaired `.FULLOBJECT_CONSTRUCTOR` or `.SUBOBJECT_CONSTRUCTOR` signature;
- a virtual-base delegate in `.SUBOBJECT_CONSTRUCTOR`;
- a `.CONSTRUCTOR` shorthand collision with an explicit pair;
- split constructors on a type that is not `VIRTUAL_POLYMORPHIC`;
- unsafe destruction through a nonvirtual base destructor;

## Test and validation strategy

Prefer Quxlang `.qxs` `STATIC_TEST` or `DUAL_TEST` coverage. Add GoogleTest coverage only for parser or query structures that cannot be exercised through a Quxlang fixture.

Coverage should include:

- zero-offset and nonzero-offset single inheritance;
- named multiple inheritance and explicit disambiguation;
- repeated nonvirtual base types and ambiguous conversions;
- one canonical virtual base in a diamond;
- distinct virtual bases in multiple branches;
- virtual-base construction/destruction order;
- proof that ignored subobject virtual-base argument expressions are not evaluated;
- cleanup after failure at each base, field, and body stage;
- virtual dispatch in steady, construction, and destruction phases;
- `POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` opt-in, category propagation, invalid virtual-base categories, multiple-slot override, `VIRTUAL(OVERRIDE)`, `VIRTUAL(FINAL)`, `VIRTUAL(PURE)`, and abstract construction diagnostics;
- explicit full/subobject constructor pairs, mismatched pairs, and `.CONSTRUCTOR` synthesis;
- implicit upcasts, virtual-base upcasts, nullable downcasts, cross-casts, ambiguity, and null preservation;
- virtual destruction from each base subobject;
- generated default/copy/move/assignment/swap behavior;
- constexpr casts and calls;
- preservation of metadata-free codegen for plain nonvirtual inheritance.

Implementation validation should use the repository's targeted compiler/testmodule scripts while iterating. After code review, run `cbuild test -c Release` once for the complete series. Inspect emitted LLVM IR or object code for representative cases to confirm constant-offset upcasts, table-based virtual-base adjustment, one-word pointers, devirtualization, and absence of runtime metadata for a plain nonvirtual hierarchy.

## Suggested implementation order

1. AST, parser, canonical base data, and invalid-hierarchy diagnostics.
2. Member lookup, static conversion paths, virtual slot analysis, and semantic tests.
3. `struct_layout` nonvirtual/complete extents and runtime-header placement.
4. Ordered `STRUCT_INIT_START`, context creation, constructor pairs, cleanup, and destructor pairs.
5. Static inheritance casts and inherited field/function receiver generation.
6. Virtual invoke, phase descriptors, RTTI, dynamic casts, and virtual destruction.
7. Constexpr support and all VMIR dependency/output plumbing.
8. Generated operations, traits, serialization-related audits, and optimization checks.

Each stage should leave the query graph and VMIR variants exhaustive and buildable. Syntax should not be enabled for a semantic operation until every reached backend either implements it or emits a deliberate target diagnostic.

# Alternatives considered

## Reuse an established C++ ABI

**Pros:** interoperability with C++ object files, mature layout rules, existing debugger knowledge.

**Cons:** substantial platform divergence, construction-vtable complexity, ABI-stability commitments, exception/runtime dependencies, and constraints unrelated to Quxlang. It also conflicts with whole-program optimization opportunities.

**Decision:** rejected. Use a Quxlang-native compiler-private ABI.

## Fat base pointers

Store both the subobject address and complete-object context in every inherited pointer.

**Pros:** simple dynamic navigation and no runtime header in some objects.

**Cons:** changes pointer size and calling conventions, penalizes every pointer operation, complicates atomics and FFI, and violates the one-word pointer objective.

**Decision:** rejected.

## One runtime header only at the complete-object root

**Pros:** minimum metadata words per complete object.

**Cons:** a pointer to a secondary or virtual base cannot find the root header without already knowing the dynamic complete type or carrying a second word. Virtual dispatch from secondary bases becomes a runtime search.

**Decision:** rejected. Runtime-bearing source subobjects need locally reachable headers, with primary-base reuse where possible.

## Flatten inherited fields into the derived field list

**Pros:** makes simple field access resemble current direct fields.

**Cons:** destroys repeated-base identity, obscures ambiguity, duplicates virtual bases, entangles lookup with layout, and breaks constructor/destructor ownership.

**Decision:** rejected. Fields stay direct; inherited access projects to a base first.

## Treat inheritance as composition plus interfaces

**Pros:** little new layout machinery and explicit delegation.

**Cons:** does not provide substitutability, shared virtual bases, inherited lookup, base pointer adjustment, phase dispatch, or C++-style destruction.

**Decision:** rejected as the language inheritance feature. Composition and Quxlang interfaces remain useful independent tools.

## Use `CAST_PTRREF` for upcasts

**Pros:** no new VMIR cast instruction.

**Cons:** `CAST_PTRREF` deliberately preserves the address, while secondary and virtual base conversions require adjustment. Overloading it would erase a valuable IR invariant.

**Decision:** rejected. Use `STRUCT_CAST`.

## Runtime name or hash lookup

**Pros:** straightforward metadata construction and convenient debugging.

**Cons:** larger binaries, collision policy, variable runtime cost, and source-name ABI leakage.

**Decision:** rejected. Use linked type ordinals and dense/sorted constant records.

## Always synthesize the subobject constructor

**Pros:** one source body and no explicit pairing requirement.

**Cons:** prevents a type from intentionally using different direct-base arguments or constructor bodies when it is complete versus embedded, and hides a semantically important callable boundary.

**Decision:** rejected as the only model. `VIRTUAL_POLYMORPHIC` users may declare both `.FULLOBJECT_CONSTRUCTOR` and `.SUBOBJECT_CONSTRUCTOR`, or may choose `.CONSTRUCTOR` synthesis when one declaration is preferable.

## Keep `.CONSTRUCTOR` and pass a hidden `most_derived` boolean

**Pros:** fewer semantic function names.

**Cons:** virtual-base ownership becomes a runtime branch inside every affected constructor, ignored arguments are harder to suppress before evaluation, and the callable contract does not express its role.

**Decision:** rejected. Even the `.CONSTRUCTOR` shorthand creates distinct `.FULLOBJECT_CONSTRUCTOR` and `.SUBOBJECT_CONSTRUCTOR` entries at compile time.

## Expose C++ colon syntax or general attributes

**Pros:** familiar to C++ users or broadly extensible.

**Cons:** conflicts with existing Quxlang declaration structure and introduces an attribute system solely for this feature.

**Decision:** rejected. Use member-like `BASE` declarations and function suffix modifiers.

## Infer the polymorphism category from declarations

**Pros:** fewer struct keywords and behavior similar to C++.

**Cons:** the object representation changes because of a member or base encountered inside the body, a derived type can acquire a split constructor ABI implicitly, and declarations do not visibly state that they support RTTI, polymorphic destruction, or virtual-base ownership.

**Decision:** rejected. Require `STRUCT POLYMORPHIC` for non-virtual polymorphism and `STRUCT VIRTUAL_POLYMORPHIC` for the virtual-inheritance category. Diagnose virtual declarations, polymorphic base edges, and virtual base edges that lack a compatible explicit category.

## Defer virtual inheritance, RTTI, or virtual destruction

**Pros:** a smaller first patch.

**Cons:** risks choosing layout, constructor, and VMIR abstractions that cannot represent the complete object model. Retrofitting phase context and virtual-base ownership would invalidate early decisions.

**Decision:** rejected as a design strategy. Implementation may be staged, but the data model and lifecycle must account for the full scope from the start.

# Blockers / Open Questions

The following choices should be resolved before the affected implementation stage. They do not block parser and hierarchy-query work unless stated otherwise.

1. **Dynamic destruction source operation.** The exact integration of a polymorphic base pointer with current `DESTROY AT(storage) Type` syntax needs a source-level decision. The recommended rule is that the supplied storage continues to own deallocation, while a virtual destructor adjusts to and destroys the complete object. This must be settled before virtual destruction is enabled.
2. **Covariant returns.** The initial rule requires exact return types. Supporting covariant pointer/reference returns requires a second return-adjustment thunk and additional slot validation. Recommendation: keep exact returns for the first implementation.
3. **Empty-base ABI details.** Empty-base optimization is part of the target design, but distinct-address rules for repeated empty bases and zero-sized Quxlang types need precise tests. Recommendation: implement EBO, prohibit nonempty tail-padding reuse initially, and record all occupied `(type, offset)` pairs.
4. **External ABI boundaries.** Passing an inherited struct to `EXTERN` code by value or as a nominal pointer would expose the private layout. Recommendation: reject it until an explicit Quxlang external object ABI is designed; raw address conversion remains a separate unsafe operation.
5. **Public RTTI API.** `STRUCT_TYPE_IS` and type ordinals are initially compiler-internal. A future source feature must decide whether users can ask for exact dynamic type, enumerate bases, or obtain a stable type token. No public API should expose descriptor addresses.
6. **Binary ABI versioning and mangling.** The paired constructor/destructor entry names, context parameter, thunk symbols, and descriptor symbols need a mangling/version convention before inherited routines can be exported from separately compiled Quxlang modules.
7. **Pure virtual failure policy.** The plan specifies a defined runtime failure during an active phase. The implementation must select the existing panic path and diagnostic payload without adding a C++-style undefined-behavior rule.
8. **Serialization and comparison order as public semantics.** This plan proposes virtual bases once, then direct nonvirtual bases, then fields. That order should be confirmed before existing generated `SERIALOID`, equality, and comparison behavior is extended, because serialized formats may become externally observable.
