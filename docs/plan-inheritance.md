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
2. **Zero-overhead abstraction.** A struct that does not opt into polymorphism gets no RTTI pointer, dispatch table, dynamic lookup, or hidden polymorphic call argument. Static nonvirtual base conversions reduce to constant pointer adjustment. Virtual inheritance requires the explicit `VIRTUAL_POLYMORPHIC` object category and therefore may use its vtable pointer for navigation. A nonvirtual `DELETE` performs no dynamic-type check or allocation-info lookup. Metadata constants are emitted only when a reached operation requires them.
3. **One-word pointers and references.** Inheritance must not turn ordinary `-> T` or `& T` values into fat pointers. Required dynamic context is stored in affected object subobjects.
4. **Efficient virtual operations.** A virtual call is a runtime-table load, slot load, and indirect call through a precomputed adjustment thunk. A virtual-base conversion is a table-offset load plus pointer adjustment. No runtime names, hashes, or graph traversal are required on the hot path.
5. **A Quxlang-native model.** Syntax follows existing member declarations, function suffixes, constructor delegates, and `AS` casts. The compiler extends generic AST, query, VMIR2, lifetime, constexpr, and backend paths rather than adding a separate inheritance-only pipeline.
6. **Determinism.** Base declaration ordinals, virtual-base order, virtual slot order, layout, RTTI records, and thunk emission must be stable for identical normalized input.
7. **Clean diagnostics.** Cycles, ambiguity, invalid overrides, abstract construction, and duplicate virtual-base initializers, are diagnosed semantically before backend lowering. Dynamic misuse of nonvirtual `DELETE` is governed by an explicit undefined-behavior precondition rather than a runtime check.

"C++-style" in this plan refers to the object semantics, not every C++ surface feature. The initial feature has topology-only inheritance: there are no `public`, `protected`, or `private` inheritance modes. Existing Quxlang declaration privacy remains in force, and deriving from a struct does not grant access to its private members.

Polymorphism is an explicit struct property. A struct must contain either the `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` keyword before it can declare or override a virtual function, participate as the dynamic type in RTTI, or provide a polymorphic destruction target. Both categories implicitly provide a virtual destructor unless an explicit `.DESTRUCTOR` declaration carries the `NONVIRTUAL` tag. Only `VIRTUAL_POLYMORPHIC` permits virtual bases and selects the split full-object/subobject construction ABI. This makes the vtable pointer and more complex virtual-inheritance lifetime contract visible at the type declaration while keeping the destructor policy on the destructor itself.

A struct in either polymorphic category is not implicitly a `DATATYPE`. The compiler therefore does not synthesize equality, three-way comparison, serialization, deserialization, or swap for it. Copy and move construction and assignment remain ordinary statically selected operations; inheritance does not make them virtual or add copy-related virtual slots. A `VIRTUAL_POLYMORPHIC` copy or move constructor still has full-object and subobject entries solely to assign virtual-base construction ownership.

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

`POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` are added to the existing struct keyword list. They are mutually exclusive object categories. A generated destructor in either category is virtual. A user-written destructor is also virtual unless its function header carries `NONVIRTUAL`.

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


