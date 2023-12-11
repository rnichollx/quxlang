//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    struct functum_exists_and_is_callable_with_resolver : public rpnx::co_resolver_base< compiler, bool, std::pair< qualified_symbol_reference, call_parameter_information > >
    {
      public:

        functum_exists_and_is_callable_with_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual rpnx::resolver_coroutine<compiler, bool> co_process(compiler* c, input_type) override;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER
