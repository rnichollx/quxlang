// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/antestatal_static_value_spec.hpp>

#include <stdexcept>


rpnx::querygraph::coroutine< quxlang::antestatal_static_value_spec > quxlang::antestatal_static_value_impl(type_symbol input)
{
    if (typeis< subtag_type >(input))
    {
        std::optional< parameter_instantiation > binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        if (!binding.has_value() || !binding->template type_is< parameter_value_instantiation >())
        {
            throw quxlang::semantic_compilation_error("requested antestatal value for a non-antestatal static: " + quxlang::to_string(input));
        }

        constexpr_value const& value = binding->template get_as< parameter_value_instantiation >().value;
        if (typeis< antestatal_value >(value))
        {
            co_return as< antestatal_value >(value);
        }
        if (typeis< constexpr_serialoid >(value))
        {
            co_return antestatal_primitive{.value = as< constexpr_serialoid >(value).bytes};
        }
        if (typeis< constexpr_string >(value))
        {
            co_return antestatal_primitive{.value = as< constexpr_string >(value).bytes};
        }
        if (typeis< constexpr_numeric >(value))
        {
            co_return antestatal_primitive{.value = as< constexpr_numeric >(value).bytes};
        }

        throw quxlang::compiler_bug("unsupported subtag constexpr value");
    }

    if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(input)))
    {
        throw quxlang::semantic_compilation_error("requested antestatal value for a non-antestatal static: " + quxlang::to_string(input));
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        throw quxlang::semantic_compilation_error("antestatal static symbol is not a variable: " + quxlang::to_string(input));
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.init_expr.has_value())
    {
        throw quxlang::semantic_compilation_error("antestatal STATIC requires a := initializer: " + quxlang::to_string(input));
    }
    if (!decl.init_args.empty())
    {
        throw quxlang::semantic_compilation_error("antestatal STATIC initializer argument lists are not implemented: " + quxlang::to_string(input));
    }

    auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);
    if (typeis< readonly_constant >(variable_type))
    {
        readonly_constant const& constant_type = as< readonly_constant >(variable_type);
        if (constant_type.kind == constant_kind::string)
        {
            constexpr_string const string_value = co_await rpnx::querygraph::request< string_static_value_query >(input);
            co_return antestatal_primitive{.value = string_value.bytes};
        }
        if (constant_type.kind == constant_kind::numeric)
        {
            constexpr_numeric const numeric_value = co_await rpnx::querygraph::request< numeric_static_value_query >(input);
            co_return antestatal_primitive{.value = numeric_value.bytes};
        }
    }

    constexpr_input2 constexpr_input;
    constexpr_input.expr = *decl.init_expr;
    constexpr_input.context = input;
    constexpr_input.type = variable_type;
    constexpr_input.antestatal_global_symbol = input;

    co_return co_await rpnx::querygraph::request< constexpr_eval_antestatal_query >(constexpr_input);
}
