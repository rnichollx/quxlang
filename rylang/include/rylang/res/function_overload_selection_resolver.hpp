//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class function_overload_selection_resolver : public rpnx::co_resolver_base< compiler, call_parameter_information, std::pair< qualified_symbol_reference, call_parameter_information > >
    {

      public:
        using key_type = std::pair< qualified_symbol_reference, call_parameter_information >;

        function_overload_selection_resolver(key_type input)
            : co_resolver_base(input)
        {
        }

        virtual rpnx::resolver_coroutine<compiler, call_parameter_information > co_process(compiler* c, key_type input) override;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER
