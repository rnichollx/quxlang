

#include "rylang/res/vm_procedure_from_canonical_functanoid_question.hpp"
#include "rylang/compiler.hpp"
#include "rylang/res/detail/vm_procedure_builder.hpp"

namespace rylang
{
    template < typename Graph >
    auto vm_procedure_from_canonical_functanoid_question_f(Graph* g, rylang::qualified_symbol_reference funcname) -> rpnx::resolver_coroutine< Graph, vm_procedure >
    {
       return rpnx::unimplemented();
    }

    template auto vm_procedure_from_canonical_functanoid_question_f< compiler >(compiler* g, rylang::qualified_symbol_reference type) -> rpnx::resolver_coroutine< compiler, vm_procedure >;

}; // namespace rylang