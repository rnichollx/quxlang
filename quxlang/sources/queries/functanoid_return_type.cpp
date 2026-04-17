// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::functanoid_return_type_spec > quxlang::functanoid_return_type_impl(instanciation_reference input)
{
    std::string input_str = quxlang::to_string(input);
    temploid_reference selected_function = input.temploid;

    auto primitive = co_await rpnx::querygraph::request< function_primitive_query >(selected_function);

    if (primitive)
    {
        auto ret_type = primitive.value().return_type;
        if (is_contextual(ret_type))
        {
            // this can happen if the return type is based on e.g. the paramters
            auto lookup_input = contextual_type_reference{.type = ret_type, .context = input};
            auto lookup_result = co_await rpnx::querygraph::request< lookup_query >(lookup_input);
            if (lookup_result)
            {
                co_return lookup_result.value();
            }
            else
            {
                throw quxlang::compiler_bug("Shouldn't be possible");
            }
        }
        co_return ret_type;

    }

    auto decl = co_await rpnx::querygraph::request< function_declaration_query >(selected_function);

    if (!decl.has_value())
    {
        throw std::logic_error("No function declaration");
    }

    contextual_type_reference decl_ctx = {.context = input, .type = decl.value().definition.return_type.value_or(void_type{})};

    auto decl_type = co_await rpnx::querygraph::request< lookup_query >(decl_ctx);

    co_return decl_type.value();
}
