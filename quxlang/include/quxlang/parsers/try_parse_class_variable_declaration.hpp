// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_CLASS_VARIABLE_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_CLASS_VARIABLE_DECLARATION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

#include <utility>

namespace quxlang::parsers
{
    inline std::optional< std::tuple< std::string, bool, ast2_variable_declaration > > try_parse_class_variable_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto begin = pos;
        skip_whitespace_and_comments(pos, ctx.iter_end);
        auto trial = ctx;

        bool is_member;

        if (skip_symbol_if_is(trial.iter_pos, trial.iter_end, "."))
        {
            is_member = true;
        }
        else if (skip_symbol_if_is(trial.iter_pos, trial.iter_end, "::"))
        {
            is_member = false;
        }
        else
        {
            return std::nullopt;
        }
        std::string name = parse_identifier(trial.iter_pos, trial.iter_end);

        if (name.empty())
        {
            throw syntax_compilation_error("Expected identifier");
        }

        skip_whitespace(trial.iter_pos, trial.iter_end);

        if (skip_keyword_if_is(trial.iter_pos, trial.iter_end, "STATIC_VAR"))
        {
            throw syntax_compilation_error("STATIC_VAR is only allowed inside function bodies");
        }

        if (!skip_keyword_if_is(trial.iter_pos, trial.iter_end, "VAR"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);

        type_symbol type = try_parse_type_symbol(trial).value();

        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);

        if (!skip_symbol_if_is(trial.iter_pos, trial.iter_end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after VAR type");
        }

        pos = trial.iter_pos;

        ast2_variable_declaration var;
        var.type = std::move(type);
        var.location = ctx.get_location_optional(begin, pos);
        // TOOD: offset, include_if

        return {{std::move(name), is_member, std::move(var)}};
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_VARIABLE_DECLARATION_HPP
