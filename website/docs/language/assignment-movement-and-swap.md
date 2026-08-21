# Assignment, Movement, and Swap

Assignment uses `:=`; equality uses `==`.

```quxlang
value := 10;
value += 2;
value -= 1;
value *= 3;
value /= 2;
value %= 5;
```

The compound forms apply the corresponding operator and assign its result.
Bitwise compounds use the spellings documented on
[Bitwise operators](bitwise-operators.md).

## Swap

`<->` exchanges two compatible objects:

```quxlang
left <-> right;
```

User-defined types can provide `.OPERATOR<->`; otherwise the type's generated
swap behavior applies when available.

## Increment and decrement

```quxlang
counter++;
counter--;
iterator++;
iterator--;
```

These are suffix operations. For an array pointer they advance or retreat one
element.

## Movement and forwarding

Object movement is expressed through `TEMP&` conversion constructors and
`FORWARD(reference)`, not by treating every assignment as an implicit move:

```quxlang
::relay FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

See [Constructors and destructors](constructors-and-destructors.md) for copy and
move conversion categories.

