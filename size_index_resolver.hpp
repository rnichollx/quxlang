//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_SIZE_INDEX_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_SIZE_INDEX_RESOLVER_HEADER

#include "compiler.hpp"
#include "compiler_fwd.hpp"
#include "size_index_resolver_fwd.hpp"

namespace rs1031
{
    template < typename Self >
    void sst_size_index_resolver::process(rs1031::compiler* c, Self* self)
    {
        auto typelist = c->get_typelist(this->m_key);
        if (!typelist.complete())
        {
            self->add_dependency(typelist);
            return;
        }

        self->set_value(0);
    }
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_SIZE_INDEX_RESOLVER_HEADER
