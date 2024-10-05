// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_ASM_PARSE_REGISTER_HEADER_GUARD
#define QUXLANG_PARSERS_ASM_PARSE_REGISTER_HEADER_GUARD
#include "rpnx/value.hpp"
#include "quxlang/data/machine.hpp"

#include <string>

namespace quxlang::parsers
{

    template <typename It>
    std::string parse_register(It& pos, It end, cpu c = cpu::arm_32)
    {
        // TODO: Validate register names here instead of in the backend

        std::string output;
        char ch;

        while (pos != end && ((*pos >= 'A' && *pos <= 'Z') || (*pos >= '0' && *pos <= '9') || *pos == '_'))
        {
            ch = *pos++;

            if (ch >= 'A' && ch <= 'Z')
            {
                output.push_back(ch - 'A' + 'a');
            }
            else
            {
                output.push_back(ch);
            }
        }

        if (output.empty())
        {
            throw std::logic_error("Expected register name");
        }

        return output;
    }


}

#endif //PARSE_REGISTER_HPP