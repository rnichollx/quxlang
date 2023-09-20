//
// Created by Ryan Nicholl on 9/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_RESOLVER_HEADER

#include "rylang/ast/module_ast.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/symbol_id.hpp"
namespace rylang
{
    class module_ast_precursor1_resolver : public rpnx::output_base< compiler, module_ast >
    {
      public:
        using key_type = std::string;
        inline module_ast_precursor1_resolver(std::string module_name)
        : m_id(module_name)
        {
        }

        virtual void process(compiler* c);

      private:
        std::string m_id;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MODULE_AST_PRECURSOR1_RESOLVER_HEADER
