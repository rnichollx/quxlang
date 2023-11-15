//
// Created by Ryan Nicholl on 11/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/class_layout.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/symbol_id.hpp"

#include <optional>
#include <tuple>

namespace rylang
{

    class operator_is_overloaded_with_resolver : public rpnx::resolver_base< compiler, std::optional< qualified_symbol_reference > >
    {
        std::string m_op;
        qualified_symbol_reference m_lhs;
        qualified_symbol_reference m_rhs;

      public:
        using key_type = std::tuple< std::string, qualified_symbol_reference, qualified_symbol_reference >;

        operator_is_overloaded_with_resolver(key_type input)
        {
            m_op = std::get< 0 >(input);
            m_lhs = std::get< 1 >(input);
            m_rhs = std::get< 2 >(input);
        }

        virtual void process(compiler* c);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_OPERATOR_IS_OVERLOADED_WITH_RESOLVER_HEADER
