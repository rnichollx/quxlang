// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BACKENDS_ASM_SYMBOL_FORMAT_HEADER_GUARD
#define QUXLANG_BACKENDS_ASM_SYMBOL_FORMAT_HEADER_GUARD

#include <string>

namespace quxlang
{
    /**
     * Formats one symbol name for LLVM integrated assembler directives, labels, and operands.
     */
    inline auto format_asm_symbol_name(std::string const& name) -> std::string
    {
        auto is_symbol_start = [](char const ch) -> bool
        {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '.' || ch == '$';
        };
        auto is_symbol_body = [&](char const ch) -> bool
        {
            return is_symbol_start(ch) || (ch >= '0' && ch <= '9');
        };

        if (!name.empty() && is_symbol_start(name.front()))
        {
            bool plain_symbol = true;
            for (char const ch : name)
            {
                if (!is_symbol_body(ch))
                {
                    plain_symbol = false;
                    break;
                }
            }
            if (plain_symbol)
            {
                return name;
            }
        }

        std::string output = "\"";
        for (char const ch : name)
        {
            if (ch == '\\' || ch == '"')
            {
                output.push_back('\\');
            }
            output.push_back(ch);
        }
        output += "\"";
        return output;
    }
} // namespace quxlang

#endif // QUXLANG_BACKENDS_ASM_SYMBOL_FORMAT_HEADER_GUARD
