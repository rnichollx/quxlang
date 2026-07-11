// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_STRUCT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_STRUCT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <utility>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/parsers/try_parse_struct.hpp>

namespace quxlang::parsers
{
    /** Parses a required STRUCT declaration. */
    inline ast2_struct_declaration parse_struct(parsing_context& ctx)
    {
        auto result = try_parse_struct(ctx);
        if (!result)
        {
            throw syntax_compilation_error("Expected struct");
        }
        return std::move(*result);
    }
}

#endif // QUXLANG_PARSERS_PARSE_STRUCT_HEADER_GUARD
