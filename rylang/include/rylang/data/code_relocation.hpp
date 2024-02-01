//
// Created by Ryan Nicholl on 1/27/24.
//

#ifndef CODE_RELOCATION_HPP
#define CODE_RELOCATION_HPP
#include <string>

namespace rylang
{
    enum class relocation_type
    {
        absolute,
        relative
    };

    struct symbol_relocation
    {
        std::string name;
        std::size_t reloffset;
        relocation_type type;
        std::size_t bits;
    };
}

#endif //CODE_RELOCATION_HPP
