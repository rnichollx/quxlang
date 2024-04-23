//
// Created by Ryan Nicholl on 4/6/24.
//

#ifndef RPNX_QUXLANG_FUNCTUM_INSTANCIATION_HEADER
#define RPNX_QUXLANG_FUNCTUM_INSTANCIATION_HEADER

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/macros.hpp>

namespace quxlang
{
     QUX_CO_RESOLVER(functum_instanciation, instanciation_reference, std::optional<instanciation_reference>);
} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTUM_INSTANCIATION_HEADER
