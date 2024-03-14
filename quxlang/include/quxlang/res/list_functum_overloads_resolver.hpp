//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef QUXLANG_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
#define QUXLANG_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    class list_functum_overloads_resolver : public rpnx::co_resolver_base< compiler, std::optional< std::set< call_parameter_information > >,type_symbol >
    {
      public:
        list_functum_overloads_resolver(type_symbol functum)
            : co_resolver_base(functum)
        {
        }

        virtual std::string question() const override;


        virtual rpnx::resolver_coroutine< compiler, std::optional< std::set< call_parameter_information > > > co_process(compiler* c, type_symbol input) override;
    };
} // namespace quxlang


#endif // QUXLANG_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
