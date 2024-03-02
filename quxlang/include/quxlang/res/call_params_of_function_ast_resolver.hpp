//
// Created by Ryan Nicholl on 12/3/23.
//

#ifndef CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HEADER_GUARD
#define CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HEADER_GUARD

#include <rpnx/resolver_utilities.hpp>
#include <quxlang/ast/function_ast.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/compiler_fwd.hpp>

namespace quxlang
{
    class call_params_of_function_ast_resolver : public rpnx::co_resolver_base< compiler, call_parameter_information, std::pair< ast2_function_declaration, type_symbol > >
    {
      public:
        explicit call_params_of_function_ast_resolver(input_type input)
            : co_resolver_base(std::move(input))
        {
        }

        virtual rpnx::resolver_coroutine< compiler, output_type > co_process(compiler* c, input_type input) override;
    };

} // namespace quxlang

#endif // CALL_PARAMS_OF_FUNCTION_AST_RESOLVER_HEADER_GUARD
