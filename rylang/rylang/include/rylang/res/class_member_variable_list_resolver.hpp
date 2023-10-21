//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER

#include "rylang/compiler.hpp"
#include "rylang/data/class_member_variable_list.hpp"
#include "rylang/data/symbol_id.hpp"

namespace rylang
{
    /** This resolver takes a class and returns a list of fields of that class */
    class class_member_variable_list_resolver : public rpnx::resolver_base< compiler, class_member_variable_list >
    {
      public:
        class_member_variable_list_resolver(lookup_chain id)
            : m_id(id)
        {
        }

        virtual void process(compiler* c);

      private:
        lookup_chain m_id;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER
