// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/functum_initialize_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>
#include <quxlang/keywords.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_initialize_spec > quxlang::functum_initialize_impl(initialization_reference input)
{
    if (typeis< submember >(input.initializee) && keywords::is_constructor_name(as< submember >(input.initializee).name) && input.parameters.named.contains("ARG"))
    {
        for (std::string const name : {"EXPLICIT", "OTHER"})
        {
            if (input.parameters.named.contains(name))
            {
                continue;
            }
            initialization_reference candidate = input;
            auto argument = candidate.parameters.named.extract("ARG");
            argument.key() = name;
            candidate.parameters.named.insert(std::move(argument));
            auto result = co_await rpnx::querygraph::request< functum_initialize_query >(candidate);
            if (result)
            {
                co_return result;
            }
        }
        co_return std::nullopt;
    }

    auto input_functum_str = quxlang::to_string(input.initializee);

    auto selection = co_await rpnx::querygraph::request< functum_select_function_query >(input);

    if (!selection)
    {
        QUX_WHY("No function found that matches the given parameters.");

        co_return std::nullopt;
        // throw quxlang::semantic_compilation_error("No function found that matches the given parameters.");
    }

    co_return co_await rpnx::querygraph::request< function_instanciation_query >(initialization_reference{
                                                               .initializee = selection.value(),
                                                               .parameters = input.parameters,
                                                               .adaptations = input.adaptations,
                                                           });
}
