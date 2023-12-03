//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP
#define FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class list_builtin_functum_overloads_resolver : public rpnx::co_resolver_base< compiler,  std::set< call_parameter_information > ,qualified_symbol_reference >
    {
    public:
        list_builtin_functum_overloads_resolver(qualified_symbol_reference functum)
            : co_resolver_base(functum)
        {
        }

        virtual rpnx::resolver_coroutine< compiler,  std::set< call_parameter_information >  > co_process(compiler* c, qualified_symbol_reference input) override;
    };
} // namespace rylang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP
