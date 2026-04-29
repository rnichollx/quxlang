// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_LABEL_REFERENCE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_LABEL_REFERENCE_HEADER_GUARD

#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

#include <optional>
#include <string>

namespace quxlang::parsers
{
    inline auto parse_label_reference(parsing_context& ctx) -> std::string
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ":"))
        {
            throw std::logic_error("Expected label reference");
        }

        skip_whitespace_and_comments(pos, end);
        auto label = parse_identifier(pos, end);
        if (label.empty())
        {
            throw std::logic_error("Expected label name after ':'");
        }
        return label;
    }

    inline auto try_parse_label_reference(parsing_context& ctx) -> std::optional< std::string >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto trial = pos;

        skip_whitespace_and_comments(trial, end);
        if (!skip_symbol_if_is(trial, end, ":"))
        {
            return std::nullopt;
        }

        pos = trial;
        skip_whitespace_and_comments(pos, end);
        auto label = parse_identifier(pos, end);
        if (label.empty())
        {
            throw std::logic_error("Expected label name after ':'");
        }
        return label;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_LABEL_REFERENCE_HEADER_GUARD