A struct that declares `VIRTUAL`, inherits a polymorphic base, is the source of `AS DYNAMIC`, or is destroyed polymorphically must declare one of these categories. A struct that declares a `VIRTUAL_BASE`, inherits any type whose closure contains a virtual base, or derives from a `VIRTUAL_POLYMORPHIC` base must declare `VIRTUAL_POLYMORPHIC`. A `POLYMORPHIC` derived type may remain `POLYMORPHIC` only when every base is plain or `POLYMORPHIC` and every base closure is free of virtual inheritance. Declaring a virtual function on an unmarked struct, attaching `NONVIRTUAL` to anything other than a destructor of a polymorphic struct, or declaring a virtual base on anything other than `VIRTUAL_POLYMORPHIC` is a semantic error.

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
destructor-virtuality-tag := "NONVIRTUAL"
```

`VIRTUAL` and destructor-only `NONVIRTUAL` share the suffix position already used by `CONST`, `MUT`, `WRITE`, and `TEMP`. The options inside `VIRTUAL(...)` are unordered and cannot be repeated. `VIRTUAL()` is invalid. A function may contain at most one `THIS` qualifier, one virtual specifier, and one `NONVIRTUAL` tag.

```quxlang
::shape STRUCT POLYMORPHIC
{
  .area FUNCTION() CONST VIRTUAL: I64
  {
    RETURN 0;
  }

  .describe FUNCTION() CONST VIRTUAL(PURE): string;

  .DESTRUCTOR FUNCTION()
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

  .DESTRUCTOR FUNCTION()
  {
  }
}
```

A polymorphic destructor is virtual without an explicit `VIRTUAL` specifier. `NONVIRTUAL` opts out on the destructor declaration itself:

```quxlang
::closed_shape STRUCT POLYMORPHIC FINAL
{
  .DESTRUCTOR FUNCTION() NONVIRTUAL
  {
  }
}
```

Generated destructors have no `NONVIRTUAL` tag and are therefore virtual for both polymorphic categories. A type that needs a generated-equivalent but nonvirtual destructor declares an empty tagged destructor body; compiler-generated base and field cleanup still surrounds that body normally. A derived destructor cannot carry `NONVIRTUAL` when any base contributes a virtual destructor slot. If every base destructor is nonvirtual, an untagged derived destructor may introduce the hierarchy's first virtual destructor slot.

The forms mean:

- bare `VIRTUAL` introduces a new virtual slot. It is an error if the declaration instead matches an inherited slot; that case must use `VIRTUAL(OVERRIDE)`.
- `VIRTUAL(OVERRIDE)` requires the declaration to match at least one inherited virtual slot.
- `VIRTUAL(FINAL)` introduces a new final virtual slot.
- `VIRTUAL(OVERRIDE, FINAL)` overrides inherited slots and prevents a further override.
- `VIRTUAL(PURE)` introduces a pure slot without a body. A pure override uses `VIRTUAL(OVERRIDE, PURE)`. A pure declaration ends in `;`.

`FINAL` and `PURE` cannot be combined. Constructors cannot use a virtual specifier. Destructor virtuality normally follows the containing struct: an untagged destructor of a polymorphic struct implicitly introduces or overrides the destructor slot, while `NONVIRTUAL` suppresses it. A destructor may still spell `VIRTUAL`, `VIRTUAL(OVERRIDE)`, or either form with `FINAL` to state or constrain the implicit behavior explicitly, but cannot be `PURE` in the initial implementation. `VIRTUAL` and `NONVIRTUAL` are mutually exclusive.

An ordinary virtual function must spell one concrete `THIS` qualifier: `CONST`, `MUT`, `WRITE`, or `TEMP`. The omitted `AUTO` default and the `INPUT` and `OUTPUT` qualifier templates are rejected because one runtime slot denotes one concrete callable ABI.

Virtual declarations use `ENABLE_IF` like other function declarations. The condition is evaluated for the concrete declaration before virtual-slot introduction, override matching, purity, or other resolution mechanics; a false declaration contributes no slot and cannot override one. Virtual function templates, function packs, default arguments, and priority overloads remain rejected initially because their open-ended overload sets are incompatible with a closed runtime slot until a separate instantiation and reachability model is specified.

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

`VIRTUAL_POLYMORPHIC` structs have two callable constructor forms:

- `.FULLOBJECT_CONSTRUCTOR` constructs a complete object and owns every reachable virtual base;
- `.SUBOBJECT_CONSTRUCTOR` constructs the type as a base subobject and never constructs a virtual base.

Every explicitly declared overload signature in one form must have a matching signature in the other form. The two declarations may have different delegate arguments and bodies. A full-object declaration may contain `VIRTUAL` delegates; a subobject declaration may not. Both declarations independently initialize the type's direct nonvirtual bases and fields, using default initialization for omitted delegates.

Ordinary source construction and `PLACE` select `.FULLOBJECT_CONSTRUCTOR`. A direct-base delegate selects `.SUBOBJECT_CONSTRUCTOR` when the selected base is `VIRTUAL_POLYMORPHIC`. Plain and `POLYMORPHIC` types use `.CONSTRUCTOR`; declaring either split form on those types is an error.

A `VIRTUAL_POLYMORPHIC` struct may write a `.CONSTRUCTOR` declaration as a synthesis template for an explicit pair. It is not a `.CONSTRUCTOR` member field or callable routine on that type. For each template overload, normalization synthesizes:

- a full-object form containing all declared delegates and the declared body;
- a subobject form containing the same direct-base and field delegates and body, with every virtual-base delegate and its argument expressions removed before expression generation.

The template declaration is consumed during normalization and creates two distinct semantic and ABI functions. No `.CONSTRUCTOR` field remains in the normalized `VIRTUAL_POLYMORPHIC` type, and lookup or code generation cannot select one. It does not pass a runtime `most_derived` flag. A template overload cannot coexist with an explicit full/subobject pair having the same normalized signature, although different signatures in one struct may choose different declaration styles. Compiler-generated implicit default/copy/move constructors on a `VIRTUAL_POLYMORPHIC` struct always synthesize the required pair.

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

The constructor-template form is useful when both generated bodies and all nonvirtual delegates are intentionally identical:

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

Every generated destructor and every user-written destructor without `NONVIRTUAL` in a polymorphic struct participates in the implicit destructor slot. The first such destructor introduces the slot and each derived destructor overrides every compatible inherited destructor slot automatically. Unlike an ordinary virtual member, a user-written destructor does not need to spell `VIRTUAL(OVERRIDE)`. An explicit virtual specifier is validated when present, and `VIRTUAL(FINAL)` or `VIRTUAL(OVERRIDE, FINAL)` can close the destructor slot. `NONVIRTUAL` is rejected on a destructor if any inherited destructor is virtual.

## Runtime object model

Pointers and references remain one machine word. Each `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` source subobject carries one hidden vtable pointer. Subobjects outside those categories carry no runtime header. Because every virtual-base owner is `VIRTUAL_POLYMORPHIC`, virtual-base navigation uses its existing vtable pointer rather than a separate navigation-only object category.

The vtable pointer addresses a compiler-private phase descriptor. Its fixed prefix identifies:

- the complete dynamic type, represented by the existing linked type-index ordinal;
- the current source subobject identity;
- a signed offset from that subobject to the complete object;
- the complete object's size and alignment;
- the active construction or destruction phase;
- the applicable cast and virtual-base navigation records.

The descriptor continues with RTTI and virtual slot entries. A `VIRTUAL_POLYMORPHIC` descriptor additionally contains the virtual-base navigation records required by that source subobject. Slot entries are function pointers to adjustment thunks, not necessarily direct user routine addresses. The fixed prefix implements the compiler-reserved `ALLOC_INFO()` virtual interface: from any polymorphic source subobject it can recover the complete allocation address and return it as `->VIRTUAL_STORAGE` together with the complete size and alignment. This is an implicit compiler operation, not a source-declarable or overridable member. A backend may lower it directly as descriptor loads and pointer adjustment rather than emitting a callable thunk.

Descriptors are immutable private constants. Construction and destruction change the hidden vtable pointers in the object; they do not mutate global tables.

Each construction or destruction descriptor also identifies its immutable phase descriptor group. The group contains fixed-index transition arrays for fields, direct bases, and canonical virtual bases. An entry names the descriptor assignments needed to enter that child phase, including complete-object-specific virtual-base offsets where required. Selector kind and ordinal determine the array and index at compile time; selecting an entry is a constant-offset load, not a name lookup, hierarchy walk, or runtime search.

This group reference is the implicit dynamic context for a reusable `VIRTUAL_POLYMORPHIC` subobject routine. For example, the active `B`-construction descriptor distinguishes `B` embedded in complete type `D` from the same `B` routine embedded in `E`. The routine's VMIR still says only “direct base ordinal 0”; native lowering follows the active descriptor to the `B`-in-`D` or `B`-in-`E` transition entry. Complete-object routines and cases whose phase group is statically known use a constant group address instead.

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

`ACCESS_FIELD` continues to address only a field declared directly by its static base type. Inherited field access is `INHERITANCE_CAST` followed by `ACCESS_FIELD`. This prevents field lists from being flattened and preserves source subobject identity.

## Construction

Construction extends the existing `NEW&& T`, `STRUCT_INIT_START`, delegate constructor, and `STRUCT_INIT_FINISH` lifecycle. It does not create a parallel object-state system.

For a complete object, semantic construction order is:

1. all reachable virtual bases, once, in depth-first left-to-right base-declaration order;
2. direct nonvirtual bases in declaration order;
3. direct fields in declaration order;
4. the constructor body.

Each base recursively constructs its own direct nonvirtual bases and fields. Written delegate order does not affect this sequence. A delegate argument expression is evaluated immediately before its selected subobject is initialized.

For a `VIRTUAL_POLYMORPHIC` type, the full-object constructor owns canonical complete-object virtual-base delegates and direct-base declaration delegates. Its subobject constructor never owns a virtual-base delegate. An explicitly declared `.SUBOBJECT_CONSTRUCTOR` generates its own delegates and body. A `.CONSTRUCTOR` template or implicit constructor generates the subobject form with virtual-base delegates removed before expression generation and the declared or generated body shared. In both cases the subobject form constructs direct nonvirtual bases and fields only. Plain and `POLYMORPHIC` types do not have these forms and always use only `.CONSTRUCTOR`.

Construction phase is derived from the existing lifetime state rather than passed as a compiler-private context value. `STORAGE_INIT_START` identifies a complete-storage construction delegate, while ordered `STRUCT_INIT_START` records distinguish fields, direct bases, and canonical virtual bases and retain their owning object. The caller therefore knows whether a constructor invocation initializes a complete object or a particular subobject without changing the constructor ABI.

Before invoking a constructor through one of those delegates, lowering installs the descriptor set for that delegate's construction phase. This makes virtual calls and dynamic RTTI operations inside the constructor observe the correct active type. After a successful invocation, the ordinary state-engine transition from dead storage to a completed delegate determines the next descriptor set: a completed base restores its enclosing constructor phase, a completed field enters that field object's steady phase, and a completed complete object enters its steady phase. Restoring the enclosing phase after the final base also makes the most-derived construction phase active before direct fields and the constructor body.

For a reusable subobject routine, lowering obtains the concrete child descriptor set by fixed-index selection from the currently active phase descriptor group. It retains the enclosing group or descriptor pointer in lowering-local SSA before overwriting any header and reapplies it after the child call when delegate state requires restoration. This temporary machine value is propagated to exceptional cleanup successors as required. It is not a VMIR operand, callable parameter, or semantic phase token.

This decision remains at the caller. For plain and `POLYMORPHIC` types, one ordinary `.CONSTRUCTOR` entry can serve both complete and base use because its caller prepares the appropriate runtime headers before entry and interprets the completed delegate role afterward. A `VIRTUAL_POLYMORPHIC` type never has that ordinary entry: a source `.CONSTRUCTOR` declaration is consumed as a template that generates distinct `.FULLOBJECT_CONSTRUCTOR` and `.SUBOBJECT_CONSTRUCTOR` routines.

A virtual call to a pure slot in the active phase is a defined runtime failure. A dynamic cast during construction sees the active phase type, not a future most-derived phase.

Implicit default, copy, and move constructors on a `VIRTUAL_POLYMORPHIC` type synthesize the same split:

- a complete copy/move initializes every canonical virtual base once from the corresponding source subobject;
- a subobject copy/move skips virtual bases;
- direct nonvirtual bases are processed before fields;
- generated constructor selection uses `.FULLOBJECT_CONSTRUCTOR` or `.SUBOBJECT_CONSTRUCTOR` according to the requested role.

Plain and `POLYMORPHIC` implicit constructors instead generate only `.CONSTRUCTOR`; they never acquire full-object/subobject forms.

## Allocation and `DELETE`

`NEW` selects its allocation family from the constructed type's declared object category:

- a nonpolymorphic type continues to use `DEFAULT_ALLOCATOR#T::ALLOC` and receives `->TYPED_STORAGE(T)`;
- a `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` type uses `DEFAULT_ALLOCATOR#VOID::VIRTUAL_ALLOC`, passing its complete size and alignment, and receives `->VIRTUAL_STORAGE`.

