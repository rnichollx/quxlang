//
// Created by Ryan Nicholl on 11/4/23.
//
#ifndef QUXLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
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

} // namespace quxlang

#endif // QUXLANG_FUNCTION_QUALIFIED_REFERENCE_RESOLVER_HEADER_GUARD