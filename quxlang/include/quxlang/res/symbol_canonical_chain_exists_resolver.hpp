//
// Created by Ryan Nicholl on 11/12/23.
//

#ifndef QUXLANG_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
#define QUXLANG_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast/file_ast.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_list.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"

namespace quxlang
{
   class symbol_canonical_chain_exists_resolver : public rpnx::resolver_base< compiler, bool >
   {
     public:
         using key_type = type_symbol;

         explicit symbol_canonical_chain_exists_resolver(type_symbol chain)
         {
             m_chain = chain;
         }

         virtual void process(compiler* c);
       private:
         type_symbol m_chain;
   };
}

#endif // QUXLANG_SYMBOL_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
