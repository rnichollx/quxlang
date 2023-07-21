//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_SIZE_INDEX_RESOLVER_FWD_HEADER
#define RPNX_RYANSCRIPT1031_SIZE_INDEX_RESOLVER_FWD_HEADER
#include "indexer.hpp"
#include "lir_type_index.hpp"
#include <tuple>

namespace rs1031
{
    class compiler;

    class sst_size_index_resolver
    {
      public:
        using key_type = std::tuple< lir_type_id >;

        using value_type = std::size_t;

      private:
        key_type m_key;

      public:
        sst_size_index_resolver(key_type key)
            : m_key(key)
        {
        }

        template < typename Self >
        void process(compiler* c, Self* self);
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_SST_SIZE_INDEX_RESOLVER_HEADER
