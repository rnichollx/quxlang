// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_INPUT_SOURCE_HEADER_GUARD
#define QUXLANG_DATA_INPUT_SOURCE_HEADER_GUARD

#include <string>
#include <cinttypes>

namespace quxlang
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