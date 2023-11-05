//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER

#include <boost/variant.hpp>
#include <compare>

namespace rylang
{
    struct module_reference;
    struct subentity_reference;
    struct primitive_type_reference;
    struct parameter_set_reference;
    struct value_expression_reference;
    struct pointer_to_reference;

    using qualified_type_reference =
        boost::variant< module_reference, boost::recursive_wrapper< subentity_reference >, boost::recursive_wrapper< primitive_type_reference >, boost::recursive_wrapper< parameter_set_reference >,
                        boost::recursive_wrapper< value_expression_reference >, boost::recursive_wrapper< pointer_to_reference > >;

    struct module_reference
    {
        std::string module_name;

        std::strong_ordering operator<=>(const module_reference& other) const = default;
    };

    struct subentity_reference
    {
        qualified_type_reference parent;
        std::string subentity_name;

        std::strong_ordering operator<=>(const subentity_reference& other) const = default;
    };

    struct primitive_type_reference
    {
        std::string type_name;

        std::strong_ordering operator<=>(const primitive_type_reference& other) const = default;
    };

    struct pointer_to_reference
    {
        qualified_type_reference target;

        std::strong_ordering operator<=>(const pointer_to_reference& other) const = default;
    };

    struct value_expression_reference
    {
        // TODO: Implement
        std::strong_ordering operator<=>(const value_expression_reference& other) const = default;
    };

    struct parameter_set_reference
    {
        qualified_type_reference callee;
        std::vector< qualified_type_reference > parameters;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
