//
// Created by Ryan Nicholl on 12/21/23.
//
#include <quxlang/res/functum_instanciation_parameter_map_resolver.hpp>

auto quxlang::functum_instanciation_parameter_map_resolver::co_process(compiler* c, input_type input) -> co_type
{
    ast2_function_declaration const & func_ast = co_await *c->lk_functum_instanciation_ast(input);

    type_symbol func_name = input.callee;
    auto functum_instanciation_parameters = input.parameters;

    output_type result;

    call_parameter_information func_args = co_await *c->lk_call_params_of_function_ast(func_ast, func_name);

    for (std::size_t i = 0; i < func_args.argument_types.size(); i++)
    {
        auto template_arg_contextual = func_args.argument_types[i];

        auto template_arg = co_await *c->lk_canonical_symbol_from_contextual_symbol(template_arg_contextual, func_name);
        std::string template_arg_str = to_string(template_arg);

        type_symbol instanciation_arg = func_args.argument_types[i];
        assert(!is_contextual(instanciation_arg));
        std::string instanciation_arg_str = to_string(instanciation_arg);

        auto match_results = match_template(template_arg, instanciation_arg);
        assert(match_results.has_value());

        for (auto x : match_results.value().matches)
        {
            if (result.parameter_map.find(x.first) != result.parameter_map.end())
            {
                throw std::runtime_error("Duplicate template parameter " + x.first + " redeclared in the same template instanciation.");
            }
            result.parameter_map[x.first] = x.second;
        }
    }

    co_return result;


}