//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef QUXLANG_MODULE_AST_RESOLVER_HEADER_GUARD
#define QUXLANG_MODULE_AST_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/module_ast.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/symbol_id.hpp"

#include <quxlang/ast2/ast2_module.hpp>

namespace quxlang
{
   class module_ast_resolver
   : public rpnx::resolver_base< compiler, ast2_module_declaration >
   {
     public:
        using key_type = std::string;
       using input_type = key_type;
        inline module_ast_resolver(std::string module_name)
        : m_id(module_name)
        {
        }

        virtual void process(compiler* c);
      private:
        std::string m_id;

   };
}

#endif // QUXLANG_MODULE_AST_RESOLVER_HEADER_GUARD
