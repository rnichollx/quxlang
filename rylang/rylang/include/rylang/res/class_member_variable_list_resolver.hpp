//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER

#include "rylang/data/symbol_id.hpp"
#include "rylang/data/class_member_variable_list.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    /** This resolver takes a class and returns a list of fields of that class */
    class class_member_variable_list_resolver : public rpnx::output_base< compiler, class_member_variable_list >
    {
        class_member_variable_list_resolver(symbol_id id);

        virtual void process(compiler* c);

      private:
        symbol_id m_id;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_RESOLVER_HEADER
