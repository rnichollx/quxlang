//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class list_functum_overloads_resolver : public rpnx::resolver_base< compiler, std::optional< std::set< call_parameter_information > > >
    {
      public:
        using key_type = qualified_symbol_reference;
        list_functum_overloads_resolver(qualified_symbol_reference functum)
        {
            m_functum = functum;
        }

        virtual void process(compiler* c) override;

      private:
        qualified_symbol_reference m_functum;
    };
} // namespace rylang


#endif // RPNX_RYANSCRIPT1031_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER
