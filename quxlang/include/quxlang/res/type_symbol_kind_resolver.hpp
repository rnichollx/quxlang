//
// Created by Ryan Nicholl on 3/2/24.
//

#ifndef RPNX_QUXLANG_TYPE_SYMBOL_KIND_RESOLVER_HEADER
#define RPNX_QUXLANG_TYPE_SYMBOL_KIND_RESOLVER_HEADER

#include "quxlang/macros.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
namespace quxlang
{
    enum class symbol_kind {
        functum_kind,
        class_kind,
        namespace_kind,
        templex_kind,
        variable_kind,
        asm_procedure_kind,
        nonexistent_kind,
    };
    
    QUX_CO_RESOLVER(type_symbol_kind, type_symbol, symbol_kind);
}

#endif // RPNX_QUXLANG_TYPE_SYMBOL_KIND_RESOLVER_HEADER
