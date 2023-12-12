//
// Created by Ryan Nicholl on 11/4/23.
//
#ifndef RYLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD
#define RYLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class [[deprecated]] function_qualified_reference_resolver : public rpnx::resolver_base< compiler, type_symbol >
    {
        type_symbol m_input;

        call_parameter_information m_args;

      public:
        using key_type = std::pair< type_symbol, call_parameter_information >;

        function_qualified_reference_resolver(key_type input)
            : m_input(input.first)
            , m_args(input.second)
        {
        }

        virtual void process(compiler* c);
    };

} // namespace rylang

#endif // RYLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD