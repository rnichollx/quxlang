//
// Created by Ryan Nicholl on 12/3/23.
//

#ifndef CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HPP
#define CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HPP

#include <rpnx/resolver_utilities.hpp>
#include <rylang/ast/function_ast.hpp>
#include <rylang/compiler_fwd.hpp>

namespace rylang
{
    class call_params_of_function_ast_resolver : public rpnx::co_resolver_base< compiler, call_parameter_information, std::pair< function_ast, qualified_symbol_reference > >
    {
      public:
        using input_type = std::pair< function_ast, qualified_symbol_reference >;
        call_params_of_function_ast_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, call_parameter_information > co_process(compiler* c, input_type input) override;
    };

} // namespace rylang

#endif // CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HPP
