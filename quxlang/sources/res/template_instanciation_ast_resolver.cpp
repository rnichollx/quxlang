// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/template_instanciation_ast_resolver.hpp>

std::string quxlang::template_instanciation_ast_resolver::question() const
{
    return "template_instanciation_ast(" + to_string(input_val) + ")";
}

auto quxlang::template_instanciation_ast_resolver::co_process(compiler* c, input_type input) -> co_type
{
    auto args = input.parameters;

    auto templ = input.initializee;

    assert(!is_contextual(templ));

    ast2_node maybe_templ_ast = co_await QUX_CO_DEP(symboid, (templ));

    // std::cout << debug_recursive() << std::endl;

    std::string type = to_string(templ);

    if (!typeis< ast2_templex >(maybe_templ_ast))
    {
        throw std::logic_error("Cannot create template instanciation of non-template symbol.");
    }

    ast2_templex templ_ast = as< ast2_templex >(maybe_templ_ast);

    std::size_t eligible_templates = 0;

    ast2_template_declaration const* selected_template = nullptr;
    std::optional< std::int64_t > template_priority;

    invotype templ_instanciation_args;
    templ_instanciation_args = args;
    for (ast2_template_declaration const& decl : templ_ast.templates)
    {
        std::size_t priority = decl.priority.value_or(0);
        if (template_priority.has_value() && *template_priority > priority)
        {
            continue;
        }

        invotype decl_args;
        decl_args.positional = decl.m_template_args;
        auto can_use_this_template = co_await *c->lk_overload_set_instanciate_with(overload_set_instanciate_with_q{.overload = temploid_ensig{.interface = templ_instanciation_args}, .call = decl_args});
        if (can_use_this_template)
        {
            if (!template_priority.has_value() || priority > *template_priority)
            {
                eligible_templates = 0;
                template_priority = priority;
            }

            eligible_templates++;
            selected_template = &decl;
        }
    }

    if (selected_template == nullptr)
    {
        throw std::logic_error("Found no matching template");
    }

    if (eligible_templates > 1)
    {
        throw std::logic_error("Ambiguous template selection");
    }

    co_return *selected_template;
}
