//
// Created by Ryan Nicholl on 11/11/23.
//

#ifndef QUXLANG_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER_GUARD
#define QUXLANG_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
#include "quxlang/data/symbol_id.hpp"

#include <optional>
#include <tuple>

namespace quxlang
{

    class operator_is_overloaded_with_resolver : public rpnx::resolver_base< compiler, std::optional< type_symbol > >
    {
        std::string m_op;
        type_symbol m_lhs;
        type_symbol m_rhs;

      public:
        using key_type = std::tuple< std::string, type_symbol, type_symbol >;

        operator_is_overloaded_with_resolver(key_type input)
        {
            m_op = std::get< 0 >(input);
            m_lhs = std::get< 1 >(input);
            m_rhs = std::get< 2 >(input);
        }

        virtual void process(compiler* c);
    };

} // namespace quxlang

#endif // QUXLANG_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER_GUARD
