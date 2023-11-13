//
// Created by Ryan Nicholl on 10/27/23.
//
#include "rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp"
#include "rylang/manipulators/qmanip.hpp"

void rylang::canonical_type_is_implicitly_convertible_to_resolver::process(compiler* c)
{
    qualified_symbol_reference from = m_from;
    qualified_symbol_reference to = m_to;

    if (from == to)
    {
        set_value(true);
        return;
    }

    if (remove_ref(to) == remove_ref(from))
    {
        if (typeis< cvalue_reference >(to) && !typeis< ovalue_reference >(from))
        {
            // All value/reference types can be implicitly cast to CONST&
            // except OUT& references
            set_value(true);
            return;
        }
        else if (!is_ref(from) && (typeis< tvalue_reference >(to) || typeis< ovalue_reference >(to)))
        {
            // TODO: Allow ivalue pseudo-type here.

            // We can convert a temporary value into a TEMP& reference
            // implicitly.
            set_value(true);
            return;
        }
        else if (typeis< mvalue_reference >(from) && typeis< ovalue_reference >(to))
        {
            // Mutable references can be implicitly cast to output references.
            set_value(true);
            return;
        }
        else if (!is_ref(to) && !typeis< ovalue_reference >(from))
        {
            set_value(true);
            return;
        }
    }

    std::string to_type_str = to_string(to);

    std::string from_type_str = to_string(from);

    set_value(to == from);
}
