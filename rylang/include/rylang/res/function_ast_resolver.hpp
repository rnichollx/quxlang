//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER

#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    class function_ast_resolver : public rpnx::resolver_base< compiler, function_ast >
    {
      public:
        using key_type = qualified_symbol_reference;

        function_ast_resolver(qualified_symbol_reference input)
            : m_function_name(input)
        {
        }

        void process(compiler* c);

      private:
        qualified_symbol_reference m_function_name;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_AST_RESOLVER_HEADER
