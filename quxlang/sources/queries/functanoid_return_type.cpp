// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::functanoid_return_type_spec > quxlang::functanoid_return_type_impl(instanciation_reference input)
{
    std::string input_str = quxlang::to_string(input);
    temploid_reference selected_function = input.temploid;

    std::optional< builtin_function_info > primitive = co_await rpnx::querygraph::request< function_primitive_query >(selected_function);

    if (primitive)
    {
        type_symbol ret_type = primitive.value().return_type;
        if (is_contextual(ret_type) || is_template(ret_type))
        {
            // this can happen if the return type is based on e.g. the paramters
            contextual_type_reference lookup_input = {.type = ret_type, .context = input};
            std::optional< type_symbol > lookup_result = co_await rpnx::querygraph::request< lookup_query >(lookup_input);
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

    std::optional< ast2_function_declaration > decl = co_await rpnx::querygraph::request< function_declaration_query >(selected_function);

    if (!decl.has_value())
    {
        throw std::logic_error("No function declaration");
    }

    contextual_type_reference decl_ctx = {.context = input, .type = decl.value().definition.return_type.value_or(void_type{})};

    std::optional< type_symbol > decl_type = co_await rpnx::querygraph::request< lookup_query >(decl_ctx);
    if (!decl_type.has_value())
    {
        throw quxlang::compiler_bug("Function return type could not be resolved");
    }

    if (is_template(decl_type.value()))
    {
        co_return co_await rpnx::querygraph::subquery_request< functanoid_deduced_return_type >(input, std::monostate{});
    }

    co_return decl_type.value();
}
