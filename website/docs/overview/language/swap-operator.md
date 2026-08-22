# Overview of the Swap Operator

Use `<->` to exchange two mutable values:

```quxlang
VAR left I32 := 3;
VAR right I32 := 8;

left <-> right;

ASSERT(left == 8);
ASSERT(right == 3);
```

Arrays swap element by element, and structures normally receive a generated
memberwise swap:

```quxlang
VAR first [2]I32 :[1, 2];
VAR second [2]I32 :[7, 9];
first <-> second;
```

A user-defined type can declare `.OPERATOR<->` when it needs different behavior.
Unions and variants swap their active alternatives as well as their payloads.

## Reference

Swap keeps both objects alive; it is not destruction followed by construction.
For generation rules, suppression modifiers, and self-swap behavior, see the
[Swap Operator Reference](../../reference/swap-operator.md).
