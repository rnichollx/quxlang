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
    struct primitive_type_bool_reference;
    struct mvalue_reference;
    struct tvalue_reference;
    struct ovalue_reference;
    struct cvalue_reference;

    struct void_type
    {
        std::strong_ordering operator<=>(const void_type& other) const = default;
    };
    struct context_reference
    {
        std::strong_ordering operator<=>(const context_reference& other) const = default;
    };

    struct bound_function_type_reference;
    //struct function_type_reference;

    using qualified_symbol_reference =
        boost::variant< void_type, context_reference, module_reference, boost::recursive_wrapper< subentity_reference >, boost::recursive_wrapper< primitive_type_integer_reference >,
                        boost::recursive_wrapper< primitive_type_bool_reference >, boost::recursive_wrapper< parameter_set_reference >, boost::recursive_wrapper< value_expression_reference >,
                        boost::recursive_wrapper< pointer_to_reference >, boost::recursive_wrapper< tvalue_reference >, boost::recursive_wrapper< mvalue_reference >,
                        boost::recursive_wrapper< cvalue_reference >, boost::recursive_wrapper< ovalue_reference >, boost::recursive_wrapper< bound_function_type_reference> >;



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

    struct mvalue_reference
    {
        qualified_symbol_reference target;
        std::strong_ordering operator<=>(const mvalue_reference& other) const = default;
    };

    struct cvalue_reference
    {
        qualified_symbol_reference target;
        std::strong_ordering operator<=>(const cvalue_reference& other) const = default;
    };
    struct ovalue_reference
    {
        qualified_symbol_reference target;
        std::strong_ordering operator<=>(const ovalue_reference& other) const = default;
    };

    struct tvalue_reference
    {
        qualified_symbol_reference target;
        std::strong_ordering operator<=>(const tvalue_reference& other) const = default;
    };

    struct bound_function_type_reference
    {
        qualified_symbol_reference object_type;
        qualified_symbol_reference function_type;
        std::strong_ordering operator<=>(const bound_function_type_reference& other) const = default;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
