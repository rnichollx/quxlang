//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
#define RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD

#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class functum_instanciation_ast_resolver : public rpnx::co_resolver_base< compiler, ast2_function_declaration, type_symbol >
    {
      public:
        using key_type = type_symbol;

        functum_instanciation_ast_resolver(type_symbol input)
            : co_resolver_base(input)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, ast2_function_declaration > co_process(compiler* c, type_symbol function_name) override;

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
            throw std::logic_error("unreachable");
        }

      private:
    };
} // namespace rylang

#endif // RYLANG_FUNCTION_AST_RESOLVER_HEADER_GUARD
