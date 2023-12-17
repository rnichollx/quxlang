//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef PARSE_CLASS_BODY_HEADER_GUARD
#define PARSE_CLASS_BODY_HEADER_GUARD

#include <optional>
#include <rylang/ast2/ast2_type_map.hpp>
#include <rylang/parsers/try_parse_class_function_declaration.hpp>
#include <rylang/parsers/try_parse_class_variable_declaration.hpp>

namespace rylang::parsers
{

    template < typename It >
    ast2_class_declaration parse_class_body(It& pos, It end)
    {
        ast2_class_declaration result;
        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::runtime_error("Expected '{'");
        }

    member:

        skip_whitespace_and_comments(pos, end);
        if (auto var = try_parse_class_variable_declaration(pos, end); var)
        {
            result.member_variables.push_back(*var);
            goto member;
        }
        else if (auto func = try_parse_class_function_declaration(pos, end); func)
        {
            result.member_functions.push_back(*func);
            goto member;
        }



        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::runtime_error("Expected '}'");
        }

        return result;
    }
} // namespace rylang::parsers

#endif // PARSE_CLASS_BODY_HEADER_GUARD
