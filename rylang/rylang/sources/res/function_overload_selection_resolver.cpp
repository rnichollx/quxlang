//
// Created by Ryan Nicholl on 11/4/23.
//
#include "rylang/res/function_overload_selection_resolver.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler.hpp"

void rylang::function_overload_selection_resolver::process(compiler* c)
{
    std::size_t eligible_overloads = 0;

    // We need to collect the overloads that are callable with the given arguments

    auto entity_ast_dep = get_dependency(
        [&]
        {
            return c->lk_entity_ast_from_canonical_chain(m_function_location);
        });

    if (!ready())
        return;

    entity_ast ent = entity_ast_dep->get();

    if (ent.type() != entity_type::function_type)
    {
        throw std::logic_error("Cannot resolve function overload of non-functum entity");
    }

    functum_entity_ast const& func_ast = ent.get_as< functum_entity_ast >();

    for (auto const& overload : func_ast.m_function_overloads) {

    }
}
