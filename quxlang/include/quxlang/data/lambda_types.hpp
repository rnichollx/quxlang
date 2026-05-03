// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_LAMBDA_TYPES_HEADER_GUARD
#define QUXLANG_DATA_LAMBDA_TYPES_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <map>
#include <utility>

namespace quxlang
{
    struct lambda_possible_capture
    {
        type_symbol reference_field_type;
        type_symbol value_field_type;

        RPNX_MEMBER_METADATA(lambda_possible_capture, reference_field_type, value_field_type);
    };

    struct lambda_environment
    {
        std::map< std::string, std::size_t > capture_indices;
        std::map< std::string, scoped_definition_v3 > scoped_definitions;
        std::map< static_local_ref, constexpr_static > statics;

        RPNX_MEMBER_METADATA(lambda_environment, capture_indices, scoped_definitions, statics);
    };

    struct lambda_symbol_info
    {
        type_symbol parent_functanoid;
        std::size_t index = 0;
    };

    inline auto lambda_capture_field_name(std::size_t index) -> std::string
    {
        return "__CAPTURE" + std::to_string(index);
    }

    inline auto lambda_closure_name(std::size_t index) -> std::string
    {
        return "__LAMBDA" + std::to_string(index);
    }

    inline auto make_lambda_closure_symbol(type_symbol parent_functanoid, std::size_t index) -> type_symbol
    {
        return subsymbol{.of = std::move(parent_functanoid), .name = lambda_closure_name(index)};
    }

    inline auto parse_lambda_index(std::string_view name) -> std::optional< std::size_t >
    {
        static constexpr std::string_view prefix = "__LAMBDA";
        if (!name.starts_with(prefix))
        {
            return std::nullopt;
        }
        std::size_t index = 0;
        for (char ch : name.substr(prefix.size()))
        {
            if (ch < '0' || ch > '9')
            {
                return std::nullopt;
            }
            index = (index * 10) + static_cast< std::size_t >(ch - '0');
        }
        return index;
    }

    inline auto parse_lambda_closure_symbol(type_symbol const& symbol) -> std::optional< lambda_symbol_info >
    {
        if (!typeis< subsymbol >(symbol))
        {
            return std::nullopt;
        }
        subsymbol const& sub = as< subsymbol >(symbol);
        if (!typeis< instanciation_reference >(sub.of))
        {
            return std::nullopt;
        }
        auto index = parse_lambda_index(sub.name);
        if (!index.has_value())
        {
            return std::nullopt;
        }
        return lambda_symbol_info{.parent_functanoid = sub.of, .index = *index};
    }

    inline auto parse_lambda_operator_symbol(type_symbol const& symbol) -> std::optional< lambda_symbol_info >
    {
        if (!typeis< submember >(symbol))
        {
            return std::nullopt;
        }
        submember const& member = as< submember >(symbol);
        if (member.name != "OPERATOR()")
        {
            return std::nullopt;
        }
        return parse_lambda_closure_symbol(member.of);
    }

    inline auto parse_lambda_constructor_symbol(type_symbol const& symbol) -> std::optional< lambda_symbol_info >
    {
        if (!typeis< submember >(symbol))
        {
            return std::nullopt;
        }
        submember const& member = as< submember >(symbol);
        if (member.name != "CONSTRUCTOR")
        {
            return std::nullopt;
        }
        return parse_lambda_closure_symbol(member.of);
    }
} // namespace quxlang

#endif // QUXLANG_DATA_LAMBDA_TYPES_HEADER_GUARD
