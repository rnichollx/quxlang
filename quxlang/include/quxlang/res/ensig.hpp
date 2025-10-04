//
// Created by Ryan Nicholl on 6/6/2025.
//

#ifndef QUXLANG_RES_ENSIG_HEADER_GUARD
#define QUXLANG_RES_ENSIG_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include "quxlang/res/resolver.hpp"
#include <string>
#include <set>

namespace quxlang
{

    using tempar_name_set = std::set< std::string >;
    QUX_CO_RESOLVER(symbol_tempars, type_symbol, tempar_name_set);
    QUX_CO_RESOLVER(ensig_tempars, temploid_ensig, tempar_name_set);
} // namespace quxlang
#endif // ENSIG_HPP
