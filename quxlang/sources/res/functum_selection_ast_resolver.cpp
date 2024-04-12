//
// Created by Ryan Nicholl on 11/5/23.
//
#include "../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/manipulators/argmanip.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/variant_utils.hpp"

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_selection_ast)
{
    std::optional< call_parameter_information > overload_set;

    auto func_addr = input;

    assert(!qualified_is_contextual(func_addr));

    std::string typestr = to_string(func_addr);

    auto entity_ast_v = co_await *c->lk_entity_ast_from_canonical_chain(func_addr.callee);

    if (!typeis< functum >(entity_ast_v))
    {
        throw std::runtime_error("Getting function AST for non-functum entity");
    }

    functum const& functum_entity_ast_v = as< functum >(entity_ast_v);

    auto match_it = functum_entity_ast_v.functions.find(func_addr.header);

    if (match_it != functum_entity_ast_v.functions.end())
    {
        QUX_CO_ANSWER(match_it->second);
    }

    throw std::runtime_error("no matching function overload");
}
