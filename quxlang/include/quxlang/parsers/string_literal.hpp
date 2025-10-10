// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_STRING_LITERAL_HEADER_GUARD
#define QUXLANG_PARSERS_STRING_LITERAL_HEADER_GUARD
#include <string>

namespace quxlang::parsers
{

    template < typename It >
    std::optional< std::string > try_parse_string_literal(It& pos, It end)
    {
        if (pos == end || *pos != '"')
            return std::nullopt;

        pos++;

        std::string result;

        while (pos != end && *pos != '"')
        {
            if (*pos == '\\')
            {
                pos++;
                if (pos == end)
                    throw std::logic_error("Unexpected end of file in string literal");
                switch (*pos)
                {
                case 'n':
                    result.push_back('\n');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case '0':
                    result.push_back('\0');
                    break;
                case '\\':
                    result.push_back('\\');
                    break;
                case '"':
                    result.push_back('"');
                    break;
                default:
                    throw std::logic_error("Invalid escape sequence in string literal");
                }
            }
            else
            {
                result.push_back(*pos);
            }

            pos++;
        }

        if (pos == end)
            throw std::logic_error("Unexpected end of file in string literal");
        if (*pos != '"')
            throw std::logic_error("Expected '\"' at end of string literal");

        pos++;

        return result;
    }

    template < typename It >
    std::optional< std::byte > try_parse_char_literal(It& pos, It end)
    {
        if (pos == end || *pos != '\'')
            return std::nullopt;

        pos++;

        if (pos == end)
            throw std::logic_error("Unexpected end of file in char literal");

        std::byte result{};

        if (*pos == '\\')
        {
            pos++;
            if (pos == end)
            {
                throw std::logic_error("Unexpected end of file in char literal");
            }
            switch (*pos)
            {
            case 'n':
                result = std::byte('\n');
                break;
            case 't':
                result = std::byte('\t');
                break;
            case 'r':
                result = std::byte('\r');
                break;
            case '0':
                result = std::byte('\0');
                break;
            case '\\':
                result = std::byte('\\');
                break;
            case '\'':
                result = std::byte('\'');
                break;
            default:
                throw std::logic_error("Invalid escape sequence in char literal");
            }
        }
        else
        {
            result = static_cast< std::byte >(*pos);
        }

        pos++;

        if (pos == end)
        {
            throw std::logic_error("Unexpected end of file in char literal");
        }
        if (*pos != '\'')
        {
            throw std::logic_error("Expected '\'' at end of char literal");
        }

        pos++;

        return result;
    }
} // namespace quxlang::parsers

#endif // STRING_LITERAL_HPP