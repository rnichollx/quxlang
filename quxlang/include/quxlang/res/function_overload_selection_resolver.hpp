//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"

namespace quxlang
{
    class function_overload_selection_resolver : public rpnx::co_resolver_base< compiler, call_parameter_information, std::pair< type_symbol, call_parameter_information > >
    {

      public:
        using key_type = std::pair< type_symbol, call_parameter_information >;

        function_overload_selection_resolver(key_type input)
            : co_resolver_base(input)
        {
        }

        virtual std::string question() const override
        {
            return "Selecting overload for " + to_string(input_val.first) + " with parameters " + to_string(input_val.second);
        }

        virtual rpnx::resolver_coroutine<compiler, call_parameter_information > co_process(compiler* c, key_type input) override;
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER_GUARD
