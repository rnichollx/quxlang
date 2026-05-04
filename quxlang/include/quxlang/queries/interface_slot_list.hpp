// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INTERFACE_SLOT_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_INTERFACE_SLOT_LIST_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/basic_types.hpp>

#include <optional>
#include <vector>

namespace quxlang
{
    struct interface_slot
    {
        interface_slot_key key;
        ast2_interface_function_declaration declaration;
        std::optional< type_symbol > default_function;

        RPNX_MEMBER_METADATA(interface_slot, key, declaration, default_function);
    };

    struct interface_slot_list_query
    {
        static constexpr auto query_id = "interface_slot_list";
        using input_type = type_symbol;
        using output_type = std::vector< interface_slot >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INTERFACE_SLOT_LIST_HEADER_GUARD
