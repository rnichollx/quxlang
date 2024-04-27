//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
#define FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/builtin_functions.hpp"

namespace quxlang
{
    class list_builtin_functum_overloads_resolver : public rpnx::co_resolver_base< compiler,  std::set< primitive_function_info > ,type_symbol >
    {
    public:
        list_builtin_functum_overloads_resolver(type_symbol functum)
            : co_resolver_base(functum)
        {
        }

        virtual auto co_process(compiler* c, type_symbol input) -> co_type override;
    };
} // namespace quxlang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
