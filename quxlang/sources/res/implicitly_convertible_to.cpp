// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/manipulators/typeutils.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(implicitly_convertible_to)
{
    type_symbol from = input.from;
    type_symbol to = input.to;

    std::string from_str = quxlang::to_string(from);
    std::string to_str = quxlang::to_string(to);

    if (from == to)
    {
        std::cout << "Convertible: " << from_str << " to " << to_str << " (exact match)" << std::endl;
        co_return true;
    }

    if (remove_ref(to) == remove_ref(from))
    {

        if (is_const_ref(to) && !is_write_ref(from))
        {
            // All value/reference types can be implicitly cast to CONST&
            // except OUT& references
            std::cout << "Convertible: " << from_str << " to " << to_str << " (ref cast)" << std::endl;
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

    if (typeis< bound_type_reference >(from))
    {
        auto from_unbound = as< bound_type_reference >(from).carried_type;
        co_return co_await QUX_CO_DEP(implicitly_convertible_to, (implicitly_convertible_to_query{.from = from_unbound, .to = to}));
    }

    co_return false;
}