Construction projects the requested complete type into that storage through the existing generic storage initialization path. If construction fails, cleanup returns polymorphic storage through `VIRTUAL_DEALLOC` with the same statically known size and alignment used by `NEW`.

`DELETE` does not require a virtual destructor. The generator selects one of three paths from the static pointee type and its destructor policy; it does not branch on that policy at runtime.

For a nonpolymorphic type `T`, `DELETE` retains the existing static path: recover `->TYPED_STORAGE(T)`, destroy the complete `T`, and call `DEFAULT_ALLOCATOR#T::DEALLOC`.

For a polymorphic type whose destructor is virtual, `DELETE` proceeds as follows:

1. evaluate and retain the source object pointer;
2. invoke the compiler-reserved `ALLOC_INFO()` operation before destruction, saving the adjusted complete allocation pointer as `->VIRTUAL_STORAGE` together with its runtime size and alignment;
3. dispatch the virtual destructor, whose thunk adjusts the source subobject pointer and invokes the most-derived full-object destructor;
4. call `DEFAULT_ALLOCATOR#VOID::VIRTUAL_DEALLOC` with the saved allocation information.

The allocation information must be captured before destruction because destructor phase transitions replace runtime descriptors and the object's runtime header is no longer available after its lifetime ends. The saved storage pointer remains live after object destruction and is consumed only by deallocation. Deallocation also runs on an exceptional destructor exit if Quxlang permits that exit mode.

For a polymorphic `T` whose destructor carries `NONVIRTUAL`, `DELETE` statically invokes `T`'s complete-object destructor and calls `VIRTUAL_DEALLOC` with `T`'s statically known complete size and alignment. It performs neither a dynamic-type test nor an `ALLOC_INFO()` lookup. This path has the following language precondition:

> The operand must point to the complete object of exactly the static pointee type, and that object must have been produced by the matching polymorphic `NEW` allocation.

Violating this precondition, including deleting a derived object through a base with a nonvirtual destructor or deleting a local or `PLACE` object, is undefined behavior. The compiler may assume the precondition and use the object pointer directly as the complete `->VIRTUAL_STORAGE` address. A statically evident violation may be diagnosed, and constexpr evaluation must reject a path that violates it, but native code performs no runtime check.

## Destruction and failure cleanup

Destruction is the exact reverse of successful construction. A complete-object destructor:

1. enters the most-derived destruction phase;
2. executes the most-derived destructor body;
3. destroys fields in reverse declaration order;
4. destroys direct nonvirtual bases in reverse declaration order, entering each base's phase;
5. destroys owned virtual bases once in reverse virtual-base construction order.

A subobject destructor performs steps 1 through 4 for its own subobject and never destroys a virtual base. The compiler generates distinct full-object and subobject destructor entries when virtual-base ownership requires them; the source continues to declare `.DESTRUCTOR` once.

If construction fails before `STRUCT_INIT_FINISH`, only delegates that reached active state are destroyed, in reverse semantic delegate order. If the constructor body fails after `STRUCT_INIT_FINISH`, completed fields and bases are destroyed, but the enclosing destructor body is not invoked because construction of that enclosing object never completed.

Failure cleanup uses the same delegate-state edges as normal destruction. Before each cleanup destructor invocation, lowering installs that delegate's destruction-phase descriptors; after the delegate becomes dead, the target state determines the enclosing phase for the next cleanup action. No separate phase stack or runtime phase token is required.

Virtual destruction through a base dispatches to a thunk that adjusts the base pointer to the complete object and invokes the most-derived full-object destructor. `DELETE` through a nonvirtual destructor invokes the statically selected complete-object entry and is subject to the exact-complete-object precondition above; ordinary base cleanup continues to invoke the selected subobject entry. Destruction and deallocation remain separate operations: the destructor thunk does not select an allocator or release storage, and the `DELETE` operation owns the saved storage until it calls the matching deallocator.

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

### `INHERITANCE_CAST`

Proposed data:

```text
inheritance_cast {
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

### `STRUCT_ALLOC_INFO`

Proposed data:

```text
struct_alloc_info {
    source: local_index,
    storage_pointer: local_index,
    size: local_index,
    align: local_index
}
```

`source` is a retained or copied nonnull instance pointer whose static target is `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`. `storage_pointer` has type `->VIRTUAL_STORAGE`; `size` and `align` have type `SZ`. The instruction implements the implicit `ALLOC_INFO()` interface by loading the source subobject's steady descriptor, applying its signed complete-object adjustment, and returning the complete allocation layout. It consumes the supplied pointer value but does not alter the pointed-to object; the generator retains the original pointer for the following virtual destructor call.

`STRUCT_ALLOC_INFO` is emitted only for the virtual-destructor `DELETE` path. A nonvirtual `DELETE` uses the static pointee type's complete layout and emits no runtime type or allocation-info operation.

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

## Instructions modified

### `STRUCT_INIT_START`

The current `invocation_args fields` operand is replaced by an ordered delegate list:

```text
struct_init_delegate {
    selector: field_ordinal | direct_base_ordinal | virtual_base_ordinal,
    value: local_index
}

