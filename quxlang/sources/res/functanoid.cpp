// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/functanoid.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/compiler_binding.hpp"
#include "quxlang/exception.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_parameter_map)
{
    initialization_reference func_ast = input;

    type_symbol selection = input.initializee;

    assert(typeis< temploid_reference >(selection));

    if (!typeis< temploid_reference >(selection))
    {
        auto selection_opt = co_await *c->lk_functum_select_function(input);
        if (!selection_opt.has_value())
        {
            throw std::logic_error("No selection reference for functum.");
        }

        selection = selection_opt.value();
    }

    type_symbol func_name = input.initializee;
    auto functum_instanciation_parameters = input.parameters;

    output_type result;

    temploid_reference selection_sl = as< temploid_reference >(selection);

    // TODO: support named parameters?
    for (std::size_t i = 0; i < functum_instanciation_parameters.positional.size(); i++)
    {
        auto template_arg_contextual = selection_sl.which.interface.positional.at(i);
        // TODO: should the selection reference be decontextualized early?

        auto template_arg = co_await *c->lk_canonical_symbol_from_contextual_symbol(template_arg_contextual, func_name);
        std::string template_arg_str = to_string(template_arg);

        type_symbol instanciation_arg = functum_instanciation_parameters.positional.at(i);
        assert(!is_contextual(instanciation_arg));
        std::string instanciation_arg_str = to_string(instanciation_arg);

        auto match_results = match_template(template_arg, instanciation_arg);
        assert(match_results.has_value());

        for (auto x : match_results.value().matches)
        {
            if (result.parameter_map.find(x.first) != result.parameter_map.end())
            {
                throw std::logic_error("Duplicate template parameter " + x.first + " redeclared in the same template instanciation.");
            }
            result.parameter_map[x.first] = x.second;
        }
    }

    co_return result;
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_return_type)
{
    std::string input_str = quxlang::to_string(input);
    auto selected_function = co_await QUX_CO_DEP(functum_select_function, (input_val));

    if (!selected_function)
    {
        throw std::logic_error("No function selected");
    }
    auto is_builtin = co_await QUX_CO_DEP(function_builtin, (selected_function.value()));

    if (is_builtin)
    {
        co_return is_builtin.value().return_type;
    }


    auto decl = co_await QUX_CO_DEP(function_declaration, (selected_function.value()));

    if (!decl.has_value())
    {
        throw std::logic_error("No function declaration");
    }

    contextual_type_reference decl_ctx = {.context = input_val, .type = decl.value().definition.return_type.value_or(void_type{})};

    auto decl_type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (decl_ctx));

    co_return decl_type;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_sigtype)
{
    sigtype result;

    result.params = input.parameters;

    result.return_type = co_await QUX_CO_DEP(functanoid_return_type, (input));

    co_return result;

}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(functanoid_param_names)
{
    // Transitional: use compiler_binder.
    compiler_binder prv(c);


    QUXLANG_DEBUG_VALUE(quxlang::to_string(input));

    param_names result;

    result.positional.resize(input.parameters.positional.size());


    // Builtin functions don't have any AST to work with, so we can't get the names of the parameters.
    // However, we have to return *something* because the code for argument generation is shared
    // between builtin and non-builtin functions.
    auto is_builtin = as<temploid_reference> (input.initializee).which.builtin;

    if (is_builtin)
    {
        co_return result;
    }

    std::optional< ast2_function_declaration > decl_opt = co_await prv.function_declaration(as< temploid_reference >(input.initializee));

    QUXLANG_COMPILER_BUG_IF(!decl_opt.has_value(), "Function declaration not found");

    ast2_function_declaration const& decl = decl_opt.value();

    std::size_t positional_index = 0;
    for (auto const& param : decl.header.call_parameters)
    {
        if (param.api_name.has_value())
        {
            result.named[param.api_name.value()] = param.name;
        }
        else
        {
            result.positional[positional_index] = param.name;
            positional_index++;
        }
    }

    co_return result;

}
