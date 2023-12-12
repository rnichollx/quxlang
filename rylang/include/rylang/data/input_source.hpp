#ifndef RPNX_RYLANG_INPUT_SOURCE_HEADER_GUARD
#define RPNX_RYLANG_INPUT_SOURCE_HEADER_GUARD

#include <string>
#include <cinttypes>

namespace rylang
{
    struct input_source
    {
        std::string name;
        std::string address;
        std::size_t line_begin;
        std::size_t line_end;
        std::size_t column_begin;
        std::size_t column_end;
    };
}

#endif