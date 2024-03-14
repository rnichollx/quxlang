# Quxlang Parsers

## X next_* (It begin, It end)

The `next_X` functions return a parsed `X` object without advancing the
iterator.

## X parse_* (It &it, It end)

The `parse_X` functions return a parsed `X` object and advance the iterator.

## It iter_parse_* (It &it, It end)

These functions return an advanced iterator past the structure they parse, 
or the begin iterator if the structure was not found.

## void skip_* (It &it, It end)

The `skip_X` functions advance the iterator without returning a parsed `X`,
they skip any `X` found.

## bool skip_*_if_is (It &it, It end, X expected)

These functions advance the iterator if they found an "expected", they
return true if the thing found is equal to `expected`, and false if nothing
was skipped. (e.g. `skip_keyword_if_is(pos, end, "IF")`).




