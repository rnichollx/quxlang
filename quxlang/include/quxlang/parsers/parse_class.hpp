// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_CLASS_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_CLASS_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <utility>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/parsers/try_parse_class.hpp>

namespace quxlang::parsers
{
    inline ast2_class_declaration parse_class(parsing_context& ctx)
    {
        auto result = try_parse_class(ctx);
        if (!result)
        {
            throw syntax_compilation_error("Expected class");
        }
        return std::move(*result);
    }
}

#endif //PARSE_CLASS_HPP
