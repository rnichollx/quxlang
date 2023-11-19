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
    class function_overload_selection_resolver : public rpnx::resolver_base< compiler, call_parameter_information >
    {
        call_parameter_information m_args;
        qualified_symbol_reference m_function_location;

      public:
        using key_type = std::pair< qualified_symbol_reference, call_parameter_information >;

        function_overload_selection_resolver(key_type input)
            : m_function_location(input.first)
            , m_args(input.second)
        {
        }

        virtual void process(compiler* c);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER
