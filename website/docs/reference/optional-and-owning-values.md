# Optional and Owning Values

These templates are library types in the `std` module. Import it with
`IMPORT std;`. Their construction, forwarding, and operators use ordinary
Quxlang language rules.

## `std::optional#T`

An optional stores either no value or one inline `T`.

| Operation | Effect |
| --- | --- |
| Default construction or construction from `NULL` | Empty optional; no `T` is constructed. |
| Construction from `@value value` or an implicitly accepted `T` | Construct an engaged value, copying or moving as appropriate. |
| `has_value()`, postfix `??` | Test engagement. |
| `value()`, postfix `->` | Reference to the value with the receiver's access qualification; requires engagement. |
| `reset()` or assignment of `NULL` | Destroy the value and become empty. |
| `emplace(...)` | Destroy the old value, forward positional/named arguments to `T`, and return `MUT& T`. |

Copying copies the contained value when engaged. Moving moves that value and
preserves the source's engagement. Self-move assignment preserves the optional.
If `emplace` construction throws, the optional remains empty. References to a
contained value must not outlive that value's lifetime.

`std::make_optional_value#T(value)` constructs an engaged optional by forwarding
its argument to the value constructor. Postfix `!?` is the negation of `??`.

## `std::unique_ptr#T`

An owning pointer is move-only and owns one `T` allocated by `NEW`.

| Operation | Effect |
| --- | --- |
| Default construction | Empty owner. |
| Construction with `@pointer pointer` | Adopt a compatible `NEW T` allocation or null. |
| Move construction/assignment | Transfer ownership and empty the source; assignment deletes the previous object. |
| `get()` | Borrow the stored `MUT->T` without transferring ownership. |
| `has_value()`, postfix `??` | Test whether an object is owned. |
| Postfix `->` | `MUT& T` to the owned object; requires a nonempty owner. |
| `release()` | Return the pointer and empty the owner without deleting. |
| `reset()` | Delete the object and become empty. |
| `reset(@pointer pointer)` | Replace ownership, deleting the old object; the same pointer preserves ownership. |
| `<->` | Exchange ownership. |

The caller relinquishes ownership when adopting a pointer and takes deletion
responsibility after `release()`. Borrowed pointers and references require the
owned object to remain alive. A constant owner still provides mutable access
to `T` through its declared dereference operation.

`std::make_unique#T(...)` allocates with `NEW` and forwards positional and named
constructor arguments. Destruction deletes the object if present.

See [Overview Examples](../overview/optional-and-owning-values.md),
[Move Semantics](move-semantics.md), and [NEW and DELETE](new-and-delete.md).