struct_init_start {
    on_value: local_index,
    delegates: vector<struct_init_delegate>
}
```

Field and direct-base ordinals are declaration ordinals for the current static struct. Virtual-base ordinals index the canonical list for the complete-object type and are legal only in a full-object constructor.

The instruction:

- transitions `on_value` from storage state to partial state;
- creates exact storage aliases for fields and base subobjects;
- records delegates in semantic construction order;
- associates every delegate with `on_value` for reverse cleanup;
- validates that selector types match the declared field or base type.

The state engine retains each delegate's selector, role, semantic ordinal, activation state, and `delegate_of` owner. Those records determine which transition is required. The active runtime descriptor or a statically known complete-object descriptor supplies the concrete descriptor group from which lowering selects the transition entry. Names no longer carry lifetime identity. This also removes the current restriction that constructor delegates are named fields only.

### `STRUCT_INIT_FINISH`

The operand remains `on_value`. Its semantics are extended to validate and retire ordered field/base delegate aliases, preserve the completed runtime headers, clear partial-construction ownership, and transition the owner to full state. It does not itself install a steady descriptor set: the successful enclosing constructor `INVOKE` determines whether completion enters steady state or restores a parent construction phase.

The cleanup engine retains enough compiled state to distinguish failure before finish from failure in the constructor body. Finishing does not authorize the enclosing destructor body after a constructor-body failure.

### `STORAGE_INIT_START`

The instruction data remains unchanged. Its construction delegate identifies complete-object storage and retains the statically selected complete type. An `INVOKE` that consumes that delegate installs the complete type's construction descriptors before entering the constructor.

### `INVOKE`

The instruction data and callable ABI remain unchanged. When `args.named["THIS"]` is a construction delegate, lowering compares the pre-instruction and post-instruction VMIR states around the call. It uses the selector kind and ordinal to choose a fixed transition entry from the active phase descriptor group, installs that entry's construction descriptors before the call, and retains the enclosing group pointer in lowering-local SSA. On normal return it installs the phase implied by the completed delegate role: steady for a complete object or field object, and the retained enclosing construction phase for a base subobject. Exceptional edges use their target delegate state and the retained pre-call group instead of a phase value returned by the call.

### `DESTROY`

The instruction data remains unchanged. When the destroyed value is an inheritance delegate, lowering selects and installs its destruction descriptors before the destructor invocation and retains the enclosing group pointer because the destroyed header cannot be read after the delegate becomes dead. The post-destruction state selects whether to reapply that enclosing phase. Compiler-generated cleanup edges apply the same rule to completed delegates in reverse semantic order.

These extensions require no phase operand, hidden constructor argument, phase token, or phase-specific VMIR instruction. The lowering-local group or descriptor pointer has no VMIR identity or source-visible lifetime and does not cross the constructor or destructor ABI.

## Instructions retained unchanged

- `ACCESS_FIELD` remains a direct-field projection by the static struct layout.
- `CAST_PTRREF` remains address-preserving and is never an inheritance cast.
- `INVOKE_INDIRECT` and the existing callable representation are reused by virtual-call lowering after slot selection.
- `MAKE_REFERENCE`, `COPY_REFERENCE`, and the `NEW&&`/`DESTROY&&` slot conventions keep their existing meanings.

## Instructions removed

No existing VMIR2 instruction needs to be removed. Phase management adds no construction-context or phase-entry instruction. The `STRUCT_INIT_START` operand replacement is an intentional IR schema change, not a compatibility layer; all producers and consumers are updated together.

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

Returns virtual slots in deterministic order, including each introducing declaration, compatible inherited slots, final overrider, pure/final state, concrete callable signature, and required `THIS` adjustments. It synthesizes the implicit destructor slot for each polymorphic root whose destructor is generated or untagged and automatically treats derived destructors as overrides. It diagnoses virtual declarations outside the two polymorphic categories, missing `VIRTUAL(OVERRIDE)` on ordinary functions, a `NONVIRTUAL` destructor after an inherited virtual destructor, illegal overrides of `VIRTUAL(FINAL)`, incompatible return types, and non-unique final overriders.

### `struct_constructor_forms_query(type_symbol)`

For a `VIRTUAL_POLYMORPHIC` struct, returns each normalized constructor signature with its full-object and subobject entries and an origin of `explicit_pair`, `constructor_template`, or `compiler_generated`. It validates explicit pairing, prevents a template/explicit collision for the same signature, rejects virtual-base delegates in a subobject declaration, consumes each `.CONSTRUCTOR` template without creating a callable field, and synthesizes the two semantic declarations for template and implicit constructors. Plain and `POLYMORPHIC` structs bypass this split-form query and expose only `.CONSTRUCTOR`; either split declaration on those categories is invalid.

### `struct_runtime_requirements_query(type_symbol)`

Returns layout-independent runtime requirements: which source subobjects need vtable headers, the struct's plain/`POLYMORPHIC`/`VIRTUAL_POLYMORPHIC` category, its effective destructor policy, whether RTTI and virtual-base navigation records are needed, and which direct base is the primary-base candidate. Keeping this separate from final offsets prevents a query cycle between virtual-slot analysis and layout.

### `struct_runtime_info_query(type_symbol)`

Combines inheritance, virtual slots, concrete layout, and reached routines into backend data for one complete type. It contains phase descriptor groups; fixed-index field, direct-base, and virtual-base transition arrays; the header assignments for each transition; complete allocation size/alignment; source-subobject-to-complete offsets; virtual-base offsets; cast records keyed by canonical type symbols; slot ordinals; and adjustment-thunk descriptions. Each emitted phase descriptor can reach its containing group at a fixed offset or through a fixed-prefix reference. Linked type-index ordinals are assigned later by output collection so this query does not depend on link-wide numbering.

### `struct_default_subobject_ctor_query(type_symbol)` and `struct_subobject_dtor_query(type_symbol)`

These parallel the existing complete-object `class_default_ctor_query` and `class_default_dtor_query` when a base delegate or cleanup edge needs the subobject entry. They return no entry for types that do not need a split, allowing the caller to use the ordinary constructor or destructor.

## Queries modified

- `struct_layout_query` consumes direct-base, inheritance, and runtime-requirement data and produces direct-base offsets, virtual-base offsets, nonvirtual extent/alignment, complete size/alignment, fields, and runtime-header placement.
- `class_placement_info_query` uses `struct_layout.complete_size` and `struct_layout.complete_align` for a complete struct. Direct-base placement uses `nonvirtual_size` and `nonvirtual_align` instead.
- `class_default_ctor_query` selects the full entry returned by `struct_constructor_forms_query` for `VIRTUAL_POLYMORPHIC` and `.CONSTRUCTOR` otherwise.
- `list_builtin_constructors_query` and constructor-call initialization expose only the full-object side to ordinary complete-object construction while preserving existing type-construction syntax.
- `class_default_dtor_query` selects the complete-object destructor entry when destruction ownership is split.
- allocator-member resolution selects typed `DEFAULT_ALLOCATOR#T` members for nonpolymorphic complete types and the `DEFAULT_ALLOCATOR#VOID` virtual allocator members for either polymorphic category.
- `class_requires_gen_default_ctor_query`, copy/move constructor queries, assignment/swap queries, and default-destructor queries include base subobjects in addition to fields.
- `have_nontrivial_member_ctor_query`, `have_nontrivial_member_dtor_query`, triviality queries, and relocatability queries account for base operations and runtime-header initialization. A runtime header makes construction nontrivial but does not by itself make byte relocation unsafe.
- `functum_list_user_overload_declarations_query`, function declaration normalization, and instantiation preserve the grouped virtual specifier, retain both user-declared constructor forms, and expose synthesized constructor forms under their exact semantic functum names.
- expression member resolution and `co_generate_dot_access` consume `struct_member_lookup_query` results and emit receiver projections before direct field or function handling.
- `declaration_is_accessible_query` checks inherited declarations using the original access context after member lookup has selected the declaring symbol.
- `output_llvm_input_query`, `vmir_dependencies_query`, `functanoid_required_struct_layouts_query`, and `routine_requirements` collect runtime info, descriptors, thunks, and every type/routine reached through a virtual slot or RTTI cast.
- `constexpr_eval_query` and `run_static_test_query` request hierarchy and runtime information needed by inheritance instructions.
- generated comparison, assignment, swap, and antestatal checks account for base subobjects. Implicit comparison and swap apply only to nonpolymorphic structs; polymorphic structs are not implicit datatypes and do not receive those generated operations. Serialization remains exact-type behavior and requires no subobject entry.

`struct_field_list_query` remains deliberately direct-field-only. `active_subdeclaroids_query`, `exists_query`, and `symbol_type_query` do not pretend that an inherited declaration is owned by the derived type; inherited access goes through the new member-resolution result.

## Queries removed

No existing query is removed. Existing queries retain their conceptual ownership boundaries, while their dependency lists and implementations are extended where complete-object behavior changes.

# Codegen

## Whole-program ABI data

`llvm_compilable_unit` gains a map of canonical complete struct types to `struct_runtime_info`. Reachability determines which descriptors and thunks are emitted. All symbols use private linkage unless a future explicit Quxlang ABI export feature says otherwise.

