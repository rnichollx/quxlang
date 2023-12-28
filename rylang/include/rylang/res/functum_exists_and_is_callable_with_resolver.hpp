//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef RYLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
#define RYLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"

namespace rylang
{
    class functum_exists_and_is_callable_with_resolver : public rpnx::co_resolver_base< compiler, bool, std::pair< type_symbol, call_parameter_information > >
    {
      public:

        functum_exists_and_is_callable_with_resolver(input_type input)
            : co_resolver_base(input)
        {
        }
         virtual std::string question() const override;

        virtual rpnx::resolver_coroutine<compiler, bool> co_process(compiler* c, input_type) override;
    };
} // namespace rylang

#endif // RYLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
