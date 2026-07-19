// Copyright 2023-2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_STRUCT_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_STRUCT_HEADER_GUARD

#include <optional>
#include <quxlang/ast2/ast2_type_map.hpp>

#include <quxlang/parsers/parse_struct_body.hpp>

namespace quxlang::parsers
{
    /** Parses a STRUCT or IPC_STRUCT declaration when the current token begins one. */
    inline std::optional< ast2_struct_declaration > try_parse_struct(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        std::optional< ast2_struct_declaration > out;

        std::optional< std::string_view > const keyword = skip_keyword_if_one_of(pos, end, {"IPC_STRUCT", "STRUCT"});
        if (!keyword.has_value())
        {
            return out;
        }

        skip_whitespace_and_comments(pos, end);

        out = parse_struct_body(ctx);
        out->is_ipc = *keyword == "IPC_STRUCT";
        out->location = ctx.get_location_optional(begin, pos);
        return out;
    }

} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_STRUCT_HEADER_GUARD