For each reached runtime-bearing complete type, LLVM codegen may emit:

- signed virtual-base offsets and navigation records for `VIRTUAL_POLYMORPHIC` types;
- for either polymorphic category, a compact subobject/cast table sorted by target type ordinal and subobject identity;
- one descriptor group for steady state and each observable construction/destruction phase, with complete size, complete alignment, and source-subobject-to-complete adjustment in the fixed prefix;
- for either polymorphic category, virtual dispatch arrays whose fixed prefix references the descriptor data, including an empty slot array when the type has no virtual functions;
- signed offsets for direct adjustments, virtual bases, and complete-object recovery;
- one `THIS` adjustment thunk per distinct source-slot/final-overrider adjustment;
- a pure-slot failure target where a phase has no implementation.

No descriptor contains a source spelling. Existing linked type-index ordinals provide runtime type identity to both polymorphic categories. Only `VIRTUAL_POLYMORPHIC` descriptors contain virtual-base navigation records.

## Static base conversion

A nonvirtual `INHERITANCE_CAST` lowers to a byte-address GEP using the queried signed offset, followed by a cast back to the destination pointer representation. Pointer casts use a null-preserving select so a nonzero base offset cannot turn null into a nonnull address.

A virtual-base `INHERITANCE_CAST` loads the source runtime header, reads the canonical virtual-base offset, recovers the complete-object address, and computes the target. The result remains one word.

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

`STRUCT_INIT_START` computes field and base storage aliases from `struct_layout`. The state engine records each alias's exact field, direct-base, or canonical virtual-base selector together with its owner and semantic order. `STORAGE_INIT_START` already distinguishes the construction delegate for complete storage.

For a reusable subobject routine, the active descriptor is the runtime carrier of complete-object-specific phase context. Its group is conceptually laid out as:

```text
phase_descriptor_group {
    current_phase_assignments,
    field_transitions[field_count],
    direct_base_transitions[direct_base_count],
    virtual_base_transitions[virtual_base_count]
}

phase_transition {
    header_assignments[]
}

phase_header_assignment {
    signed_header_offset,
    descriptor_address
}
```

The concrete binary layout may fold or colocate these constants, but every source descriptor must reach its group without a search. Selector kind chooses one transition array and the semantic ordinal supplies a compile-time index. The number of header assignments for a selected static subobject hierarchy is also compile-time-known, so LLVM lowering emits direct loads and stores rather than a runtime loop. Complete-object calls and other statically resolved cases use descriptor constants and eliminate the group load entirely.

For example, a single `B` subobject routine may run as `B` inside either `D` or `E`, with different virtual-base offsets. If it initializes direct base ordinal zero, lowering is conceptually:

```text
enclosing_group = load_group(B.runtime_header)
child_transition = enclosing_group.direct_base_transitions[0]
install_unrolled(child_transition.header_assignments)
INVOKE A-subobject-constructor
install_unrolled(enclosing_group.current_phase_assignments)
```

When `B` is inside `D`, its active descriptor reaches the `B`-in-`D` group; inside `E`, it reaches the `B`-in-`E` group. The `B` routine and its VMIR are not specialized by complete type. This occupies the same role as a C++ construction-vtable table, but the active object descriptor carries the group identity instead of a hidden constructor/destructor parameter. The expected runtime difference is at most a cached descriptor/group load in cases that cannot be constant-folded; the required header stores are inherent to correct construction-phase dispatch in either model.

Existing `INVOKE` lowering uses those records to bracket a constructor call with descriptor stores:

1. before a complete-object constructor call, install the root construction descriptor set;
2. before a base constructor call, install that exact base's construction descriptor set;
3. before a field constructor call, install that field object's root construction descriptor set;
4. after normal return, apply the state-engine transition and install the descriptor set implied by the completed delegate: complete-object steady state, field-object steady state, or the enclosing constructor's construction phase after a base;
5. on an exceptional edge, use the edge's target delegate state to install the phase required by the first cleanup action.

The backend already has both the state immediately before an instruction and the state produced after applying it, so descriptor stores remain part of ordinary call lowering and post-instruction state handling. Before overwriting a child header, it retains the enclosing group or descriptor pointer in lowering-local SSA across the call. That pre-call value is available to the normal successor and propagated to exceptional cleanup successors, using PHIs where cleanup edges merge, and remains usable even after child destruction ends the child's lifetime. `STRUCT_INIT_FINISH` validates completion but does not force a steady phase; the enclosing successful constructor invocation makes that decision.

`DESTROY` and generated cleanup edges apply the symmetric rule. Before each destructor call they install the exact delegate's destruction descriptor set. After that delegate becomes dead, the target state selects the enclosing destruction or construction-failure phase needed by the next action. Exceptional construction cleanup and ordinary reverse destruction therefore use the same delegate-state mechanism rather than a separate phase stack.

Constructor and destructor routines receive no compiler-private context or phase argument. Explicit constructor pairs lower independently. On a `VIRTUAL_POLYMORPHIC` type, a source `.CONSTRUCTOR` declaration is consumed as a template and cloned into full/subobject semantic routines before VMIR generation; no `.CONSTRUCTOR` field or routine is emitted. Plain and `POLYMORPHIC` types retain the ordinary `.CONSTRUCTOR` callable. In every case the caller prepares the runtime headers before entry, so the function body does not branch on its construction role.

Phase descriptors, groups, transition arrays, and header assignments are static constants and descriptor installation performs no allocation. There is no runtime type test, selector comparison, name lookup, or hierarchy traversal. Stores and transition loads may be removed when the group is statically known, the queried pre-call and post-call descriptor sets are identical, or the runtime header is unobservable.

Virtual destructor slots point to thunks that recover the complete object and call the full-object destructor entry. They do not deallocate storage.

`STRUCT_ALLOC_INFO` loads the complete-object adjustment, size, and alignment from the steady descriptor, adjusts the source address, and returns the adjusted address as `->VIRTUAL_STORAGE`. The virtual `DELETE` path retains those outputs across the destructor call and then invokes `DEFAULT_ALLOCATOR#VOID::VIRTUAL_DEALLOC`. The nonvirtual path contains no descriptor load or type comparison: it uses the statically known complete layout and relies on the exact-complete-object precondition.

## Constexpr

The constexpr interpreter represents a struct as a canonical subobject graph instead of a flat direct-field map when inheritance is present. It tracks:

- distinct nonvirtual identities;
- canonical virtual-base identities;
- direct fields for each subobject;
- the active construction/destruction phase;
- canonical complete-object identity and ordered delegate state.

Static casts, virtual calls, exact type tests, dynamic casts, and allocation-info recovery are constexpr-capable when all reached functions are constexpr-evaluable. No host address, native byte offset, descriptor pointer, or phase token is exposed; the interpreter follows semantic subobject identities and derives phase changes from the same `INVOKE`, `DESTROY`, and cleanup state transitions as native codegen. Constant evaluation diagnoses a nonvirtual `DELETE` whose operand is not the exact complete object of its static pointee type; this does not introduce a native runtime check.


# Detailed implementation

## Public compiler data model

The following data changes are required. Names are proposed C++ interface names and should receive Doxygen comments when implemented.

