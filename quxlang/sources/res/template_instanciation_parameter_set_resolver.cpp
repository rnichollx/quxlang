//
// Created by Ryan Nicholl on 12/18/23.
//

#include <quxlang/res/template_instanciation_parameter_set_resolver.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/compiler.hpp>

namespace quxlang
{
    std::string template_instanciation_parameter_set_resolver::question() const
    {
        return "template_instanciation_parameter_set(" + to_string(input_val) + ")";
    }

    auto template_instanciation_parameter_set_resolver::co_process(compiler* c, input_type input) -> co_type
    {
        // The goal of this function is to extract the named template arguments from the instanciation reference

        // Step 1. obtain the template declaration
        // Step 2. match the template declaration's parameters against the input parameters
        // Step 3. extract the matches as the return values

        type_symbol template_name = input.callee;
        auto template_instanciation_parameters = input.parameters;

        // We have to get the template AST by template instanciation because it's possible that there could be multiple templates
        // attached to the templex.

        ast2_template_declaration const& template_ast = co_await *c->lk_template_instanciation_ast(input);

        QUXLANG_DEBUG({
        std:: cout << "template ast: " << to_string(template_ast) << std::endl;
            });

        // assert(template_ast.m_template_args.size() == input.parameters.size());

        output_type result;

        for (std::size_t i = 0; i < template_ast.m_template_args.size(); i++)
        {
            auto template_arg_contextual = template_ast.m_template_args[i];

            auto template_arg = co_await *c->lk_canonical_symbol_from_contextual_symbol(template_arg_contextual, template_name);
            std::string template_arg_str = to_string(template_arg);

            type_symbol instanciation_arg = template_instanciation_parameters.positional_parameters[i];
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
} // namespace quxlang