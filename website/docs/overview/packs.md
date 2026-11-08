# Packs

Packs assemble arguments while preserving each argument's value category.
Their fields are independent values, so forwarding an owned field can initialize
a parameter without constructing an intermediate record or invoking a move
constructor.

```quxlang
::make_owned TEMPLATE(@T TYPE AUTO)
  FUNCTION(%...values TT, @KWARGS ...): std::unique_ptr#T
{
  RETURN std::unique_ptr#T(@pointer NEW T:(
    ...FORWARD PACK(values), ...FORWARD PACK(KWARGS)));
}
```

A local pack uses `PACK` and a `...{}` or `...[]` literal. Ordinary expansion
preserves the pack; `FORWARD PACK` consumes it.

```quxlang
PACK arguments ...{ .value = I32(@OTHER 42) };
inspect(...arguments);
FORWARD PACK(arguments) TO consume;
```

Fields can be extracted and recombined without construction. This supports
wrappers that select options, add defaults, or route arguments to several calls.

```quxlang
PACK arguments ...{ .value = 42, .limit = 100 };
PACK options FISSION FIELDS("limit") FROM (arguments);
VAR limit EMIT FIELD("limit") FROM (options);
CAPTURE(limit) NAMED("maximum") INTO(arguments);
FORWARD PACK(arguments) TO clamp;
```

Nested packs support the same ownership operations. An extracted nested pack
can be forwarded or divided further. An ordinary [composite](composites.md)
provides generated struct storage when a structural value is required.

The [pack reference](../reference/packs.md) specifies declaration, inspection,
forwarding, and lifetime rules.
