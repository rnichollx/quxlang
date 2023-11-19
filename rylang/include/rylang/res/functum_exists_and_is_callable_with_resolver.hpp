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
    struct functum_exists_and_is_callable_with_resolver : public rpnx::resolver_base< compiler, bool >
    {
        qualified_symbol_reference func;
        call_parameter_information args;
      public:
        using key_type = std::pair< qualified_symbol_reference, call_parameter_information >;

        functum_exists_and_is_callable_with_resolver(key_type input)
        {
            func = input.first;
            args = input.second;
        }

        virtual void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER
