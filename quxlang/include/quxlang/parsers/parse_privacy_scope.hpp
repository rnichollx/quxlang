// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_PRIVACY_SCOPE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_PRIVACY_SCOPE_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

#include <optional>
#include <utility>

namespace quxlang::parsers
{
    /** Parses a PRIVATE scope list, leaving the input untouched when PRIVATE is absent. */
    inline std::optional< privacy_scope > try_parse_privacy_scope(parsing_context& ctx)
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        parse_iterator const begin = pos;
        if (!skip_keyword_if_is(pos, end, "PRIVATE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("expected ( after PRIVATE");
        }

        privacy_scope output;
        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            parse_iterator const entry_begin = pos;
            privacy_scope_entry entry;

            if (skip_keyword_if_is(pos, end, "CLASS"))
            {
                entry.kind = privacy_scope_kind::class_;
            }
            else if (skip_keyword_if_is(pos, end, "MODULE"))
            {
                entry.kind = privacy_scope_kind::module;
            }
            else
            {
                entry.kind = privacy_scope_kind::named;
                entry.named_context = parse_type_symbol(ctx);
            }
            entry.location = ctx.get_location_optional(entry_begin, pos);
            output.entries.push_back(std::move(entry));

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("expected , or ) in PRIVATE scope list");
            }
        }

        if (output.entries.empty())
        {
            throw syntax_compilation_error("PRIVATE requires at least one scope");
        }
        output.location = ctx.get_location_optional(begin, pos);
        return output;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_PRIVACY_SCOPE_HEADER_GUARD
