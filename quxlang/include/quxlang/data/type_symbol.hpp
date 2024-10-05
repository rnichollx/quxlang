// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD

#include "numeric_literal.hpp"
#include "rpnx/metadata.hpp"
#include "rpnx/variant.hpp"
#include <boost/variant.hpp>
#include <compare>
#include <rpnx/compare.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/resolver_utilities.hpp>
#include <map>
#include <vector>

#include <quxlang/data/fwd.hpp>

namespace quxlang
{

    struct calltype
    {
        std::map< std::string, type_symbol > named;
        std::vector< type_symbol > positional;

        inline auto size() const
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(calltype, named, positional);
    };

    struct declared_parameter
    {
        std::optional<std::string> api_name;
        std::optional<std::string> name;
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(declared_parameter, api_name, name, type, default_value);
    };

    struct parameter_type
    {
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(parameter_type, type, default_value);
    };

    struct paratype
    {
        // The "this" special parameter is replaced with a
        // named parameter with the name "THIS"
        std::vector< parameter_type > positional_parameters;
        std::map< std::string, parameter_type > named_parameters;

        RPNX_MEMBER_METADATA(paratype, positional_parameters, named_parameters)
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
        bool builtin = false;
        calltype call_parameters;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(function_overload, builtin, call_parameters, priority);
    };

    struct overload
    {
        bool builtin = false;
        paratype params;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(overload, builtin, params, priority);
    };

    struct signature
    {
        overload ol;
        std::optional<type_symbol> return_type;

        RPNX_MEMBER_METADATA(signature, ol, return_type);
    };

    struct void_type
    {
        RPNX_EMPTY_METADATA(void_type);
    };

    struct numeric_literal_reference
    {
        RPNX_EMPTY_METADATA(numeric_literal_reference);
    };

    struct context_reference
    {
        RPNX_EMPTY_METADATA(context_reference);
    };

    struct template_reference
    {
        std::string name;
        RPNX_MEMBER_METADATA(template_reference, name);
    };

    struct bound_type_reference;
    // struct function_type_reference;

    struct module_reference
    {
        std::string module_name;

        RPNX_MEMBER_METADATA(module_reference, module_name);
    };

    struct subsymbol
    {
        type_symbol of;
        std::string name;

        RPNX_MEMBER_METADATA(subsymbol, of, name);
    };

    struct submember
    {
        type_symbol of;
        std::string name;

        RPNX_MEMBER_METADATA(submember, of, name);
    };

    struct int_type
    {
        std::size_t bits = 0;
        bool has_sign = false;

        RPNX_MEMBER_METADATA(int_type, bits, has_sign);
    };

    struct bool_type
    {
        RPNX_EMPTY_METADATA(bool_type);
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
        RPNX_EMPTY_METADATA(value_expression_reference);
    };

    struct instantiation_type
    {
        type_symbol callee;

        calltype parameters;
        RPNX_MEMBER_METADATA(instantiation_type, callee, parameters);
    };

    struct selection_reference
    {
        type_symbol templexoid;
        function_overload overload;
        RPNX_MEMBER_METADATA(selection_reference, templexoid, overload);
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

    struct wvalue_reference
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(wvalue_reference, target);
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

    struct nvalue_slot
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(nvalue_slot, target);
    };

    struct dvalue_slot
    {
        type_symbol target;
        RPNX_MEMBER_METADATA(dvalue_slot, target);
    };

    struct bound_type_reference
    {
        type_symbol carried_type;
        type_symbol bound_symbol;
        RPNX_MEMBER_METADATA(bound_type_reference, carried_type, bound_symbol);
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
struct rpnx::resolver_traits< quxlang::instantiation_type >
{
    static std::string stringify(quxlang::type_symbol const& v)
    {
        return quxlang::to_string(v);
    }
};


#include <quxlang/data/expression.hpp>



#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD