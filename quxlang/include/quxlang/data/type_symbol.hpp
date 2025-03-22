// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_DATA_TYPE_SYMBOL_HEADER_GUARD

#include "numeric_literal.hpp"
#include "rpnx/metadata.hpp"
#include "rpnx/variant.hpp"
#include <compare>
#include <map>
#include <rpnx/compare.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/resolver_utilities.hpp>
#include <vector>

#include <quxlang/data/fwd.hpp>

RPNX_ENUM(quxlang, overload_class, std::uint16_t, user_defined, builtin, intrinsic);

RPNX_ENUM(quxlang, qualifier, std::uint16_t, mut, constant, temp, write, auto_, input, output);
RPNX_ENUM(quxlang, pointer_class, std::uint16_t, instance, array, machine, ref);

namespace quxlang
{
    std::optional< pointer_class > pointer_class_template_match(pointer_class template_class, pointer_class match_class);
    std::optional< qualifier > qualifier_template_match(qualifier template_qual, qualifier match_qual);

    std::optional< qualifier > qualifier_template_match_noconv(qualifier template_qual, qualifier match_qual);

    struct void_type
    {
        RPNX_EMPTY_METADATA(void_type);
    };

    struct thistype
    {
        RPNX_EMPTY_METADATA(thistype);
    };

    struct invotype
    {
        std::map< std::string, type_symbol > named;
        std::vector< type_symbol > positional;

        inline auto size() const
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(invotype, named, positional);
    };

    struct declared_parameter
    {
        std::optional< std::string > api_name;
        std::optional< std::string > name;
        type_symbol type;
        std::optional< expression > default_value;

        RPNX_MEMBER_METADATA(declared_parameter, api_name, name, type, default_value);
    };

    struct declared_parameters
    {
        std::vector< declared_parameter > positional;
        std::map< std::string, declared_parameter > named;

        RPNX_MEMBER_METADATA(declared_parameters, positional, named);
    };

    struct temploid_header
    {
        declared_parameters params;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(temploid_header, params, priority);
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
        std::vector< parameter_type > positional;
        std::map< std::string, parameter_type > named;

        RPNX_MEMBER_METADATA(paratype, positional, named)
    };

    struct param_names
    {
        std::vector< std::optional< std::string > > positional;
        std::map< std::string, std::string > named;

        RPNX_MEMBER_METADATA(param_names, positional, named);
    };

    struct function_arg
    {
        std::string name;
        std::optional< std::string > api_name;
        type_symbol type;

        RPNX_MEMBER_METADATA(function_arg, name, api_name, type)
    };

    struct argif
    {
        type_symbol type;
        bool is_defaulted = false;

        RPNX_MEMBER_METADATA(argif, type, is_defaulted);
    };

    // Interface type - used in things like overload resolution
    struct intertype
    {
        // Positional arguments of the intertype (nameless)
        std::vector< argif > positional;

        // The named arguments of the intertype (e.g. @foo, @bar, @THIS, etc.)
        std::map<std::string, argif > named;

        RPNX_MEMBER_METADATA(intertype, positional, named);
    };

    // Ensig is the portion #[...]
    struct temploid_ensig
    {
        intertype interface;
        std::optional< std::int32_t > priority;

        RPNX_MEMBER_METADATA(temploid_ensig, interface, priority);
    };




    struct overload
    {
        bool builtin = false;
        invotype params;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(overload, builtin, params, priority);
    };

    struct signature
    {
        temploid_ensig ensig;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(signature, ensig, return_type);
    };

    struct sigtype
    {
        invotype params;
        std::optional< type_symbol > return_type;

        RPNX_MEMBER_METADATA(sigtype, params, return_type);
    };

    struct numeric_literal_reference
    {
        RPNX_EMPTY_METADATA(numeric_literal_reference);
    };

    struct context_reference
    {
        RPNX_EMPTY_METADATA(context_reference);
    };

    struct array_type
    {
        type_symbol element_type;
        expression element_count;

        RPNX_MEMBER_METADATA(array_type, element_type, element_count);
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

    struct pointer_type
    {
        type_symbol target;
        pointer_class ptr_class;
        qualifier qual;

        RPNX_MEMBER_METADATA(pointer_type, target, ptr_class, qual);
    };

    struct value_expression_reference
    {
        // TODO: Implement
        RPNX_EMPTY_METADATA(value_expression_reference);
    };



    struct initialization_reference
    {
        type_symbol initializee;

        invotype parameters;
        RPNX_MEMBER_METADATA(initialization_reference, initializee, parameters);
    };



    struct temploid_reference
    {
        type_symbol templexoid;
        temploid_ensig which;
        RPNX_MEMBER_METADATA(temploid_reference, templexoid, which);
    };

    struct instanciation_reference
    {
        temploid_reference temploid;
        invotype params;
        RPNX_MEMBER_METADATA(instanciation_reference, temploid, params);
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

    std::optional< type_symbol > func_class(type_symbol const& func);

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
struct rpnx::resolver_traits< quxlang::initialization_reference >
{
    static std::string stringify(quxlang::type_symbol const& v)
    {
        return quxlang::to_string(v);
    }
};

#include <quxlang/data/expression.hpp>

#endif // QUXLANG_QUALIFIED_SYMBOL_REFERENCE_HEADER_GUARD