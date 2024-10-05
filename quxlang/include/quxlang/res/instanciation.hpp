//
// Created by Ryan Nicholl on 4/21/24.
//

#ifndef QUXLANG_RES_INSTANCIATION_HEADER_GUARD
#define QUXLANG_RES_INSTANCIATION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>
#include <optional>

namespace quxlang
{
    /// This resolver cannonicalizes the instanciation of a templexoid.
    // For functums, this is the same as callee instanciation, for templates,
    // this is a template instanciation.
    // e.g. `::foo#(TEMP &I32, CONST & I32)` -> `::foo#[I32, I32](I32, I32)`
    QUX_CO_RESOLVER(instanciation, instanciation_reference, std::optional<instanciation_reference>);
} // namespace quxlang

#endif // RPNX_QUXLANG_INSTANCIATION_HEADER