1. Extend `declaroid` with `ast2_base_declaration`, containing `type_symbol base_type` and `inheritance_kind kind`. A base is an ordinary `member_subdeclaroid`: its member name is the named base selector (or empty for `.BASE`), while the existing member wrapper carries `include_if`, documentation, privacy, and source location. Keeping bases on the ordinary member path preserves source order and conditional-declaration behavior without adding a new subdeclaroid category.
2. Add `inheritance_kind { nonvirtual, virtual_ }`.
3. Add `optional<ast2_virtual_specifier> virtual_specifier` and `bool is_nonvirtual` to `ast2_function_header`. The specifier represents `VIRTUAL` and contains `is_override`, `is_final`, and `is_pure` for the parenthesized options. `is_nonvirtual` records the `NONVIRTUAL` function-header tag and is validated as destructor-only during normalization.
4. Add a constructor-delegate kind to `ast2_function_delegate` and normalized `function_delegate`, distinguishing ordinary member/direct-base selection from a nominal virtual-base target.
5. Add `struct_polymorphism_kind { none, polymorphic, virtual_polymorphic }`, `struct_destructor_policy { category_default, nonvirtual }`, and canonical semantic data types: `struct_base_declaration`, `struct_subobject_id`, `struct_subobject_path`, `struct_inheritance_info`, `struct_member_lookup_result`, `struct_virtual_slot_key`, `struct_virtual_slot`, `struct_phase_key`, `struct_phase_header_assignment`, `struct_phase_transition`, `struct_phase_descriptor_group`, `struct_runtime_requirements`, and `struct_runtime_info`. A header assignment contains the target runtime-header offset relative to the active phase root and the descriptor key. A transition contains the ordered header assignments needed to install one phase. A descriptor group contains its phase identity and ordinal-indexed field, direct-base, and virtual-base transition arrays. Runtime requirements record whether the effective destructor is virtual; runtime info records these groups together with complete allocation size/alignment and subobject-to-complete adjustments.
6. Add `constructor_form_origin { explicit_pair, constructor_template, compiler_generated }` and `struct_constructor_form` with normalized signature, full-entry symbol, subobject-entry symbol, and origin. `struct_constructor_forms` maps normalized signatures to those records. A `VIRTUAL_POLYMORPHIC` `.CONSTRUCTOR` template is not retained as a member field in normalized data.
7. Add VMIR's ordered `struct_init_delegate` selector variant and `struct_alloc_info` instruction with `->VIRTUAL_STORAGE`, `SZ` size, and `SZ` alignment outputs. Extend delegate state with the selector role and semantic ordinal required to choose phase descriptors around existing `INVOKE` and `DESTROY` operations; do not add a context or phase value type.
8. Add `struct_base_layout_info` with subobject identity, selector, canonical type, declaration ordinal, and signed offset; add `struct_virtual_base_layout_info` with canonical identity, type, virtual ordinal, and signed offset; and add `struct_runtime_header_layout` with offset, polymorphism category, and optional primary-base identity.
9. Expand `struct_layout` with `direct_bases`, `virtual_bases`, optional runtime-header placement, `nonvirtual_size`, `nonvirtual_align`, `complete_size`, and `complete_align`. Remove the old ambiguous `size` and `align` fields. `fields` remains the list of direct fields.
10. Extend `llvm_compilable_unit` and dependency records with reached `struct_runtime_info` and adjustment-thunk requirements.

These are deliberate schema changes. Every producer and consumer should move in one change series; no old-field compatibility alias should remain.

## Parser and AST normalization

1. Add `BASE`, `VIRTUAL_BASE`, `SUBOBJECT_CONSTRUCTOR`, and `FULLOBJECT_CONSTRUCTOR` to the appropriate keyword sets, and add `POLYMORPHIC`, `VIRTUAL_POLYMORPHIC`, and `FINAL` to struct keywords. Add `NONVIRTUAL` to the function-header suffix parser as a destructor-only tag.
2. Extend struct-body declaration parsing before the generic `.name` path so `.BASE Type;` is recognized without treating `BASE` as a member name.
3. Parse `.name BASE Type;`, `.name VIRTUAL_BASE Type;`, and anonymous `.BASE Type;` into `member_subdeclaroid` values whose declaration payload is `ast2_base_declaration`. Reuse the member wrapper's `INCLUDE_IF` handling so inactive edges do not enter the hierarchy.
4. Replace the one-shot `THIS` suffix parser in `try_parse_function_declaration.hpp` with a suffix loop that accepts one `THIS` qualifier, one `VIRTUAL` specifier, and one `NONVIRTUAL` tag. Parse the virtual specifier's optional comma-separated `OVERRIDE`, `FINAL`, and `PURE` options as a group. Validate duplicates, `VIRTUAL`/`NONVIRTUAL` conflict, empty parentheses, and `FINAL`/`PURE` conflict there; declaration-name and hierarchy-dependent checks belong in normalization and queries.
5. Allow a `VIRTUAL(PURE)` function to end in `;` and store an empty body that code generation never instantiates.
6. Extend `try_parse_function_delegates.hpp` to recognize `VIRTUAL type` before the existing callsite argument parser.
7. Add `DYNAMIC` to the `AS` modifier parser and intercept it in expression generation before constructor-based conversion selection.
8. Update AST metadata, comparison, hashing, serialization, debug printing, and parser tests for every new field and variant alternative.
9. Audit declaration consumers for the new `declaroid` alternative. Hierarchy queries consume member declarations whose payload is `ast2_base_declaration`; ordinary overload, symbol-kind, and field queries skip that payload.
10. Keep `.CONSTRUCTOR`, `.FULLOBJECT_CONSTRUCTOR`, and `.SUBOBJECT_CONSTRUCTOR` as distinct parsed declaration names. Constructor-form synthesis occurs in queries, not in the parser; normalization consumes a `VIRTUAL_POLYMORPHIC` `.CONSTRUCTOR` declaration instead of publishing it as a member field.
11. Update constructor classification in formal-signature normalization, instantiation, and generation so all three names receive the `NEW&& THISTYPE` `THIS` convention and existing constructor-specific validation.

## Hierarchy, lookup, and override analysis

1. Implement direct-base normalization and recursive hierarchy construction before touching layout.
2. Read the polymorphism category through the existing struct keyword/tag path and derive the destructor policy from the active `.DESTRUCTOR` declaration's `NONVIRTUAL` tag. Reject virtual declarations on an unmarked struct, reject `NONVIRTUAL` on a non-destructor or a destructor outside a polymorphic struct, reject `VIRTUAL_BASE` outside `VIRTUAL_POLYMORPHIC`, require every struct deriving from a polymorphic base to declare a compatible category, and require `VIRTUAL_POLYMORPHIC` whenever a base closure contains virtual inheritance.
3. Assign declaration ordinals before filtering physical layout; diagnostics and semantic order must not depend on alignment sorting.
4. Build canonical virtual roots and stable subobject identities. Cache conversion paths and ambiguity results in `struct_inheritance_info` rather than repeatedly walking the AST.
5. Route dot access and bound member calls through `struct_member_lookup_query`. The returned receiver path is carried through overload selection so the chosen callable and adjusted `THIS` cannot become separated.
6. Build virtual slots from normalized concrete signatures. Synthesize the category-default destructor slot for generated and untagged destructors, automatically override inherited destructor slots, reject a `NONVIRTUAL` destructor when a base destructor is virtual, preserve each introduced slot across descendants, compute final overriders, and generate one thunk requirement for each source-subobject adjustment.
7. Normalize constructor forms per signature. Retain explicit full/subobject declarations, consume an allowed `.CONSTRUCTOR` template to synthesize both entries, and reject missing pairs or signature collisions. Assert that the normalized `VIRTUAL_POLYMORPHIC` member set contains no `.CONSTRUCTOR` field.
8. Keep constructor and destructor names out of ordinary inherited member lookup. They are selected only by lifecycle role.
9. Diagnose abstract complete-object use at the query that chooses construction or by-value placement, not only during LLVM emission.

## Layout and runtime descriptors

