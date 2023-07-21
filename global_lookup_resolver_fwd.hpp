//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_GLOBAL_LOOKUP_RESOLVER_FWD_HEADER
#define RPNX_RYANSCRIPT1031_GLOBAL_LOOKUP_RESOLVER_FWD_HEADER

#include "lookup_sequence.hpp"
#include <string>

namespace rs1031
{
    class compiler;

    class global_lookup_resolver
    {
      public:
        using key_type = static_lookup_sequence;
        using value_type = std::size_t;

      private:
        key_type m_key;

      public:
        global_lookup_resolver(key_type key)
            : m_key(key)
        {
        }

        template < typename Self >
        void process(compiler* c, Self* self);
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_GLOBAL_LOOKUP_RESOLVER_FWD_HEADER
