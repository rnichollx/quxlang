//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD
#define FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class list_user_functum_overloads_resolver : public rpnx::co_resolver_base< compiler, std::set< call_parameter_information >, type_symbol >
    {
      public:
        list_user_functum_overloads_resolver(type_symbol functum)
            : co_resolver_base(functum)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, std::set< call_parameter_information > > co_process(compiler* c, type_symbol input) override;
    };
} // namespace rylang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
