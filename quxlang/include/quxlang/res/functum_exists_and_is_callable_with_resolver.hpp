//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "rpnx/resolver_utilities.hpp"

namespace quxlang
{
    struct functum_exists_and_is_callable_with_q
    {
        type_symbol functum;
        call_type call;

        RPNX_MEMBER_METADATA(functum_exists_and_is_callable_with_q, functum, call);
    };
    QUX_CO_RESOLVER(functum_exists_and_is_callable_with, functum_exists_and_is_callable_with_q, bool);
} // namespace quxlang

#endif // QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
