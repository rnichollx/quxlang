//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_FWD_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_FWD_HEADER

#include "lir.hpp"

namespace rs1031
{
    class compiler;
    class type_to_typed_field_declaration_list_resolver
    {
      public:
        using key_type = lir_type_id;

        using value_type = lir_typed_field_declaration_list;

      private:
        key_type m_key;

      public:
        type_to_typed_field_declaration_list_resolver(key_type key)
            : m_key(key)
        {
        }

        template < typename Self >
        void process(compiler* c, Self* self);
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_TYPE_TO_TYPED_FIELD_DECLARATION_LIST_RESOLVER_FWD_HEADER
