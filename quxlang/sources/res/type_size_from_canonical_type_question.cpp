//
// Created by Ryan Nicholl on 11/25/23.
//

#include "quxlang/res/type_size_from_canonical_type_question.hpp"

namespace quxlang
{
    template < typename Graph >
    auto type_size_from_canonical_type_question_f(Graph* g, type_symbol type) -> rpnx::resolver_coroutine< Graph, std::size_t >
    {
        assert(!qualified_is_contextual(type));
        auto placement = co_await *g->lk_type_placement_info_from_canonical_type(type);
        co_return placement.size;
    }

    template auto type_size_from_canonical_type_question_f< compiler >(compiler* g, type_symbol type) -> rpnx::resolver_coroutine< compiler, std::size_t >;

} // namespace quxlang