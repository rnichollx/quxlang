

#include "rylang/res/vm_procedure_from_canonical_functanoid_question.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    template < typename Graph >
    auto vm_procedure_from_canonical_functanoid_question_f(Graph* g, rylang::qualified_symbol_reference funcname) -> rpnx::resolver_coroutine< Graph, vm_procedure >
    {
        // function_ast function_ast_v = co_await *g->lk_function_ast_from_canonical_functanoid(funcname);

        vm_procedure output;

        vm_generation_frame_info frame;

        co_return output;
    }

    template auto vm_procedure_from_canonical_functanoid_question_f< compiler >(compiler* g, rylang::qualified_symbol_reference type) -> rpnx::resolver_coroutine< compiler, vm_procedure >;

}; // namespace rylang