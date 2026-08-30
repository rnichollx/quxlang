# `VISIT`

`VISIT` is a statement for type-specializing code over the active payload of a
[`VARIANT` or `INLINE_VARIANT`](variants.md). It does not accept unions or
other value types.

## Syntax

The five forms are:

```text
VISIT variable;
VISIT expression AS name;
VISIT variable { statements }
VISIT expression AS name { statements }
VISIT EXTEND expression AS name;
```

Without `AS`, the subject must be one bare identifier. The identifier is also
the payload binding and shadows the variant variable within the specialized
region. With `AS`, `name` is the payload binding and the subject may be any
expression. `EXTEND` is valid only in the final semicolon form.

The header `AS` is recognized only at the top level of the subject. An `AS`
inside a cast, call, lambda, bracketed expression, comment, or nested braces is
part of the expression rather than the `VISIT` header.

## Specialized region

The semicolon forms specialize the lexical remainder of the current block.
The attached-block forms specialize only their attached block. A continuation
therefore consumes all following sibling statements in its block; nested
continuations are interpreted inside-out.

The subject is evaluated exactly once. The compiler generates the specialized
region once for every non-`VOID` alternative in declaration order, even when a
particular compile-time execution already knows the active alternative. Nested
visits multiply their specializations. Every generated specialization remains
a lowering dependency.

## Payload binding and qualification

Each specialization projects a reference to the active payload. The reference
preserves the subject's usable qualifier: a mutable subject exposes a mutable
payload, while a constant subject exposes only constant access. Mutating a
mutable payload changes the object held by the variant.

An attached-block binding ends when its block joins and the outer lookup is
restored. A continuation binding remains in force through the enclosing
block's lexical remainder.

## Temporary lifetime

The forms have three lifetime policies:

| Form | Evaluation temporaries retained through the specialized region |
| --- | --- |
| `VISIT variable;` | The borrowed variable reference only |
| `VISIT expression AS name;` | The exact final temporary variant, when temporary, and its payload reference |
| `VISIT variable { ... }` | The borrowed variable reference only |
| `VISIT expression AS name { ... }` | Every temporary created while evaluating the expression |
| `VISIT EXTEND expression AS name;` | Every temporary created while evaluating the expression |

The ordinary named continuation discards intermediate evaluation temporaries
before dispatch. It does not copy or move the final variant to extend its
lifetime. The attached named form and `EXTEND` dispatch from the complete
post-evaluation state, so intermediate temporaries remain alive until the
specialized region exits.

Normal lifetime cleanup applies on fallthrough, `RETURN`, `BREAK`, `CONTINUE`,
and a valid `GOTO` out of the region.

## `VOID` alternatives

If the variant declaration contains `VOID`, only the two attached-block forms
are permitted. The `VOID` runtime path skips directly past the attached block.
The compiler does not generate the block as a `VOID` specialization and does
not create a payload binding for that path.

All three continuation forms are rejected for any variant whose declaration
contains `VOID`, regardless of which alternative is active at a particular use.

## Valueless state

Visiting a valueless variant is undefined behavior. The generated valueless
dispatch path terminates as unreachable; it performs no cleanup and has no
successor. Programs must test `value??` or otherwise establish that an
alternative is active before visiting a potentially valueless variant.

## Generated identities and labels

Each alternative expansion has distinct internal identities for lambdas,
static locals, and point labels. Point labels declared inside a visited region
are remapped per specialization, including when visits are nested.

A `GOTO` within a specialization may exit to an outer point label when the
ordinary lifetime-state rules permit that transition. A `GOTO` outside the
visited region cannot enter an alternative-local point label.

Use [`MATCH`](match.md) when alternatives need different source blocks,
guards, union option selection, or explicit valueless handling.
