# Conversions



## Conversion hierarchy


### From value

Rank 1: Identity conversion: `t -> t`
Rank 2: Materialize to `TEMP&`: `t -> TEMP& t`
Rank 3: Direct templating match: `t -> [template match]`
Rank 4: Materializing to temp templating match: `t -> TEMP& T -> [template match]`
Rank 5: Direct materialization to `CONST`: `t -> CONST& t`
Rank 6: Templating materialization to `CONST`: `t -> CONST& t -> [template match]`
Rank 7: Other binding conversion (not implemented)
Rank 8: Implicit conversion

### From reference