1. Refactor `struct_layout.cpp` into semantic placement of headers/bases/fields/virtual bases. Continue using `class_placement_info_query` for field types, but use base layouts' nonvirtual extents and alignments for base placement.
2. Select the primary base from runtime requirements before assigning offsets, preserving compatibility between the root polymorphism category and the candidate base's vtable header.
3. Implement empty-base optimization with an occupied-address set keyed by canonical subobject type and offset.
4. Produce complete size and nonvirtual extent in one query result so consumers cannot accidentally use complete size for a base.
5. Build runtime descriptor records only after offsets and final overriders are known. Put complete allocation size/alignment and each source subobject's signed complete-object adjustment in the fixed prefix used by `STRUCT_ALLOC_INFO`. Give every construction/destruction descriptor fixed access to its phase descriptor group. Populate ordinal-indexed field, direct-base, and virtual-base transition arrays with unrollable header assignments, including complete-object-specific virtual-base offsets. Deduplicate identical groups, transitions, phase tables, and adjustment thunks.
6. Reject inherited types at external ABI boundaries until an explicit external layout convention is selected.

## VMIR generation and lifetime state

1. Add the inheritance cast, dynamic dispatch, RTTI, and allocation-info instructions and metadata variants in `vmir2.hpp`, the textual assembler/parser, debug rendering, and equality metadata. Do not add an instruction for construction context or phase entry.
2. Replace `slot_state.delegates`' name-based `invocation_args` with ordered semantic delegate records. Record selector, role, semantic ordinal, activation stage, cleanup entry, and owning object. Do not put native descriptor or group pointers in `slot_state`.
3. Extend `state_engine.hpp` so complete objects, bases, and fields use the same partial/full/dead state transitions. Make the pre/post states around existing `INVOKE` and `DESTROY` operations the authority for which phase transition is required; native lowering separately resolves the concrete transition group from a static complete type or the active descriptor. Do not create a separate base-lifetime or phase stack.
4. Refactor `co_generate_struct_ctor_delegates` to request inheritance info, create all delegate slots in semantic order, emit one `STRUCT_INIT_START`, and then generate each selected initializer in that order.
5. For `VIRTUAL_POLYMORPHIC`, generate each explicitly declared full/subobject constructor from its own normalized declaration. For template and implicit forms, generate both routines from the synthesized declarations and suppress virtual delegate expression generation entirely in the synthesized subobject routine. For plain and `POLYMORPHIC`, generate only `.CONSTRUCTOR`.
6. Extend generated default/copy/move/assignment routines to process bases. Polymorphic assignment remains static and operates on each canonical virtual base once for the exact complete type before direct nonvirtual bases and fields. Do not synthesize swap for a polymorphic struct and do not add compiler-private subobject assignment, swap, comparison, or serialization methods.
7. Generate full/subobject destructor entries and teach all cleanup exits to select the correct entry from the delegate role. Do not emit explicit phase operations; call and cleanup lowering derives descriptor writes from the selected entry and delegate-state transition.
8. Generate `INHERITANCE_CAST` for inherited field access, inherited receiver binding, implicit base conversions, and exact base selection.
9. Generate `INVOKE_VIRTUAL` only after ordinary overload resolution chooses a virtual slot. Explicit `Type::.member` calls remain direct.
10. Generate dynamic cast and exact type-test instructions with canonical target types.
11. Route polymorphic `NEW` through `DEFAULT_ALLOCATOR#VOID::VIRTUAL_ALLOC`, project the complete object through the existing storage initialization path, and use `VIRTUAL_DEALLOC` on construction-failure cleanup.
12. Split `DELETE` generation by the static destructor policy. Emit `STRUCT_ALLOC_INFO`, virtual full-object destruction, and saved-info deallocation for a virtual destructor; emit static destruction and static-layout `VIRTUAL_DEALLOC` with no runtime check for a destructor tagged `NONVIRTUAL`.

## VMIR consumers and dependency collection

Every VMIR visitor must handle the new instruction variants, including `STRUCT_ALLOC_INFO`, and the changed init record:

- `vmir2/state_engine.hpp` validates lifetimes and cleanup;
- `vmir2/assembler.hpp` and `sources/vmir2/assembler.cpp` provide stable textual forms;
- `sources/vmir2/routine_requirements.cpp` collects descriptor, layout, type-index, thunk, and final-overrider dependencies;
- `sources/queries/vmir_dependencies.cpp` propagates those dependencies;
- `sources/ir2_constexpr_interpreter.cpp` implements semantic subobject behavior;
- `sources/llvm-backend.cpp` implements native layout operations and descriptors;
- `sources/cortado-backend.cpp` emits the initial capability diagnostic;
- `sources/queries/output_llvm_input.cpp` closes descriptor and thunk reachability.

VMIR consumers that model execution must apply the same phase rule. Native lowering compares the state immediately before and after `INVOKE` or `DESTROY`; exceptional and cleanup edges use their target state. It combines the semantic selector with the statically known group or active descriptor, emits a constant-index transition load where necessary, and retains the enclosing group in lowering-local SSA. The constexpr interpreter updates semantic phase identity at those transitions without materializing descriptor pointers. The assembler and serialized VMIR therefore contain delegate selectors but no context local, phase token, group pointer, or phase-entry instruction.

Query specs, dependency typelists, query registration shards, and generator dependency lists must be updated in the same series. A new query is not complete until its spec dependencies and compiler querygraph registration are present.

## Generated operations and type traits

Audit every operation that currently iterates only `struct_field_list`:

- default/copy/move construction;
- destruction and failure cleanup;
- copy/move assignment;
- swap;
- equality and three-way comparison generation;
- serialization and deserialization for nonpolymorphic exact types;
- trivial construction/destruction/relocation queries;
- `ANTESTATAL`, `SERIALOID`, `STRINGLIKE`, and related struct-tag validation;
- dependency discovery for every generated routine.

Generated construction, destruction, and polymorphic assignment process canonical virtual bases once before direct nonvirtual bases and fields. Constructor and destructor full/subobject entries carry lifetime ownership. Assignment is statically generated inline for the exact complete type and does not create a callable subobject method. Implicit comparison, serialization, deserialization, and swap do not apply to polymorphic structs.

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
- a `VIRTUAL_POLYMORPHIC` `.CONSTRUCTOR` template collision with an explicit pair;
- split constructors on a type that is not `VIRTUAL_POLYMORPHIC`;
- `NONVIRTUAL` on a non-destructor, on a destructor outside a polymorphic struct, or on a destructor that overrides an inherited virtual destructor;

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
- descriptor-phase restoration after every base constructor and destructor, including exceptional cleanup;
- reusable `VIRTUAL_POLYMORPHIC` subobject constructor and destructor routines embedded in at least two complete types whose virtual bases have different offsets, proving that their active descriptors select different concrete transition groups without routine specialization;
- emitted-code checks that transition selection is a constant-index load followed by unrolled header stores, with no selector branch, runtime loop, name lookup, or hierarchy traversal;
- use of one ordinary `.CONSTRUCTOR` callable for complete and base construction of a `POLYMORPHIC` type, with no hidden context or phase ABI argument;
- consumption of a `VIRTUAL_POLYMORPHIC` `.CONSTRUCTOR` template into two callable routines, with no normalized `.CONSTRUCTOR` field, emitted `.CONSTRUCTOR` symbol, or hidden `most_derived` boolean;
- VMIR checks proving that constructor/destructor phase management uses ordered delegate state around `INVOKE` and `DESTROY` and emits no context, token, or phase-entry instruction;
- `POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` opt-in, category propagation, implicit virtual destructors, valid and invalid destructor `NONVIRTUAL` tags, invalid virtual-base categories, multiple-slot override, `VIRTUAL(OVERRIDE)`, `VIRTUAL(FINAL)`, `VIRTUAL(PURE)`, and abstract construction diagnostics;
- explicit full/subobject constructor pairs, mismatched pairs, and `.CONSTRUCTOR` synthesis;
- implicit upcasts, virtual-base upcasts, nullable downcasts, cross-casts, ambiguity, and null preservation;
- polymorphic `NEW` through `VIRTUAL_ALLOC`, including construction-failure deallocation;
- virtual destruction from each base subobject followed by one `VIRTUAL_DEALLOC` of the adjusted complete allocation;
- exact-type nonvirtual polymorphic `DELETE`, with emitted-code checks proving that it performs no RTTI test or `STRUCT_ALLOC_INFO` lookup;
- generated default/copy/move behavior, static polymorphic assignment, generated swap for nonpolymorphic structs, and the absence of generated swap for polymorphic structs;
- constexpr casts, calls, virtual allocation-info recovery, and rejection of invalid nonvirtual deletion;
- preservation of metadata-free codegen for plain nonvirtual inheritance.

