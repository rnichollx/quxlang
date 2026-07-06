// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CONTEXT_HPP
#define QUXLANG_CONTEXT_HPP
#include "quxlang/ast2/source_location.hpp"
#include "quxlang/data/compilation_result.hpp"

#include "quxlang/parsers/lexer.hpp"

namespace quxlang::parsers
{
    struct lexer;
    struct parsing_context
    {
        std::uint64_t file_id = {};
        bool source_locations_enabled = false;
        bool parsing_runtime_module = false;
        /// Enables parsing implementation-only subtag symbols in internal symbol-string tests.
        bool allow_internal_subtag_symbols = false;

        parse_iterator iter_begin;
        parse_iterator iter_pos;
        parse_iterator iter_end;

        auto get_location(parse_iterator begin, parse_iterator end) const -> source_location
        {
            source_location loc;

            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, begin);
            loc.end_index = std::distance(iter_begin, end);
            return loc;
        }

        auto get_location(parse_iterator begin) const -> source_location
        {
            source_location loc;
            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, begin);
            return loc;
        }

        auto get_location() const -> source_location
        {
            source_location loc;
            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, iter_pos);
            return loc;
        }

        auto get_location_optional(parse_iterator begin, parse_iterator end) const -> std::optional<source_location>
        {
            if (!source_locations_enabled)
            {
                return std::nullopt;
            }
            return get_location(begin, end);
        }

        auto get_location_optional(parse_iterator begin) const -> std::optional<source_location>
        {
            if (!source_locations_enabled)
            {
                return std::nullopt;
            }
            return get_location(begin);
        }

        auto get_location_optional() const -> std::optional<source_location>
        {
            if (!source_locations_enabled)
            {
                return std::nullopt;
            }
            return get_location();
        }
    };

    inline auto make_unlocated_parsing_context(std::string const& input) -> parsing_context
    {
        return parsing_context{
            .source_locations_enabled = false,
            .iter_begin = input.begin(),
            .iter_pos = input.begin(),
            .iter_end = input.end(),
        };
    }

    template < typename Func >
    auto in_context(std::string const& ctx_name, parsing_context &ctx, Func f)
    {
        auto start_context = ctx;

        try
        {
            return f(ctx);
        }
        catch (compilation_error& err)
        {
            err.traceback.push_back(trace_frame{.trace_context = ctx_name, .location = start_context.get_location_optional()});
            throw;
        }
    }




}; // namespace quxlang::parsers

#endif // QUXLANG_CONTEXT_HPP
