//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RYLANG_MODULE_AST_RESOLVER_HEADER_GUARD
#define RYLANG_MODULE_AST_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/module_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/symbol_id.hpp"

namespace rylang
{
   class module_ast_resolver
   : public rpnx::resolver_base< compiler, module_ast >
   {
     public:
        using key_type = std::string;
        inline module_ast_resolver(std::string module_name)
        : m_id(module_name)
        {
        }

        virtual void process(compiler* c);
      private:
        std::string m_id;

   };
}

#endif // RYLANG_MODULE_AST_RESOLVER_HEADER_GUARD
