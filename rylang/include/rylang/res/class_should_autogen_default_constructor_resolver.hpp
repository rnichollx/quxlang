//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class class_should_autogen_default_constructor_resolver : public virtual rpnx::resolver_base< compiler, bool >
    {
      public:
        using key_type = qualified_symbol_reference;
        class_should_autogen_default_constructor_resolver(qualified_symbol_reference cls)
            : m_cls(cls)
        {
        }

        void process(compiler* c);

      private:
        qualified_symbol_reference m_cls;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER
