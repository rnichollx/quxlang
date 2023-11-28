//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class list_functum_overloads_resolver : public rpnx::co_resolver_base< compiler, std::optional< std::set< call_parameter_information > >,qualified_symbol_reference >
    {
      public:
        list_functum_overloads_resolver(qualified_symbol_reference functum)
            : co_resolver_base(functum)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, std::optional< std::set< call_parameter_information > > > co_process(compiler* c, qualified_symbol_reference input) override;
    };
} // namespace rylang


#endif // RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER
