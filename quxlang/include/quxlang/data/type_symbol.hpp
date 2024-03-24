//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD
#define QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD

#include "numeric_literal.hpp"
#include "rpnx/metadata.hpp"
#include "rpnx/variant.hpp"
#include <boost/variant.hpp>
#include <compare>
#include <rpnx/compare.hpp>
#include <rpnx/resolver_utilities.hpp>
#include <rpnx/metadata.hpp>

namespace quxlang
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
    struct selection_reference;

    struct void_type
    {
        RPNX_MEMBER_METADATA(void_type);
    };

    struct numeric_literal_reference
    {
        RPNX_MEMBER_METADATA(numeric_literal_reference);
    };

    struct context_reference
    {
        RPNX_MEMBER_METADATA(context_reference);
    };

    struct template_reference
    {
        std::string name;
        RPNX_MEMBER_METADATA(template_reference, name);
    };

    struct bound_function_type_reference;
    // struct function_type_reference;

    using type_symbol = rpnx::variant< void_type, context_reference, template_reference, module_reference, subentity_reference, primitive_type_integer_reference, primitive_type_bool_reference, instanciation_reference, selection_reference, value_expression_reference, subdotentity_reference, instance_pointer_type, tvalue_reference, mvalue_reference, cvalue_reference, ovalue_reference, bound_function_type_reference, numeric_literal_reference, avalue_reference >;

    struct module_reference
    {
        std::string module_name;

        RPNX_MEMBER_METADATA(module_reference, module_name);
    };

    struct subentity_reference
    {
        type_symbol parent;
        std::string subentity_name;

        RPNX_MEMBER_METADATA(subentity_reference, parent, subentity_name);
    };

    struct subdotentity_reference
    {
        type_symbol parent;
        std::string subdotentity_name;

        RPNX_MEMBER_METADATA(subdotentity_reference, parent, subdotentity_name);
    };

    struct primitive_type_integer_reference
    {
        std::size_t bits;
        bool has_sign;

        RPNX_MEMBER_METADATA(primitive_type_integer_reference, bits, has_sign);
    };

    struct primitive_type_bool_reference
    {
        RPNX_MEMBER_METADATA(primitive_type_bool_reference);
    };

    struct instance_pointer_type
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(instance_pointer_type, target);
    };

    struct array_pointer_type
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(array_pointer_type, target);
    };

    struct arithmetic_pointer_type
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(arithmetic_pointer_type, target);
    };

    struct value_expression_reference
    {
        // TODO: Implement
        RPNX_MEMBER_METADATA(value_expression_reference);
    };

    struct instanciation_reference
    {
        type_symbol callee;
        std::vector< type_symbol > parameters;
        RPNX_MEMBER_METADATA(instanciation_reference, callee, parameters);
    };

    struct selection_reference
    {
        type_symbol callee;
        std::vector< type_symbol > parameters;
        RPNX_MEMBER_METADATA(selection_reference, callee, parameters);
    };

    struct mvalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(mvalue_reference, target);
    };

    struct cvalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(cvalue_reference, target);
    };

    struct ovalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(ovalue_reference, target);
    };

    struct avalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(avalue_reference, target);
    };

    struct tvalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(tvalue_reference, target);
    };

    struct bound_function_type_reference
    {
        type_symbol object_type;
        type_symbol function_type;
        RPNX_MEMBER_METADATA(bound_function_type_reference, object_type, function_type);
    };

    std::string to_string(type_symbol const&);

} // namespace quxlang

#include "quxlang/manipulators/qmanip.hpp"

template <>
struct rpnx::resolver_traits< quxlang::type_symbol >
{
    static std::string stringify(quxlang::type_symbol const& v)
    {
        return quxlang::to_string(v);
    }
};

template <>
struct rpnx::resolver_traits< quxlang::instanciation_reference >
{
    static std::string stringify(quxlang::type_symbol const& v)
    {
        return quxlang::to_string(v);
    }
};

#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD