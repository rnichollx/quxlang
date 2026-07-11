// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_STRUCT_FIELD_DECLARATION_HEADER_GUARD
#define QUXLANG_DATA_STRUCT_FIELD_DECLARATION_HEADER_GUARD

#include "contextual_type_reference.hpp"
#include <quxlang/data/basic_types.hpp>
namespace quxlang
{
    /** Describes a struct field before its declared type is resolved in context. */
    struct struct_field_declaration
    {
        std::string name;
        type_symbol type;

        RPNX_MEMBER_METADATA(struct_field_declaration, name, type);
    };

    /** Describes a struct field with its resolved type. */
    struct struct_field
    {
        std::string name;
        type_symbol type;

        RPNX_MEMBER_METADATA(struct_field, name, type);
    };

} // namespace quxlang

#endif // QUXLANG_DATA_STRUCT_FIELD_DECLARATION_HEADER_GUARD
