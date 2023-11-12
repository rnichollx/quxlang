//
// Created by Ryan Nicholl on 10/27/23.
//
#include "rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp"

void rylang::canonical_type_is_implicitly_convertible_to_resolver::process(compiler* c)
{
    // For now, don't allow implicit conversions
    set_value(to == from);
}
