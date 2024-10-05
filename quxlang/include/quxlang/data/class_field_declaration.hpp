// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_FIELD_DECLARATION_HEADER_GUARD
#define QUXLANG_DATA_CLASS_FIELD_DECLARATION_HEADER_GUARD

#include "contextual_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
namespace quxlang
{
    struct class_field_declaration
    {
        std::string name;
        type_symbol type;

        RPNX_MEMBER_METADATA(class_field_declaration, name, type);
    };

} // namespace quxlang

#endif // QUXLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD
