// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_FUNCTION_HEADER_GUARD
#define QUXLANG_PARSERS_FUNCTION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/peek_symbol.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <iterator>
#include <string>
#include <string_view>

namespace quxlang::parsers
{
    /** Parses one numeric overload ranking modifier for a function or template. */
    inline auto try_parse_overload_priority(parsing_context& ctx, std::optional< std::int64_t >& priority, std::optional< std::int64_t >& weak_priority) -> bool
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        std::optional< std::string_view > keyword = skip_keyword_if_one_of(pos, end, {"PRIORITY", "WEAK_PRIORITY"});
        if (!keyword.has_value())
        {
            return false;
        }
        std::optional< std::int64_t >& destination = *keyword == "PRIORITY" ? priority : weak_priority;
        if (destination.has_value())
        {
            throw syntax_compilation_error("Duplicate " + std::string(*keyword) + " modifier");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after " + std::string(*keyword));
        }
        skip_whitespace_and_comments(pos, end);
        std::string number;
        if (pos != end && *pos == '-')
        {
            number += *pos++;
        }
        while (pos != end && *pos >= '0' && *pos <= '9')
        {
            number += *pos++;
        }
        std::int64_t value = 0;
        std::from_chars_result parsed = std::from_chars(number.data(), number.data() + number.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != number.data() + number.size())
        {
            throw syntax_compilation_error("Expected a signed 64-bit decimal integer in " + std::string(*keyword));
        }
        if (*keyword == "PRIORITY" && (value < std::numeric_limits< std::int32_t >::min() || value > std::numeric_limits< std::int32_t >::max()))
        {
            throw syntax_compilation_error("PRIORITY requires a signed 32-bit integer");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after " + std::string(*keyword) + " integer");
        }
        destination = value;
        return true;
    }

    /** Parses the options following CANCELLABLE for a coroutine invocation. */
    inline auto parse_coroutine_cancellation_mode(parsing_context& ctx) -> coroutine_cancellation_mode
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "(")) return coroutine_cancellation_mode::cooperative;
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "INSTANT")) throw syntax_compilation_error("Expected INSTANT in CANCELLABLE options");
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")")) throw syntax_compilation_error("Expected ')' after CANCELLABLE(INSTANT");
        return coroutine_cancellation_mode::instant;
    }

    template < typename It >
    std::string parse_argument_name(It& pos, It end, bool required = true)
    {
        static constexpr std::array< std::string_view, 66 > argument_keywords = {
            "T", "BITS", "EXPONENT", "ARGUMENT", "CHANNEL", "CLOCK", "ELAPSED", "FIRST", "INPUT", "ITERATIONS", "JSON", "NAME", "OFFSET", "OUTPUT", "REPETITIONS", "START", "STATE", "SIZE", "ALIGN", "PTR", "ADDR", "COUNT", "THIS", "INITVAL", "GENERIC_THIS", "GENERIC_OTHER", "RHS", "LHS", "RETURN", "ARG", "OTHER", "INDEX", "VALUE", "EXPR", "FILE", "LINE", "COLUMN", "TAG", "MESSAGE", "GUARD", "NODE", "DEINITIALIZER", "FRAME", "RECORD", "EXCEPTION", "SUCCESS", "FAILURE", "EXPECTED", "DESIRED", "EXPLICIT", "REINTERPRET", "CHECKED", "ASSUME", "PARTIAL", "APPROXIMATE", "OUTPUT_ITERATOR", "INPUT_ITERATOR", "DESERIALIZE_INPUT_ITERATOR",
            "CONTROLLER", "KIND", "COROUTINE", "FUNCTION", "FUNCTUM", "ARGUMENTS", "DESTINATION", "RECEIVER",
        };

        auto identifier = parse_identifier(pos, end);
        if (identifier.empty())
        {
            auto keyword_end = iter_parse_keyword(pos, end);
            for (auto keyword : argument_keywords)
            {
                if (static_cast< std::size_t >(std::distance(pos, keyword_end)) == keyword.size() && std::equal(pos, keyword_end, keyword.begin(), keyword.end()))
                {
                    pos = keyword_end;
                    return std::string(keyword);
                }
            }
            if (required)
            {
                throw syntax_compilation_error("Expected identifier or keyword");
            }
            return {};
        }

        return identifier;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_FUNCTION_HEADER
