// Copyright (c) 2023, 2024 Ryan P. Nicholl <rnicholl@protonmail.com>
// All rights reserved.
// A license


#include <quxlang/res/temploid_instanciation_ast_resolver.hpp>

auto quxlang::temploid_instanciation_ast_resolver::co_process(compiler* c, input_type input) -> co_type
{
    auto args = input.parameters;

    auto templ = input.callee;

    assert(!is_contextual(templ));

    ast2_node maybe_templ_ast = co_await *c->lk_entity_ast_from_canonical_chain(templ);

    //std::cout << debug_recursive() << std::endl;

    std::string type = to_string(templ);

    if (typeis< ast2_templex >(maybe_templ_ast))
    {
        co_return co_await *c->lk_template_instanciation_ast(input);
    }
    else if (typeis< ast2_functum >(maybe_templ_ast))
    {
        co_return co_await *c->lk_functum_instanciation_ast(input);
    }

    throw std::runtime_error("Cannot instanciate non-temploid entity.");

}