// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CONTEXT_HPP
#define QUXLANG_CONTEXT_HPP
#include "quxlang/ast2/source_location.hpp"
#include "quxlang/data/compilation_result.hpp"

namespace quxlang::parsers
{
    using parse_iterator = std::string::const_iterator;
    struct parsing_context
    {
        std::uint64_t file_id;

        parse_iterator iter_begin;
        parse_iterator iter_pos;
        parse_iterator iter_end;

        source_location get_location(parse_iterator begin, parse_iterator end)
        {
            source_location loc;

            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, begin);
            loc.end_index = std::distance(iter_begin, end);
            return loc;
        }

        source_location get_location(parse_iterator begin)
        {
            source_location loc;
            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, begin);
            return loc;
        }

        source_location get_location()
        {
            source_location loc;
            loc.file_id = file_id;
            loc.begin_index = std::distance(iter_begin, iter_pos);
            return loc;
        }
    };

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
            err.traceback.push_back(trace_frame{.trace_context = ctx_name, .location = start_context.get_location()});
            throw;
        }
    }




}; // namespace quxlang::parsers

#endif // QUXLANG_CONTEXT_HPP
