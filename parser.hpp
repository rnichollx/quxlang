//
// Created by Ryan Nicholl on 2/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_PARSER_HEADER
#define RPNX_RYANSCRIPT1031_PARSER_HEADER
#include "ast.hpp"
#include <cctype>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace rs1031
{

    template < typename It >
    struct unexpected_eof
    {
        It pos;
    };

    // parse_* functions should examine the input and return an interator to the end of the parsed position.
    // get_* functions should return a string corresponding to the result of the parse_* function.
    // skip_* functions advance the input parameter to the end of the parsed position.
    // compile_* functions should emit AST information

    // Symbol is e.g. ".." "::", "->", etc.
    template < typename It >
    auto parse_symbol(It begin, It end) -> It;

    template < typename It >
    inline std::string get_symbol(It begin, It end)
    {
        auto pos = parse_symbol(begin, end);
        return std::string(begin, pos);
    }

    template < typename It >
    inline bool skip_symbol(It& begin, It end)
    {
        auto pos = parse_symbol(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

    template < typename It >
    inline bool skip_symbol_if_is(It& begin, It end, std::string_view symbol)
    {
        auto pos = parse_symbol(begin, end);
        if (pos == begin)
        {
            return false;
        }
        if (std::string(begin, pos) == symbol)
        {
            begin = pos;
            return true;
        }
        return false;
    }

    template < typename It >
    auto parse_keyword(It begin, It end) -> It;

    template < typename It >
    inline std::string get_keyword(It begin, It end)
    {
        auto pos = parse_keyword(begin, end);
        return std::string(begin, pos);
    }

    template < typename It >
    inline bool skip_keyword(It& begin, It end)
    {
        auto pos = parse_keyword(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

    template < typename It >
    inline bool skip_keyword_if_is(It& begin, It end, std::string_view keyword)
    {
        auto pos = parse_keyword(begin, end);
        if (pos == begin)
        {
            return false;
        }
        if (std::string(begin, pos) == keyword)
        {
            begin = pos;
            return true;
        }
        return false;
    }

    template < typename It >
    auto parse_identifier(It begin, It end) -> It;

    /// Identifiers may begin with a-z, and contain a-z, 0-9, and _. They may not begin with a number, and they may not begin or end with '_'.
    template < typename It >
    auto parse_identifier(It begin, It end) -> It
    {
        auto pos = begin;
        if (pos == end)
        {
            return pos;
        }
        bool was_underscore = false;
        bool started = true;
        while (pos != end)
        {
            char c = *pos;
            if (c >= 'a' && c <= 'z' || (started && ((c >= '0' && c <= '9') || c == '_')))
            {
                was_underscore = c == '_';
                ++pos;
                started = true;
            }
            else
                break;
        }
        if (was_underscore)
        {
            return begin;
        }
        return pos;
    }

    template < typename It >
    inline std::string get_identifier(It begin, It end)
    {
        auto pos = parse_identifier(begin, end);
        return std::string(begin, pos);
    }

    template < typename It >
    inline bool skip_identifier(It& begin, It end)
    {
        auto pos = parse_identifier(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

    template < typename It >
    inline std::string get_skip_identifier(It& begin, It end)
    {
        auto pos = parse_identifier(begin, end);
        if (pos == begin)
        {
            return {};
        }
        auto result = std::string(begin, pos);
        begin = pos;
        return result;
    }

    template < typename It >
    auto parse_whitespace(It begin, It end)
    {
        while (begin != end && std::isspace(*begin))
        {
            ++begin;
        }
        return begin;
    }

    template < typename It >
    inline bool skip_whitespace(It& begin, It end)
    {
        auto pos = parse_whitespace(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

    template < typename It >
    auto parse_line_comment(It begin, It end) -> It
    {
        if (get_symbol(begin, end) == "//")
        {
            while (begin != end && *begin != '\n' && *begin != '\r')
            {
                ++begin;
            }
        }
        return begin;
    }

    template < typename It >
    inline bool skip_line_comment(It& begin, It end)
    {
        auto pos = parse_line_comment(begin, end);
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

    template < typename It >
    auto parse_symbol(It begin, It end) -> It
    {
        auto pos = begin;
        bool started = false;
        while (pos != end && !std::isspace(*pos) && !std::isalpha(*pos) && !std::isdigit(*pos) &&
               (!started || (*pos != '(' && *pos != ')') && (*pos != '{' && *pos != '}') && (*pos != ',' && *pos != ';')) && *pos != '_')
        {
            char c = *pos;
            ++pos;

            if (c == '(' || c == ')')
            {
                break;
            }
            started = true;
        }

        return pos;
    }

    template < typename It >
    inline auto parse_keyword(It begin, It end) -> It
    {
        bool started = false;
        auto pos = begin;
        while (pos != end && ((*pos == '_') || (*pos >= 'A' && *pos <= 'Z') || (started && (*pos >= '0' && *pos <= '9'))))
        {
            ++pos;
            started = true;
        }
        return pos;
    }

    template < typename It >
    inline bool skip_wsc(It& begin, It end)
    {
        auto pos = begin;
        while (skip_whitespace(pos, end) || skip_line_comment(pos, end))
            ;
        if (pos == begin)
        {
            return false;
        }
        begin = pos;
        return true;
    }

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_PARSER_HEADER
