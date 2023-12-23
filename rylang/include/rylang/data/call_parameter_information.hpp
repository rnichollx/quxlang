//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RYLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD
#define RYLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD

#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/ordering.hpp"

namespace rylang
{
    struct call_parameter_information
    {
        std::vector< type_symbol > argument_types;

        std::strong_ordering operator<=>(const call_parameter_information& other) const
        {
            return strong_ordering_from_less(argument_types, other.argument_types);
        }
    };
} // namespace rylang


template<>
struct rpnx::resolver_traits<rylang::call_parameter_information>
{
    static std::string stringify(rylang::call_parameter_information const& v)
    {
        return rpnx::resolver_traits<std::vector< rylang::type_symbol >>::stringify(v.argument_types);
    }
};
#endif // RYLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD
