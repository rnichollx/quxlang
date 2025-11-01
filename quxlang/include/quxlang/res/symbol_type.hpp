// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_SYMBOL_TYPE_HEADER_GUARD
#define QUXLANG_RES_SYMBOL_TYPE_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>
#include <rpnx/metadata.hpp>


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
    namespace_, argument
)
// clang-format on

namespace quxlang
{
    QUX_CO_RESOLVER(symbol_type, type_symbol, symbol_kind);

}

#endif // RPNX_QUXLANG_SYMBOL_TYPE_HEADER
