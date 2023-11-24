//
// Created by Ryan Nicholl on 11/23/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_RETURN_TYPE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_RETURN_TYPE_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
   class functanoid_return_type_resolver : public rpnx::resolver_base< compiler, qualified_symbol_reference >
   {
     public:
       using key_type = functanoid_reference;

       functanoid_return_type_resolver(functanoid_reference input)
           : m_function_name(input)
       {
       }

       void process(compiler* c);

     private:
       functanoid_reference m_function_name;
   };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_RETURN_TYPE_RESOLVER_HEADER
