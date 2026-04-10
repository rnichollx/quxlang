// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_initialize_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_initialize_spec > quxlang::functum_initialize_impl(initialization_reference input)
{
    auto input_functum_str = quxlang::to_string(input.initializee);

    auto selection = co_await rpnx::querygraph::request< functum_select_function_query >(input);

    if (!selection)
    {
        QUX_WHY("No function found that matches the given parameters.");

        co_return std::nullopt;
        // throw std::logic_error("No function found that matches the given parameters.");
    }

    co_return co_await rpnx::querygraph::request< function_instanciation_query >(initialization_reference{
                                                               .initializee = selection.value(),
                                                               .parameters = input.parameters,
                                                               .adaptations = input.adaptations,
                                                           });
}
