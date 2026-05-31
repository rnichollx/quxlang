# Quxlang Compiler Style Guide


## Naming Conventions

Use `snake_case` for variable names, function names, and file names. C++ files use `.cpp` extension, and header files
use `.hpp` extension.

## Includes

Include Quxlang headers using absolute paths, e.g. `#include <quxlang/some_header.hpp>`. Angle brackets are preferred.
Do not use any relative paths for includes.

## Documentation Comments

Documentation comments should generally be "blackbox doc comments". I.e., if the implementation is deleted, it should be 
reasonably be possible to delete the implementation and then write something very similar using only the doc comment as
a guide. That is to say, documentation should be detailed enough that we can mostly understand the original behavior 
from only documentation comments, without reading any code.

## Design Patterns

### Do not introduce simple `make_*` functions in general

Bad:

```quxlang
    inline auto make_type_instantiation(type_symbol type) -> parameter_instantiation
    {
        return parameter_type_instantiation{.type = std::move(type)};
    }
```

Better:

Use `parameter_type_instantiation{.type = type}` inline.

Borderline:

```
    inline auto make_unlocated_parsing_context(std::string const& input) -> parsing_context
    {
        return parsing_context{
            .source_locations_enabled = false,
            .iter_begin = input.begin(),
            .iter_pos = input.begin(),
            .iter_end = input.end(),
        };
    }
```

### Avoid short functions unless logically needed

Avoid patterns that require jumping around.

Functions should fully describe what they do in doc comments.

If the doc comment would be larger than the code they replace, they probably should not exist unless it encapsulates 
private implementation details. Struct members are not private implementation details, so simple struct manipulation
generally should not produce small manipulator functions.

 
