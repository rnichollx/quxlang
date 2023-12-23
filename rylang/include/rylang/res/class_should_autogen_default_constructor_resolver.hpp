//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RYLANG_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD
#define RYLANG_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/class_list.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    class class_should_autogen_default_constructor_resolver : public virtual rpnx::resolver_base< compiler, bool >
    {
      public:
        using key_type = type_symbol;
        class_should_autogen_default_constructor_resolver(type_symbol cls)
            : m_cls(cls)
        {
        }

        virtual std::string question() const override
        {
            return "class_should_autogen_default_constructor(" + to_string(m_cls) + ")";
        }

        void process(compiler* c);

      private:
        type_symbol m_cls;
    };
} // namespace rylang

#endif // RYLANG_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD
