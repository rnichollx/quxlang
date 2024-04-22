//
// Created by Ryan Nicholl on 4/21/24.
//

#ifndef RPNX_QUXLANG_TEMPLOID_INSTANCIATION_HEADER
#define RPNX_QUXLANG_TEMPLOID_INSTANCIATION_HEADER

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>
#include <optional>

namespace quxlang
{
    /// This resolver cannonicalizes the instanciation of a templexoid.
    // For functums, this is the same as callee instanciation, for templates,
    // this is a template instanciation.
    // e.g. `::foo@(TEMP &I32, CONST & I32)` -> `::foo@[I32, I32](I32, I32)`
    QUX_CO_RESOLVER(templexoid_instanciation, instanciation_reference, std::optional<instanciation_reference>);
} // namespace quxlang

#endif // RPNX_QUXLANG_TEMPLOID_INSTANCIATION_HEADER
