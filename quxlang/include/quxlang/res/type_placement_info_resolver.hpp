// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TYPE_PLACEMENT_INFO_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_TYPE_PLACEMENT_INFO_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/symbol_id.hpp"
#include "quxlang/data/type_placement_info.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(type_placement_info, type_symbol, type_placement_info);

} // namespace quxlang

#endif // QUXLANG_type_placement_info_RESOLVER_HEADER_GUARD
