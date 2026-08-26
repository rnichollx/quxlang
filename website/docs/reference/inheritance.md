# Inheritance

Inheritance is a `STRUCT`-only object-model feature. A derived struct contains
base subobjects; inherited lookup and derived-to-base conversions select those
subobjects without copying or slicing them.

!!! warning "JVM backend"
    **Inheritance is not yet implemented on the JVM target.** The Cortado backend
    cannot yet lower inheritance construction, conversion, virtual dispatch,
    RTTI, allocation-info, and destruction operations during compilation.

## Direct base declarations

The accepted forms are:

```quxlang
.selector BASE base_type;
.selector VIRTUAL_BASE base_type;
.BASE base_type;
```

A named declaration introduces a base projection member. An anonymous `.BASE`
is currently permitted only when the struct has exactly one active direct base,
and it currently cannot be virtual. This limitation may be removed in a later
version.

```quxlang
::command_source STRUCT
{
  .command_id VAR I32;

  .CONSTRUCTOR FUNCTION()
  {
  }

  .CONSTRUCTOR FUNCTION(@command_id I32)
    :> .command_id:(@OTHER command_id)
  {
  }
}

::button STRUCT
{
  .source BASE command_source;
  .enabled VAR BOOL;
}
```

`button.source` projects the base subobject. `button.command_id` uses inherited
member lookup and reaches the same object state.

A base declaration is an ordered member-like declaration and may carry `DOC`
or `INCLUDE_IF`. It cannot currently carry declaration privacy. The following hierarchy
constraints are enforced:

- the base must resolve to a concrete `STRUCT` instantiation;
- inheritance cycles are rejected;
- an `IPC_STRUCT` cannot declare a base or be used as one;
- a `FINAL` struct cannot be used as a base;
- selector names cannot collide with members or other direct-base selectors;
- a virtual base must be named;
- one struct cannot declare the same virtual base type twice directly;
- repeated nonvirtual base types require named paths when more than one direct
  base is active.

Inheritance has no public, protected, or private edge modes. Existing
declaration privacy still applies to the selected declaration, and deriving
from a struct grants no additional access.

## Polymorphism categories

An ordinary struct may use nonvirtual inheritance without hidden polymorphic
metadata. Runtime polymorphism is explicit:

| Category | Contract |
| --- | --- |
| no category | Nonvirtual bases, static lookup, and static destruction |
| `STRUCT POLYMORPHIC` | Runtime type identity, virtual functions, RTTI, and nonvirtual base edges |
| `STRUCT VIRTUAL_POLYMORPHIC` | All polymorphic behavior plus virtual bases and the split constructor ABI |

`POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` are mutually exclusive. A struct that
derives from `POLYMORPHIC` must declare a polymorphic category. A struct that
derives from `VIRTUAL_POLYMORPHIC`, or whose hierarchy contains a virtual base,
must be `VIRTUAL_POLYMORPHIC`.

`FINAL` prevents further derivation:

```quxlang
::leaf STRUCT POLYMORPHIC FINAL
{
}
```

## Inherited member lookup

Lookup first considers the static struct's direct declarations and named base
selectors. A direct declaration hides inherited declarations of the same name.
If no direct declaration matches, lookup searches the base graph.

Repeated nonvirtual subobjects remain distinct. When more than one surviving
base path provides a member, unqualified lookup is ambiguous even if the
declarations have different overload signatures. Select a named base first:

```quxlang
::split_command STRUCT
{
  .primary BASE command_source;
  .secondary BASE command_source;
}

VAR command split_command;
command.primary.command_id := 1;
command.secondary.command_id := 2;
```

The same canonical virtual subobject reached through multiple paths is
deduplicated.

To call one declared function body without virtual dispatch, name the declaring
struct and pass `@THIS` explicitly:

```quxlang
RETURN animal::.speak(@THIS value);
```

!!! warning "Semantic Change"
    The ability to bypass virtual dispatch may be removed in a future version.

## Static hierarchy conversions

Pointers and references convert implicitly from a derived struct to a base
when the destination identifies exactly one base subobject. Conversion to a
repeated nonvirtual base is ambiguous until a named base projection selects one
path. Null pointers remain null.

Inheritance does not provide an implicit value conversion and does not slice a
derived object into a base value.

## Virtual functions

Virtual behavior is declared after the function parameter list, alongside a
concrete receiver qualifier:

```quxlang
.read FUNCTION() CONST VIRTUAL: I32
{
  RETURN 0;
}

.read FUNCTION() CONST VIRTUAL(OVERRIDE): I32
{
  RETURN 1;
}
```

The forms are:

| Form | Meaning |
| --- | --- |
| `VIRTUAL` | Introduce a virtual slot |
| `VIRTUAL(OVERRIDE)` | Override at least one inherited slot |
| `VIRTUAL(FINAL)` | Introduce a slot that cannot be overridden |
| `VIRTUAL(OVERRIDE, FINAL)` | Override and prevent another override |
| `VIRTUAL(PURE);` | Introduce an abstract slot without a body |
| `VIRTUAL(OVERRIDE, PURE);` | Replace an inherited slot with an abstract override |

!!! warning "Syntax Change"
    The PURE keyword is likely to be replaced later.

Options are comma-separated and unordered, but cannot be repeated.
`VIRTUAL()` is invalid, and `FINAL` cannot be combined with `PURE`.

A virtual function must use one concrete `THIS` qualifier: `CONST`, `MUT`,
`WRITE`, or `TEMP`. Deduced `AUTO`, `INPUT`, and `OUTPUT` receiver forms cannot
define one fixed runtime slot. Constructors cannot be virtual.

Virtual signatures require exact parameter and return types. Covariant return
types are not implemented. Virtual declarations may use `ENABLE_IF`; its
condition is evaluated before slot introduction, override matching, purity, and
the other virtual-resolution rules. A false declaration contributes no virtual
slot and cannot override one. Virtual functions cannot be templates and cannot
use packs, default arguments, or overload priority. A declaration matching an
inherited slot must say `VIRTUAL(OVERRIDE)`; an override marker that matches no
inherited slot is also rejected.

A type for which any virtual slot selects a pure overrider is abstract. Direct
construction, `PLACE`, and `NEW` of that type are rejected.

During construction and destruction, virtual calls use the active subobject's
phase. A base constructor or base destructor therefore dispatches as that base,
not as a derived portion whose lifetime has not begun or has already ended.

## Constructors and base delegates

Plain and `POLYMORPHIC` structs use `.CONSTRUCTOR`. Direct-base delegates join
the ordinary `:>` list:

```quxlang
::labeled_command STRUCT
{
  .source BASE command_source;
  .label VAR I32;

  .CONSTRUCTOR FUNCTION(%command I32, %label I32)
    :> .source:(@command_id command), .label:(@OTHER label)
  {
  }
}
```

A named base delegate uses its selector. The sole anonymous base uses its base
type as the delegate target. Omitted bases request default construction.

Construction order is semantic rather than written delegate order:

1. virtual bases owned by the complete object;
2. direct nonvirtual bases in declaration order;
3. fields in declaration order;
4. the constructor body.

!!! warning "Semantic Change"
    The ordering above is likely to change in future versions of Quxlang.

### `VIRTUAL_POLYMORPHIC` constructor forms

A `VIRTUAL_POLYMORPHIC` struct has distinct callable constructor entries:

- `.FULLOBJECT_CONSTRUCTOR` owns and constructs reachable virtual bases;
- `.SUBOBJECT_CONSTRUCTOR` constructs the type as a base and does not construct
  virtual bases.

Explicit declarations must occur as a matching pair for each normalized
signature. A subobject constructor cannot contain a `VIRTUAL` base delegate.
Ordinary object construction and `PLACE` select the full-object entry; a base
delegate selects the subobject entry for a `VIRTUAL_POLYMORPHIC` base.

