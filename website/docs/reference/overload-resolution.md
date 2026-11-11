# Overload Resolution

Declarations with the same callable name form an overload set. A call maps its
arguments to each candidate, rejects candidates that cannot accept them, then
selects the unique candidate with the best implemented conversion and binding
fit.

## Overload sets

```quxlang
::select FUNCTION(@ARG:value I32): I32 { RETURN 1; }
::select FUNCTION(@ARG:value I64): I32 { RETURN 2; }
```

User declarations, generated operations, and compiler-provided built-ins may
contribute candidates to the same callable surface. Source order is not a
ranking rule.

## Argument mapping

Before comparing types, the call's named and positional groups are mapped to
the candidate's external parameter names and positional sequence. A candidate
is rejected when it has a named argument accepted by neither an explicit
parameter nor `KWARGS`, too many fixed positional arguments, a missing required
argument, or an incompatible variadic shape.

[Default Arguments](default-arguments.md) fill omitted parameters only for the
candidate that declares them. [Variadic Packs](variadic-packs.md) capture a
remaining positional suffix. A final `@KWARGS ...` captures unmatched named
arguments as a [pack](packs.md#parameter-packs). When ordinary
ranking ties, candidates without `KWARGS` are preferred, followed by candidates
with more explicitly matched named arguments.

Argument expressions retain source evaluation order even when named arguments
are written in a different order from the declaration.

## Candidate viability

After mapping, every argument must initialize or bind its parameter under the
adaptations allowed for that call. Viability accounts for:

- exact value and reference types;
- reference qualifier binding and requalification;
- template and `AUTO(tag)` deduction consistency;
- literal fitting and constant categories;
- permitted objectization or temporary-reference binding;
- user and built-in implicit class conversions;
- receiver qualification for member functions;
- `ENABLE_IF` for the concrete candidate instantiation.

A failure in any required parameter removes the candidate rather than partially
calling it.

## Better-fit ordering for values

For an ordinary pure value source, the implemented preference ladder is:

1. exact concrete value identity;
2. binding the concrete value as `TEMP&`;
3. direct value template matching;
4. `TEMP&` template matching;
5. binding a concrete `CONST&`;
6. `CONST&` template matching;
7. applicable binding-conversion paths;
8. implicit class conversion.

This ordering means a pure value prefers `TEMP& AUTO(...)` over
`CONST& AUTO(...)`. It also means a concrete copy/value match normally outranks
a generic or converting alternative.

## Better-fit ordering for references

For a mutable reference source, the implemented progression prefers:

1. exact reference identity;
2. direct reference template matching;
3. concrete const requalification;
4. requalification with template matching;
5. objectization to a concrete value;
6. objectization with template matching;
7. binding-conversion paths;
8. implicit class conversion.

An existing `TEMP& T` identity match outranks requalification to `CONST& T`.
The same ordering holds between exact and const-requalified templated temporary
references.

`WRITE&` is selected only by calls whose argument can satisfy its pure-output
contract; it is not an alias for an ordinary mutable input reference.

## Literal ordering

Literal-specific categories are considered before broad templates when
applicable:

- `NUMERIC_CONSTANT` outranks a direct generic template match for a numeric
  literal;
- `STRING_CONSTANT` outranks a direct generic template match for a string
  literal;
- null-pointer initialization outranks a direct generic template match for
  `NULL`;
- for a fitting nonnegative integer literal, an unsigned integer fit outranks a
  signed integer fit;
- a fitting signed integer fit outranks floating-point conversion.

Explicit casts let the caller choose a concrete type before overload resolution
when literal inference would otherwise select an unwanted candidate.

## Templates and deduction

Each templated candidate is deduced independently. Repeated `AUTO(tag)` uses
must agree on one type. A positional pack written `AUTO(tag)` requires every
captured element to agree with the same deduction, while plain pack `AUTO`
retains heterogeneous element types.

Deduction that cannot resolve every required template binding removes the
candidate. A successful deduction does not automatically outrank a concrete
candidate; the argument adaptation ranks above determine the fit.

## `PRIORITY` and `WEAK_PRIORITY`

`PRIORITY(integer)` and `WEAK_PRIORITY(integer)` follow a function's parameter
list and precede `ENABLE_IF` and the return type. Each modifier may occur once,
and both modifiers may appear on the same declaration in either order. The
arguments are decimal integer literals with an optional leading minus sign.
`PRIORITY` accepts signed 32-bit values. `WEAK_PRIORITY` accepts signed 64-bit
values. An omitted modifier has value zero.

```quxlang
::choose FUNCTION(@ARG I32) WEAK_PRIORITY(-1): I32 { RETURN 1; }
::choose FUNCTION(@ARG AUTO) WEAK_PRIORITY(10): I32 { RETURN 2; }

ASSERT(choose(7 AS I32) == 1);
```

After viability filtering, the candidates with the greatest `PRIORITY` value
participate in ordinary ranking. Ordinary ranking compares argument adaptations,
then named argument capture, fixed positional parameters, and variadic prefix
length. If multiple candidates survive these rules, the candidates with the
greatest `WEAK_PRIORITY` value survive. A unique survivor is selected. Multiple
survivors with equal weak priority produce an ambiguity error.

Weak priority cannot select a candidate eliminated by any earlier ranking rule.
A candidate that is worse for at least one argument and better for none is
eliminated before weak priority is considered. Weak priority can resolve a tie
between candidates that each provide a better match for different arguments.

These modifiers also apply after a `TEMPLATE(...)` parameter list and before
its `ENABLE_IF` or declaration. Template selection filters viable candidates
by `PRIORITY`, then resolves remaining ties by `WEAK_PRIORITY`. Virtual functions,
declarations matching inherited virtual slots, and interface functions reject
either priority modifier.

## `ENABLE_IF`

`ENABLE_IF(condition)` appears after the parameter list and before the return
type:

```quxlang
::width_class FUNCTION(@ARG:value AUTO(t))
  ENABLE_IF(BITS(t) < 32 AS I32): I32
{
  RETURN 1;
}
```

The condition is evaluated for the candidate instantiation. A false condition
makes that candidate non-viable without removing the declaration from its
scope. This differs from `INCLUDE_IF`, which controls whether the declaration
exists for the target at all.

An invalid `ENABLE_IF` expression for an attempted instantiation is a
constraint failure only where the language's candidate-substitution path can
classify it as such; declarations should write explicit boolean constraints
rather than rely on unrelated failures.

## Member functions and receivers

Member overload selection includes `THIS`. The receiver's mutable, constant,
temporary, or other declared qualifier must satisfy the member's receiver
contract. A better match for explicit arguments does not make a candidate
viable when its receiver cannot bind.

Generated constructors and operators use the same selection machinery, with
their destination slots and reserved argument names included in the candidate
signature.

## Ambiguity and failure

Selection must produce one uniquely better candidate. If no candidate is
viable, the call is ill-formed. If two viable candidates have indistinguishable
best fit and equal weak priority, the call is ambiguous and is also ill-formed.
Declaration order does not break the tie.

See [Call Arguments](call-arguments.md), [References](references.md),
[Conversions](conversions.md), and [Templates](templates-and-value-parameters.md)
for the component rules used by selection.
