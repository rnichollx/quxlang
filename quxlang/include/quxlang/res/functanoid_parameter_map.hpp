//
// Created by Ryan Nicholl on 12/19/23.
//

#ifndef QUXLANG_RES_FUNCTANOID_PARAMETER_MAP_HEADER_GUARD
#define QUXLANG_RES_FUNCTANOID_PARAMETER_MAP_HEADER_GUARD
#include <rpnx/resolver_utilities.hpp>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    class compiler;

    QUX_CO_RESOLVER(functanoid_parameter_map, instanciation_reference, temploid_instanciation_parameter_set);
}

#endif //FUNCTUM_INSTANCIATION_PARAMETER_MAP_RESOLVER_HPP
