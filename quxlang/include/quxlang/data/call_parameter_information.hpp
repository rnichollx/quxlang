//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD
#define QUXLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD

#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/ordering.hpp"

namespace quxlang
{
    struct call_parameter_information
    {
        std::vector< type_symbol > argument_types;

        std::strong_ordering operator<=>(const call_parameter_information& other) const
        {
            return strong_ordering_from_less(argument_types, other.argument_types);
        }
    };
} // namespace quxlang


template<>
struct rpnx::resolver_traits<quxlang::call_parameter_information>
{
    static std::string stringify(quxlang::call_parameter_information const& v)
    {
        return rpnx::resolver_traits<std::vector< quxlang::type_symbol >>::stringify(v.argument_types);
    }
};
#endif // QUXLANG_CALL_PARAMETER_INFORMATION_HEADER_GUARD
