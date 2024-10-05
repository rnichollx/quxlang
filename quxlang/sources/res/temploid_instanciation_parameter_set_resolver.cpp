//
// Created by Ryan Nicholl on 12/21/23.
//
#include <quxlang/res/temploid_instanciation_parameter_set_resolver.hpp>

#include <quxlang/compiler.hpp>

auto quxlang::temploid_instanciation_parameter_set_resolver::co_process(compiler* c, input_type input) -> co_type
{
    QUXLANG_DEBUG({std::cout << "temploid_instanciation_parameter_set_resolver::co_process input_type=" << to_string(input) << std::endl;});


    selection_reference sel = as<selection_reference>(input.callee);
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