//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef QUXLANG_RES_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "quxlang/data/type_symbol.hpp"
#include <optional>
#include "quxlang/macros.hpp"

namespace quxlang
{
    struct overload_set_instanciate_with_q
    {
        function_overload overload;
        call_type call;

        RPNX_MEMBER_METADATA(overload_set_instanciate_with_q, overload, call);
    };

    QUX_CO_RESOLVER(overload_set_instanciate_with, overload_set_instanciate_with_q, std::optional< call_type >);

} // namespace quxlang

#endif // OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD
