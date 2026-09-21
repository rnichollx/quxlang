// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_IMPORT_HEADER_GUARD
#define QUXLANG_PARSERS_IMPORT_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/include_if.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <optional>
#include <string>
#include <utility>

namespace quxlang::parsers
{
    /** Parses a module-private import alias, retaining its optional inclusion condition. */
    inline auto try_parse_import_declaration(parsing_context& ctx, std::optional< std::string > declared_name)
        -> std::optional< global_subdeclaroid >
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        parse_iterator begin = pos;
        std::optional< expression > condition = try_parse_include_if(ctx, "IMPORT_IF");
        if (!condition.has_value() && !skip_keyword_if_is(pos, end, "IMPORT"))
        {
            return std::nullopt;
        }
        skip_whitespace_and_comments(pos, end);
        std::string module_name = parse_identifier(pos, end);
        if (module_name.empty())
        {
            throw syntax_compilation_error("Expected module name after import keyword");
        }
        skip_whitespace_and_comments(pos, end);
        std::string name = declared_name.value_or(module_name);
        if (skip_keyword_if_is(pos, end, "AS"))
        {
            if (declared_name.has_value())
            {
                throw syntax_compilation_error("AS cannot rename an import with an explicit declaration name");
            }
            skip_whitespace_and_comments(pos, end);
            name = parse_identifier(pos, end);
            if (name.empty())
            {
                throw syntax_compilation_error("Expected import name after AS");
            }
            skip_whitespace_and_comments(pos, end);
        }
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after import declaration");
        }
        ast2_alias_declaration declaration{.target = absolute_module_reference{.module_name = std::move(module_name)}};
        declaration.location = ctx.get_location_optional(begin, pos);
        return global_subdeclaroid{
            .decl = std::move(declaration),
            .name = std::move(name),
            .include_if = std::move(condition),
            .privacy = privacy_scope{.entries = {privacy_scope_entry{.kind = privacy_scope_kind::module}}},
            .location = ctx.get_location_optional(begin, pos),
        };
    }
}

#endif // QUXLANG_PARSERS_IMPORT_HEADER_GUARD
