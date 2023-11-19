//
// Created by Ryan Nicholl on 11/4/23.
//
#ifndef RPNX_RYANSCRIPT1031_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class function_qualified_reference_resolver : public rpnx::resolver_base< compiler, qualified_symbol_reference >
    {
        qualified_symbol_reference m_input;

        call_parameter_information m_args;

      public:
        using key_type = std::pair< qualified_symbol_reference, call_parameter_information >;

        function_qualified_reference_resolver(key_type input)
            : m_input(input.first)
            , m_args(input.second)
        {
        }

        virtual void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER