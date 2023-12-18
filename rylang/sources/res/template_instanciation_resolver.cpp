//
// Created by Ryan Nicholl on 12/17/23.
//

#include <rylang/res/template_instanciation_resolver.hpp>
auto rylang::template_instanciation_resolver::co_process(compiler* c, input_type input) -> co_type
{
    auto args = input.parameters;

    auto templ = input.callee;

    assert(!is_contextual(templ));

    ast2_map_entity maybe_templ_ast = co_await *c->lk_entity_ast_from_canonical_chain(templ);

    std::string type = to_string(templ);
    if (!typeis< ast2_templex >(maybe_templ_ast))
    {
        throw std::runtime_error("Cannot create template instanciation of non-template symbol.");
    }

    ast2_templex templ_ast = as< ast2_templex >(maybe_templ_ast);

    std::size_t eligible_templates = 0;

    ast2_template_declaration const* selected_template = nullptr;
    std::optional< std::int64_t > template_priority;


    call_parameter_information templ_instanciation_args;
    templ_instanciation_args.argument_types = args;
    for (ast2_template_declaration const& decl : templ_ast.templates)
    {
        std::size_t priority = decl.priority.value_or(0);
        if (template_priority.has_value() && *template_priority > priority)
        {
            continue;
        }

        call_parameter_information decl_args;
        decl_args.argument_types = decl.m_template_args;
        bool can_use_this_template = co_await * c->lk_overload_set_is_callable_with(decl_args, templ_instanciation_args);
        if (can_use_this_template)
        {
            if (! template_priority.has_value() || priority > *template_priority)
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
        throw std::runtime_error("Found no matching template");
    }

    if (eligible_templates > 1)
    {
        throw std::runtime_error("Ambiguous template selection");
    }

    co_return selected_template->m_class;
}