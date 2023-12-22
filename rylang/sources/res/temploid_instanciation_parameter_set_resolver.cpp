//
// Created by Ryan Nicholl on 12/21/23.
//
#include <rylang/res/temploid_instanciation_parameter_set_resolver.hpp>

auto rylang::temploid_instanciation_parameter_set_resolver::co_process(compiler* c, input_type input) -> co_type
{

    auto ast = co_await *c->lk_entity_ast_from_canonical_chain(input.callee);

    if (typeis< ast2_templex >(ast))
    {
        co_return co_await *c->lk_template_instanciation_parameter_set(input);
    }
    else if (typeis< ast2_functum >(ast))
    {
        co_return co_await *c->lk_functum_instanciation_parameter_map(input);
    }

    assert(false);

    throw std::logic_error("wut");
}