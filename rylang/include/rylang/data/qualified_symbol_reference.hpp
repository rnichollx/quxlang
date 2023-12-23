//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef RYLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
#define RYLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD

#include "numeric_literal.hpp"
#include <boost/variant.hpp>
#include <compare>
#include <rpnx/resolver_utilities.hpp>

namespace rylang
{
    struct module_reference;
    struct subentity_reference;
    struct subdotentity_reference;
    struct instanciation_reference;
    struct value_expression_reference;
    struct instance_pointer_type;
    struct primitive_type_integer_reference;
    struct primitive_type_bool_reference;
    struct mvalue_reference;
    struct tvalue_reference;
    struct ovalue_reference;
    struct cvalue_reference;
    struct avalue_reference;

    struct void_type
    {
        std::strong_ordering operator<=>(const void_type& other) const = default;
    };

    struct numeric_literal_reference
    {
        std::strong_ordering operator<=>(const numeric_literal_reference& other) const = default;
    };

    struct context_reference
    {
        std::strong_ordering operator<=>(const context_reference& other) const = default;
    };

    struct template_reference
    {
        std::string name;
        std::strong_ordering operator<=>(const template_reference& other) const = default;
    };

    struct bound_function_type_reference;
    // struct function_type_reference;

    using type_symbol = boost::variant< void_type, context_reference, template_reference, module_reference, boost::recursive_wrapper< subentity_reference >, boost::recursive_wrapper< primitive_type_integer_reference >, boost::recursive_wrapper< primitive_type_bool_reference >, boost::recursive_wrapper< instanciation_reference >, boost::recursive_wrapper< value_expression_reference >, boost::recursive_wrapper< subdotentity_reference >, boost::recursive_wrapper< instance_pointer_type >, boost::recursive_wrapper< tvalue_reference >, boost::recursive_wrapper< mvalue_reference >, boost::recursive_wrapper< cvalue_reference >, boost::recursive_wrapper< ovalue_reference >, boost::recursive_wrapper< bound_function_type_reference >, boost::recursive_wrapper< numeric_literal_reference >, boost::recursive_wrapper< avalue_reference > >;

    struct module_reference
    {
        std::string module_name;

        std::strong_ordering operator<=>(const module_reference& other) const = default;
    };

    struct subentity_reference
    {
        type_symbol parent;
        std::string subentity_name;

        std::strong_ordering operator<=>(const subentity_reference& other) const = default;
    };

    struct subdotentity_reference
    {
        type_symbol parent;
        std::string subdotentity_name;

        std::strong_ordering operator<=>(const subdotentity_reference& other) const = default;
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

    struct instance_pointer_type
    {
        type_symbol target;

        std::strong_ordering operator<=>(const instance_pointer_type& other) const = default;
    };

    struct array_pointer_type
    {
        type_symbol target;
        std::strong_ordering operator<=>(const array_pointer_type& other) const = default;
    };

    struct arithmetic_pointer_type
    {
        type_symbol target;
        std::strong_ordering operator<=>(const arithmetic_pointer_type& other) const = default;
    };

    struct value_expression_reference
    {
        // TODO: Implement
        std::strong_ordering operator<=>(const value_expression_reference& other) const = default;
    };

    struct instanciation_reference
    {
        type_symbol callee;
        std::vector< type_symbol > parameters;
        std::strong_ordering operator<=>(const instanciation_reference& other) const = default;
    };

    struct mvalue_reference
    {
        type_symbol target;
        std::strong_ordering operator<=>(const mvalue_reference& other) const = default;
    };

    struct cvalue_reference
    {
        type_symbol target;
        std::strong_ordering operator<=>(const cvalue_reference& other) const = default;
    };
    struct ovalue_reference
    {
        type_symbol target;
        std::strong_ordering operator<=>(const ovalue_reference& other) const = default;
    };

    struct avalue_reference
    {
        type_symbol target;
        std::strong_ordering operator<=>(const avalue_reference& other) const = default;
    };

    struct tvalue_reference
    {
        type_symbol target;
        std::strong_ordering operator<=>(const tvalue_reference& other) const = default;
    };

    struct bound_function_type_reference
    {
        type_symbol object_type;
        type_symbol function_type;
        std::strong_ordering operator<=>(const bound_function_type_reference& other) const = default;
    };

    std::string to_string(type_symbol const&);

} // namespace rylang

#include "rylang/manipulators/qmanip.hpp"

template<>
struct rpnx::resolver_traits<rylang::type_symbol>
{
    static std::string stringify(rylang::type_symbol const& v)
    {
        return rylang::to_string(v);
    }
};

template<>
struct rpnx::resolver_traits<rylang::instanciation_reference>
{
    static std::string stringify(rylang::type_symbol const& v)
    {
        return rylang::to_string(v);
    }
};



#endif // RYLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
