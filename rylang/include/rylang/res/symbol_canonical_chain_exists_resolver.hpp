//
// Created by Ryan Nicholl on 11/12/23.
//

#ifndef RPNX_RYANSCRIPT1031_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
   class symbol_canonical_chain_exists_resolver : public rpnx::resolver_base< compiler, bool >
   {
     public:
         using key_type = qualified_symbol_reference;

         explicit symbol_canonical_chain_exists_resolver(qualified_symbol_reference chain)
         {
             m_chain = chain;
         }

         virtual void process(compiler* c);
       private:
         qualified_symbol_reference m_chain;
   };
}

#endif // RPNX_RYANSCRIPT1031_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER
