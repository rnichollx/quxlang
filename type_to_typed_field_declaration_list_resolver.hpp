//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_HEADER

#include "compiler.hpp"
#include "type_to_typed_field_declaration_list_resolver_fwd.hpp"

namespace rs1031
{
    template < typename Self >
    void type_to_typed_field_declaration_list_resolver::process(compiler* c, Self* self)
    {
        auto untyped_list = c->get_untyped_field_declaration_list(this->m_key);
        if (!untyped_list.complete())
        {
            self->add_dependency(untyped_list);
            return;
        }

        lir_typed_field_declaration_list result;
    }
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_HEADER
