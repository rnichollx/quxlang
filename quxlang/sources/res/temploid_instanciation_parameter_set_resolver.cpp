// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/res/temploid_instanciation_parameter_set_resolver.hpp>

#include <quxlang/compiler.hpp>

auto quxlang::temploid_instanciation_parameter_set_resolver::co_process(compiler* c, input_type input) -> co_type
{
    QUXLANG_DEBUG({std::cout << "temploid_instanciation_parameter_set_resolver::co_process input_type=" << to_string(input) << std::endl;});


    temploid_reference sel = as<temploid_reference>(input.initializee);
    auto ast = co_await QUX_CO_DEP(symboid, (sel.templexoid));

    auto idx = ast.type_index().name();

    if (typeis< ast2_templex >(ast))
    {
        co_return co_await *c->lk_template_instanciation_parameter_set(input);
    }
    else if (typeis< functum >(ast))
    {
        co_return co_await *c->lk_functanoid_parameter_map(input);
    }

    assert(false);

    throw std::logic_error("wut");
}