Implementation validation should use the repository's targeted compiler/testmodule scripts while iterating. After code review, run `cbuild test -c Release` once for the complete series. Inspect emitted LLVM IR or object code for representative cases to confirm constant-offset upcasts, table-based virtual-base adjustment, one-word pointers, devirtualization, fixed-index phase-transition loads, unrolled descriptor stores immediately around constructor/destructor calls, restoration from retained enclosing groups on normal and exceptional edges, an unchanged constructor calling convention, polymorphic virtual-allocation pairing, absence of dynamic checks in nonvirtual `DELETE`, and absence of runtime metadata for a plain nonvirtual hierarchy.

## Suggested implementation order

1. AST, parser, canonical base data, and invalid-hierarchy diagnostics.
2. Member lookup, static conversion paths, virtual slot analysis, and semantic tests.
3. `struct_layout` nonvirtual/complete extents and runtime-header placement.
4. Ordered `STRUCT_INIT_START`, phase descriptor groups and fixed-index transitions, state-driven constructor/destructor phase handling, constructor pairs, cleanup, and destructor pairs.
5. Static inheritance casts and inherited field/function receiver generation.
6. Virtual invoke, phase descriptors, RTTI, dynamic casts, polymorphic allocation info, and virtual/nonvirtual `DELETE` paths.
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

**Decision:** rejected. Use `INHERITANCE_CAST`.

## Runtime name or hash lookup

**Pros:** straightforward metadata construction and convenient debugging.

**Cons:** larger binaries, collision policy, variable runtime cost, and source-name ABI leakage.

**Decision:** rejected. Use linked type ordinals and dense/sorted constant records.

## Always synthesize the subobject constructor

**Pros:** one source body and no explicit pairing requirement.

**Cons:** prevents a type from intentionally using different direct-base arguments or constructor bodies when it is complete versus embedded, and hides a semantically important callable boundary.

**Decision:** rejected as the only source model. `VIRTUAL_POLYMORPHIC` users may declare both `.FULLOBJECT_CONSTRUCTOR` and `.SUBOBJECT_CONSTRUCTOR`, or may write a `.CONSTRUCTOR` template when one declaration is preferable. The template is consumed and still produces both callable routines.

## Keep `.CONSTRUCTOR` and pass a hidden `most_derived` boolean

**Pros:** fewer semantic function names.

**Cons:** virtual-base ownership becomes a runtime branch inside every affected constructor, ignored arguments are harder to suppress before evaluation, and the callable contract does not express its role.

**Decision:** rejected. A `VIRTUAL_POLYMORPHIC` type cannot have a `.CONSTRUCTOR` field or routine. Its `.CONSTRUCTOR` declaration is only a synthesis template, consumed at normalization to create distinct `.FULLOBJECT_CONSTRUCTOR` and `.SUBOBJECT_CONSTRUCTOR` routines. Neither routine receives a `most_derived` boolean.

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

**Cons:** risks choosing layout, constructor, and VMIR abstractions that cannot represent the complete object model. Retrofitting construction/destruction phase semantics and virtual-base ownership would invalidate early decisions.

**Decision:** rejected as a design strategy. Implementation may be staged, but the data model and lifecycle must account for the full scope from the start.

## Add explicit VMIR construction-context and phase-entry instructions

**Pros:** makes each descriptor write locally visible in the instruction stream and gives the backend a direct phase operand.

**Cons:** duplicates information already represented by complete-storage delegates, ordered `STRUCT_INIT_START` selectors, `delegate_of` ownership, and the pre/post lifetime states around `INVOKE`, `DESTROY`, and cleanup edges. A standalone context local is not naturally tied to object lifetime, a phase token adds an unnecessary restore protocol, and a hidden context parameter would alter constructor/destructor ABIs.

**Decision:** rejected. Phase management uses the existing instructions and delegate-state transitions. The active runtime descriptor supplies complete-object-specific context through fixed-index transition groups, and lowering retains an enclosing group pointer only as backend-local SSA across a call. The backend installs descriptors before calls and after state transitions; the constexpr interpreter applies the same semantic transitions. No construction-context instruction, phase-entry instruction, phase token, or hidden context parameter is added.

## Dynamically check nonvirtual `DELETE`

**Pros:** deleting a derived object through a nonvirtual base destructor could produce a defined runtime failure instead of partially destroying an object.

**Cons:** every nonvirtual polymorphic `DELETE` would need an exact-type test or allocation-info lookup even though a valid program already identifies the exact complete type. This charges valid opt-out use for diagnosing a contract violation and prevents the compiler from treating the static object address and layout as authoritative.

**Decision:** rejected. Nonvirtual `DELETE` has an exact-complete-object precondition whose violation is undefined behavior. Native code performs no runtime check; constexpr evaluation rejects a known violation, and the compiler may diagnose a statically evident one.

# Blockers / Open Questions

The following choices should be resolved before the affected implementation stage. They do not block parser and hierarchy-query work unless stated otherwise.

1. **Covariant returns.** The initial rule requires exact return types. Supporting covariant pointer/reference returns requires a second return-adjustment thunk and additional slot validation. Recommendation: keep exact returns for the first implementation.
2. **Empty-base ABI details.** Empty-base optimization is part of the target design, but distinct-address rules for repeated empty bases and zero-sized Quxlang types need precise tests. Recommendation: implement EBO, prohibit nonempty tail-padding reuse initially, and record all occupied `(type, offset)` pairs.
3. **External ABI boundaries.** Passing an inherited struct to `EXTERN` code by value or as a nominal pointer would expose the private layout. Recommendation: reject it until an explicit Quxlang external object ABI is designed; raw address conversion remains a separate unsafe operation.
4. **Public RTTI API.** `STRUCT_TYPE_IS` and type ordinals are initially compiler-internal. A future source feature must decide whether users can ask for exact dynamic type, enumerate bases, or obtain a stable type token. No public API should expose descriptor addresses.
5. **Binary ABI versioning and mangling.** The paired `VIRTUAL_POLYMORPHIC` constructor/destructor entry names, allocation-info prefix, thunk symbols, and descriptor symbols need a mangling/version convention before inherited routines can be exported from separately compiled Quxlang modules. Constructor and destructor ABIs contain no context parameter or phase token.
6. **Pure virtual failure policy.** The plan specifies a defined runtime failure during an active phase. The implementation must select the existing panic path and diagnostic payload without applying the separate nonvirtual-`DELETE` undefined-behavior rule to virtual dispatch.
7. **Nonpolymorphic exact-type serialization order.** Implicit serialization and comparison are available only when the exact struct remains an implicit datatype. Such a hierarchy cannot contain virtual bases. Its generated representation processes direct nonvirtual bases in declaration order and then direct fields; no subobject serialization entry is generated.
