//
// Created by Ryan Nicholl on 8/16/23.
//

#ifndef RPNX_RYANSCRIPT1031_MODULE_LOOKUP_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_MODULE_LOOKUP_RESOLVER_HEADER

#include "rylang/compiler.hpp"
#include "rylang/data/lookup_singular.hpp"
#include <string>

namespace rylang
{
    /** Looks up a global symbol from a named module. */
    class module_lookup_resolver : public rpnx::output_base< compiler, symbol_id >
    {
        lookup_singular m_lookup;

      public:
        module_lookup_resolver(std::string const& module_name, lookup_singular lookup)
        : m_lookup(lookup)
        {
        }

        void process(compiler* c);
    };
}

#endif // RPNX_RYANSCRIPT1031_MODULE_LOOKUP_RESOLVER_HEADER
