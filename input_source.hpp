#ifndef RPNX_RYANSCRIPT1031_INPUT_SOURCE_HEADER
#define RPNX_RYANSCRIPT1031_INPUT_SOURCE_HEADER

#include <string>
#include <cinttypes>

namespace rs1031
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