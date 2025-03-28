// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/manipulators/qmanip.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(implicitly_convertible_to)
{
    type_symbol from = input.from;
    type_symbol to = input.to;

    std::string from_str = quxlang::to_string(from);
    std::string to_str = quxlang::to_string(to);



    if (from == to)
    {
        co_return true;
    }

    if (remove_ref(to) == remove_ref(from))
    {

        if (is_const_ref(to) && !is_write_ref(from))
        {
            // All value/reference types can be implicitly cast to CONST&
            // except OUT& references
            co_return true;
        }
        else if (!is_ref(from) && (is_temp_ref(to) || is_write_ref(to)))
        {
            // TODO: Allow ivalue pseudo-type here.

            // We can convert a temporary value into a TEMP& reference
            // implicitly.
            co_return true;
        }
        else if (is_mut_ref(from) && is_write_ref(to))
        {
            // Mutable references can be implicitly cast to output references.
            co_return true;
        }
        else if (!is_ref(to) && !is_write_ref(from))
        {
            co_return true;
        }
    }

    std::string to_type_str = to_string(to);

    std::string from_type_str = to_string(from);

    if (typeis< int_type >(to) && typeis< numeric_literal_reference >(from))
    {
        co_return true;
    }

    co_return false;
}
