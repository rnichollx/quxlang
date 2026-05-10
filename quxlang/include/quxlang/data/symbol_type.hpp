// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_SYMBOL_TYPE_HEADER_GUARD
#define QUXLANG_DATA_SYMBOL_TYPE_HEADER_GUARD

#include <cstdint>

#include <rpnx/macros.hpp>

// clang-format off
RPNX_ENUM(quxlang, symbol_kind, std::int64_t,
    noexist,
    module,
    class_,
    pseudotype,
    functum, function, funtanoid,
    global_variable,
    local_variable,
    member_variable,
    templex, template_,
    namespace_, argument,
    static_test,
    option,
    interface_,
    implementation_,
    enum_,
    flagset_,
    enum_value,
    flagset_value
)
// clang-format on

#endif // QUXLANG_SYMBOL_TYPE_HEADER_GUARD
