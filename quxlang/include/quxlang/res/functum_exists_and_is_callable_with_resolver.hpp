//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD

#include "quxlang/data/qualified_symbol_reference.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"

namespace quxlang
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
} // namespace quxlang

#endif // QUXLANG_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
