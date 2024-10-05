// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_FIELD_INFO_HEADER_GUARD
#define QUXLANG_DATA_CLASS_FIELD_INFO_HEADER_GUARD

#include "canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
#include <vector>

namespace quxlang

{
    struct class_field_info
    {
        std::string name;
        type_symbol type;
        std::size_t offset = 0;

        RPNX_MEMBER_METADATA(class_field_info, name, type, offset);
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_FIELD_INFO_HEADER_GUARD
