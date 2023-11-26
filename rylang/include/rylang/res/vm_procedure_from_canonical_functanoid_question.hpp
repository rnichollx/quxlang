//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/vm_procedure.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/function_statement.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/vm_generation_frameinfo.hpp"

namespace rylang
{
    template < typename Graph >
    auto vm_procedure_from_canonical_functanoid_question_f(Graph* g, qualified_symbol_reference type) -> rpnx::resolver_coroutine< Graph, vm_procedure >;

    extern template auto vm_procedure_from_canonical_functanoid_question_f< compiler >(compiler* g, qualified_symbol_reference type) -> rpnx::resolver_coroutine< compiler, vm_procedure >;

    struct vm_procedure_from_canonical_functanoid_question
    {
        template < typename Graph, typename... Ts >
        auto ask(Graph* g, Ts&&... ts) -> rpnx::resolver_coroutine< Graph, std::size_t >
        {
            return vm_procedure_from_canonical_functanoid_question_f< Graph >(g, std::forward< Ts >(ts)...);
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
