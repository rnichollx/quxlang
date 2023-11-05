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
    struct parameter_set_reference;
    struct value_expression_reference;
    struct pointer_to_reference;
    struct primitive_type_integer_reference;
    struct context_reference
    {
        std::strong_ordering operator<=>(const context_reference& other) const = default;
    };

    using qualified_symbol_reference =
        boost::variant< context_reference, module_reference, boost::recursive_wrapper< subentity_reference >, boost::recursive_wrapper< primitive_type_integer_reference >,
                        boost::recursive_wrapper< parameter_set_reference >, boost::recursive_wrapper< value_expression_reference >, boost::recursive_wrapper< pointer_to_reference > >;

    // TODO: Consider adding absolute_qualified_symbol_reference

    struct module_reference
    {
        std::string module_name;

        std::strong_ordering operator<=>(const module_reference& other) const = default;
    };

    struct subentity_reference
    {
        qualified_symbol_reference parent;
        std::string subentity_name;

        std::strong_ordering operator<=>(const subentity_reference& other) const = default;
    };

    struct primitive_type_integer_reference
    {
        std::size_t bits;
        bool has_sign;

        std::strong_ordering operator<=>(const primitive_type_integer_reference& other) const = default;
    };

    struct primitive_type_bool_reference
    {
        std::strong_ordering operator<=>(const primitive_type_bool_reference& other) const = default;
    };

    struct pointer_to_reference
    {
        qualified_symbol_reference target;

        std::strong_ordering operator<=>(const pointer_to_reference& other) const = default;
    };

    struct value_expression_reference
    {
        // TODO: Implement
        std::strong_ordering operator<=>(const value_expression_reference& other) const = default;
    };

    struct parameter_set_reference
    {
        qualified_symbol_reference callee;
        std::vector< qualified_symbol_reference > parameters;
        std::strong_ordering operator<=>(const parameter_set_reference& other) const = default;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
