# Operator Precedence

Postfix operations bind most tightly. Binary groups then bind in the order
shown below, from tightest to loosest.

| Binding order | Forms |
| --- | --- |
| Postfix | calls, `.member`, `[]`, `[&]`, `->`, `<-`, `??`, `?!`, `++`, `--`, `!!`, `#!!` |
| Bitwise | `#&&`, `#&!`, `#^^`, `#|!`, `#||`, `#^>`, `#^<`, `#^!`, `#++`, `#--`, `#+%`, `#-%` |
| Multiplication | `*` |
| Addition and subtraction | `+`, `-` |
| Division and remainder | `/`, `%` |
| Conversion | `AS` with an optional conversion mode |
| Type or option test | `ISA Type`, `IS option` |
| Comparison | `<=>`, `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Logical | `&&`, `&!`, `^^`, `|!`, `||`, `^>`, `^<`, `^!` |
| Assignment and swap | `:=`, `<->`, and compound assignments |

The current grammar gives `*` tighter binding than `+` and `-`, but gives `+`
and `-` tighter binding than `/` and `%`. It also gives bitwise operators
tighter binding than arithmetic. These choices differ from C-family
precedence, so parenthesize mixed expressions when the intended grouping is not
immediately visible:

```quxlang
VAR quotient I32 := (left + right) / divisor;
VAR masked I32 := (value + offset) #&& mask;
VAR selected BOOL := (left < right) && enabled;
```

## Postfix chains

Calls, member selection, indexing, and pointer operations can form one chain:

```quxlang
VAR value I32 := container.item_at(@index index).field;
VAR pointed I32 := pointers[index]->;
```

`AS`, `ISA`, and `IS` bind after a complete higher-precedence operand. Use
parentheses when a cast or fusion test participates in more arithmetic or
member access.

Operator overloads use the same table as built-in operations; a type cannot
define a new precedence. See [User-defined operators](user-defined-operators.md),
[Arithmetic and comparisons](arithmetic-and-comparisons.md),
[Logical and booliation operators](logical-and-booliation.md), and
[Bitwise operators](bitwise-operators.md).