Alternatively, `.CONSTRUCTOR` is accepted as a synthesis template:

```quxlang
::root STRUCT VIRTUAL_POLYMORPHIC
{
  .value VAR I32;

  .CONSTRUCTOR FUNCTION(%value I32)
    :> .value:(@OTHER value)
  {
  }
}

::branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared_root VIRTUAL_BASE root;

  .CONSTRUCTOR FUNCTION(%value I32)
    :> VIRTUAL root:[value]
  {
  }
}
```

Normalization consumes that declaration and generates a full-object and a
subobject constructor. The full-object form retains the `VIRTUAL root`
delegate; the subobject form removes it, including its argument expressions.
The type has no callable `.CONSTRUCTOR` member afterward. 

An explicit pair uses the reserved names directly:

```quxlang
.FULLOBJECT_CONSTRUCTOR FUNCTION(%value I32)
  :> VIRTUAL root:[value]
{
}

.SUBOBJECT_CONSTRUCTOR FUNCTION(%value I32)
{
}
```

Written virtual-base delegates name the canonical virtual-base type rather
than a path selector. The complete object's full constructor initializes each
shared virtual base once.

## Dynamic casts

`AS DYNAMIC` performs an RTTI cast between instance pointers:

```quxlang
VAR base_pointer MUT->animal := object<-;
VAR derived_pointer MUT->dog :=
  base_pointer AS DYNAMIC MUT->dog;
```

The source pointee must be `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`.

- a null source produces null;
- a unique downcast or cross-cast produces a pointer to the target subobject;
- a missing or ambiguous target produces null;
- the pointer category and qualifiers are preserved;
- qualification cannot be discarded.

Dynamic reference casts and a `dynamic_cast<void*>`-style complete-object cast
are not implemented.

!!! warning "Semantic Change"
    Casting null ptr may become undefined behaivor in a future version.

## Destructors and `DELETE`

Both polymorphic categories implicitly provide a virtual destructor. A
user-written `.DESTRUCTOR` is also virtual without spelling `VIRTUAL`.
`NONVIRTUAL` opts out on the destructor declaration:

```quxlang
::exact_owner STRUCT POLYMORPHIC FINAL
{
  .DESTRUCTOR FUNCTION() NONVIRTUAL
  {
  }
}
```

`NONVIRTUAL` is valid only on a destructor of a polymorphic struct. It cannot be
combined with `VIRTUAL`, and it cannot suppress an inherited virtual destructor
slot.

For a virtual destructor, `DELETE` saves the complete allocation information,
dispatches destruction through the runtime type, and deallocates the saved
storage. Deleting a derived object through a polymorphic base pointer therefore
destroys the full object.

For a `NONVIRTUAL` destructor, deletion uses the static type and performs no
dynamic-type check. The pointer must identify a complete object of exactly that
type. Violating this precondition, including deleting a derived object through
the base pointer, is undefined behavior.

Destruction runs the current destructor body, fields in reverse declaration
order, direct nonvirtual bases in reverse declaration order, and—when ending a
complete `VIRTUAL_POLYMORPHIC` object—virtual bases in reverse construction
order.

See [`NEW` and `DELETE`](new-and-delete.md) for the general allocation contract.

## Generated-operation boundary

Inheritance does not make copy, move, or assignment virtual. Their selection
remains based on the static type, and eligible implementations may still be
generated across base subobjects.

A `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` struct is not implicitly a datatype.
The compiler therefore does not generate equality, three-way comparison,
serialization, deserialization, or swap for it. These operations require an
explicit user-defined contract when they are meaningful.

## Related pages

- [Structures](structs-and-members.md)
- [Constructors and Destructors](constructors-and-destructors.md)
- [Conversions](conversions.md)
- [`NEW` and `DELETE`](new-and-delete.md)
- [Target Availability](availability-and-targets.md)
- [Backends and Layout](toolchain/backends-and-layout.md)
