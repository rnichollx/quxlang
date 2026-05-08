// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_CLASS_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_CLASS_FUNCTION_DECLARATION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_subentity.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <quxlang/parsers/try_parse_function_declaration.hpp>

#include <utility>

namespace quxlang::parsers
{
    inline std::optional< std::tuple< std::string, bool, ast2_function_declaration > > try_parse_class_function_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
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
        std::string name = parse_subentity(trial.iter_pos, trial.iter_end);

        if (name.empty())
        {
            throw syntax_compilation_error("Expected identifier");
        }

        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);

        if (!skip_keyword_if_is(trial.iter_pos, trial.iter_end, "FUNCTION"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(trial.iter_pos, trial.iter_end);

        auto function_opt = try_parse_function_declaration(trial);
        if (!function_opt)
        {
            return std::nullopt;
        }

        auto function = std::move(*function_opt);

        pos = trial.iter_pos;

        return { { std::move(name), is_member, std::move(function) } };
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
