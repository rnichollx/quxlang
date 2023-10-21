//
// Created by Ryan Nicholl on 10/20/23.
//
#include "rylang/res/type_size_from_canonical_type_resolver.hpp"

void rylang::type_size_from_canonical_type_resolver::process(compiler* c)
{
   canonical_type_reference const& type = m_type;

   if (type.type() == boost::typeindex::type_id<canonical_pointer_type_reference>())
   {
       auto machine_info_dep = get_dependency(
           [&]
           {
               return c->lk_machine_info();
           });
   }

}
