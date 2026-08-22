# Assignment Operators

Assignment replaces the value of an existing object. Quxlang spells simple
assignment `:=` so that it is distinct from equality `==`.

## Simple assignment

```quxlang
VAR value I32 := 3;
value := 8;
```

The left operand must provide writable access to an existing object. Assignment
dispatches to `OPERATOR:=` for the left operand's type, with the destination as
`THIS` and the source as `OTHER`. Primitive assignment stores a value of the
same concrete type. Applicable conversions are selected before the operator
call under the ordinary overload rules.

`:=` has a different role inside an object declaration:

```quxlang
VAR copied I32 := value;
```

That syntax initializes a new object by calling a constructor with `OTHER`; it
does not call assignment on an already-live `copied`. See [Variables](variables.md).

## Generated assignment

Structures can receive generated memberwise assignment when their declaration
does not suppress it and every stored member is assignable. Arrays assign
corresponding elements. Unions and variants assign according to their active
alternative and valueless policy.

A type can define `.OPERATOR:=` to replace or extend the generated behavior:

```quxlang
.OPERATOR:= FUNCTION(@OTHER CONST& buffer)
{
  .size := OTHER.size;
}
```

An assignment operator normally returns `VOID`; the mutation is its effect.
`NO_IMPLICIT_ASSIGNMENT` and feature-specific `NO_DEFAULT_ASSIGN` declarations
suppress generated assignment where those modifiers are supported.

## Compound assignment

Compound assignment applies an operator and stores the result back into the
left operand:

```quxlang
value += 2;
value -= 1;
value *= 3;
value /= 2;
value %= 5;
```

The arithmetic forms correspond to `+`, `-`, `*`, `/`, and `%`. Integer and
`BYTE` values also support the bitwise compounds:

```text
#&&=  #||=  #^^=  #&!=  #|!=  #^!=  #^>=  #^<=
#++=  #--=  #+%=  #-%=
```

The shift and rotate compounds take an unsigned pointer-sized count. The other
bitwise compounds use the operand contract specified on
[Bitwise Operators](bitwise-operators.md).

Compound operators are individual operator calls. A user type must provide the
compound operator itself when no built-in overload applies; defining `+` and
`OPERATOR:=` does not by itself promise a synthesized `+=` overload.

## Aliasing and lifetime

Assignment does not begin or end the destination object's lifetime. The object
must already be live, and its assignment implementation is responsible for
handling self-assignment or aliasing allowed by its parameter types. To replace
an object by ending and beginning a lifetime explicitly, use
[Object Storage and Lifetime](typed-storage-and-lifetime.md).

Assignment does not implicitly consume a named source. Copy versus move
construction and reference-category preservation are covered by
[Move Semantics](move-semantics.md).
