//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER

#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class function_ast_resolver : public rpnx::co_resolver_base< compiler, function_ast, qualified_symbol_reference >
    {
      public:
        using key_type = qualified_symbol_reference;

        function_ast_resolver(qualified_symbol_reference input)
            : co_resolver_base(input)
        {
        }

        rpnx::resolver_coroutine< compiler, function_ast > co_process(compiler* c, qualified_symbol_reference function_name);

        virtual std::string question() const override
        {
            return "What is the function AST for " + to_string(input_val) + "?";
        }

        virtual std::string answer() const override
        {
            if (has_value())
            {
                return "OK";
            }

            else
            {
                try
                {
                    get();
                }
                catch (std::exception& e)
                {
                    return e.what();
                }
                catch (...)
                {
                    return "Unknown error";
                }
            }
        }

      private:
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER
