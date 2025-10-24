// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_INSTANCIATION_HEADER_GUARD
#define QUXLANG_RES_INSTANCIATION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <optional>
#include <quxlang/res/resolver.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

namespace quxlang
{
    /// This resolver cannonicalizes the instanciation of a templexoid.
    // For functums, this is the same as callee instanciation, for templates,
    // this is a template instanciation.
    // e.g. `::foo#(TEMP &I32, CONST & I32)` -> `::foo#[I32, I32](I32, I32)`
    QUX_CO_RESOLVER(instanciation, initialization_reference, std::optional< instanciation_reference >);

    QUX_CO_RESOLVER(instanciation_tempar_map, instanciation_reference, temploid_instanciation_parameter_set);
} // namespace quxlang

#endif // RPNX_QUXLANG_INSTANCIATION_HEADER
