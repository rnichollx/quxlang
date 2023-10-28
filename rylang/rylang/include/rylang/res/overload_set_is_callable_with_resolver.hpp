//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_overload_set.hpp"
#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    class overload_set_is_callable_with_resolver : public rpnx::resolver_base< compiler, bool >
    {
        call_overload_set os;
        call_overload_set args;

      public:
        using key_type = std::pair< call_overload_set, call_overload_set >;

        overload_set_is_callable_with_resolver(key_type input)
            : os(input.first)
            , args(input.second)
        {
        }

        void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER
