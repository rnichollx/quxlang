//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef QUXLANG_RES_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_list.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
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

        void process(compiler* c) override;

      private:
        type_symbol m_cls;
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_SHOULD_AUTOGEN_DEFAULT_CONSTRUCTOR_RESOLVER_HEADER_GUARD
