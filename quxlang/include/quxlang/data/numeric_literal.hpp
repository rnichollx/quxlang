// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_NUMERIC_LITERAL_HEADER_GUARD
#define QUXLANG_DATA_NUMERIC_LITERAL_HEADER_GUARD

#include <string>
#include <rpnx/metadata.hpp>

namespace quxlang
{
    struct expression_numeric_literal
    {
        std::string value;

        RPNX_MEMBER_METADATA(expression_numeric_literal, value);
    };

    struct expression_string_literal
    {
        std::string value;

        RPNX_MEMBER_METADATA(expression_string_literal, value);
    };

} // namespace quxlang

#endif // QUXLANG_NUMERIC_LITERAL_HEADER_GUARD
