//
// Created by Ryan Nicholl on 11/23/23.
//

#ifndef QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"

namespace quxlang
{
   class functanoid_return_type_resolver : public rpnx::resolver_base< compiler, type_symbol >
   {
     public:
       using key_type = instanciation_reference;

       functanoid_return_type_resolver(instanciation_reference input)
           : m_function_name(input)
       {
       }

       void process(compiler* c);

     private:
       instanciation_reference m_function_name;
   };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD
