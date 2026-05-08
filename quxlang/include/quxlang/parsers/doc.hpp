// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_DOC_HEADER_GUARD
#define QUXLANG_PARSERS_DOC_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>

#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    inline std::string dedent_doc(std::string doc)
    {
        auto close_begin = doc.end();
        auto line_begin = close_begin;
        while (line_begin != doc.begin() && *(line_begin - 1) != '\n')
        {
            --line_begin;
        }

        std::string indent(line_begin, close_begin);
        if (indent.empty())
        {
            return doc;
        }

        for (char c : indent)
        {
            if (c != ' ' && c != '\t')
            {
                return doc;
            }
        }

        std::string out;
        out.reserve(doc.size());

        auto line_pos = doc.begin();
        while (line_pos != doc.end())
        {
            auto line_end = line_pos;
            while (line_end != doc.end() && *line_end != '\n')
            {
                ++line_end;
            }

            if (static_cast< std::size_t >(std::distance(line_pos, line_end)) >= indent.size() && std::equal(indent.begin(), indent.end(), line_pos))
            {
                line_pos += static_cast< std::ptrdiff_t >(indent.size());
            }

            out.append(line_pos, line_end);
            if (line_end != doc.end())
            {
                out.push_back('\n');
                line_pos = line_end + 1;
            }
            else
            {
                line_pos = line_end;
            }
        }

        return out;
    }

    inline std::optional< std::string > try_parse_doc(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        if (!skip_keyword_if_is(pos, end, "DOC"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "<$"))
        {
            throw syntax_compilation_error("expected <$ after DOC");
        }

        auto doc_begin = pos;
        while (pos != end)
        {
            if (*pos == '$')
            {
                auto close_begin = pos;
                if (skip_symbol_if_is(pos, end, "$>"))
                {
                    std::string out(doc_begin, close_begin);
                    out = dedent_doc(std::move(out));
                    skip_whitespace_and_comments(pos, end);
                    return out;
                }
                ++pos;
            }
            else
            {
                ++pos;
            }
        }

        throw syntax_compilation_error("unexpected end of file in DOC block");
    }

    inline std::optional< std::string > try_parse_single_doc(parsing_context& ctx)
    {
        auto out = try_parse_doc(ctx);

        if (out && try_parse_doc(ctx))
        {
            throw syntax_compilation_error("only one DOC block is allowed per declaration");
        }

        return out;
    }

} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_DOC_HEADER_GUARD
