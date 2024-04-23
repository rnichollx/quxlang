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
#include <rpnx/metadata.hpp>
#include <rpnx/resolver_utilities.hpp>

#include <quxlang/data/fwd.hpp>

namespace quxlang
{

    struct call_type
    {
        std::optional< type_symbol > this_parameter;
        std::vector< type_symbol > positional_parameters;
        std::map< std::string, type_symbol > named_parameters;

        RPNX_MEMBER_METADATA(call_type, this_parameter, positional_parameters, named_parameters);
    };

    struct parameter
    {
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(parameter, type, default_value);
    };

    // TODO: Replace use of call_type in temploid parameters with this type
    struct temploid_parameters
    {
        // The "this" special parameter is replaced with a
        // named parameter with the name "THIS"
        std::vector< parameter > positional_parameters;
        std::map< std::string, parameter > named_parameters;

        RPNX_MEMBER_METADATA(temploid_parameters, positional_parameters, named_parameters)
    };

    struct function_arg
    {
        std::string name;
        std::optional< std::string > api_name;
        type_symbol type;

        RPNX_MEMBER_METADATA(function_arg, name, api_name, type)
    };

    // TODO: Rename this to temploid_header or something,
    //  it is called "function header" but is also used for templates...
    struct function_overload
    {
        call_type call_parameters;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(function_overload, call_parameters, priority);
    };

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

        call_type parameters;
        RPNX_MEMBER_METADATA(instanciation_reference, callee, parameters);
    };

    struct selection_reference
    {
        type_symbol callee;
        function_overload overload;
        RPNX_MEMBER_METADATA(selection_reference, callee, overload);
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