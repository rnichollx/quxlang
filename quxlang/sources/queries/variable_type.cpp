// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/specs/variable_type_spec.hpp>

#include <vector>

rpnx::querygraph::coroutine< quxlang::variable_type_spec > quxlang::variable_type_impl(type_symbol input)
{
    if (input.type_is< builtin_symbol >())
    {
        std::string const& name = input.get_as< builtin_symbol >().name;
        if (is_cpu_attribute_enabled_name(name))
        {
            co_return bool_type{};
        }
        if (name == "STEPPING_COUNT" || name == "ACTIVE_STEPPING" || name == "UNIT_TEST_COUNT")
        {
            std::optional< type_symbol > const& resolved_size = co_await rpnx::querygraph::request< lookup_query >(
                contextual_type_reference{.context = input, .type = size_type{}});
            if (!resolved_size.has_value())
            {
                throw compiler_bug("Could not resolve SZ for compiler-provided pointer-width object");
            }
            co_return *resolved_size;
        }
        if (name == "MAIN_FUNCTION_ARRAY" || name == "POST_DETECT_FUNCTION_ARRAY")
        {
            if (name == "MAIN_FUNCTION_ARRAY")
            {
                co_return llvm_backend::main_function_array_object_type();
            }
            co_return llvm_backend::post_detect_function_array_object_type();
        }
        if (name == "UNIT_TEST_NAMES")
        {
            co_return llvm_backend::unit_test_names_object_type();
        }
        if (name == "UNIT_TEST_PROC")
        {
            co_return llvm_backend::unit_test_proc_object_type();
        }
    }

    if (typeis< subtag_type >(input))
    {
        auto binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        if (binding.has_value() && binding->template type_is< parameter_value_instantiation >())
        {
            co_return binding->template get_as< parameter_value_instantiation >().type;
        }
        throw quxlang::compiler_bug("Subtag is not a variable.");
    }

    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);

    if (!typeis< ast2_variable_declaration >(sym))
    {
        throw quxlang::compiler_bug("Variable not declared.");
    }

    type_symbol var_decl_type = as< ast2_variable_declaration >(sym).type;
    contextual_type_reference ctx_type_ref = {.context = input, .type = var_decl_type};

    auto var_type = co_await rpnx::querygraph::request< lookup_query >(ctx_type_ref);

    if (!var_type.has_value())
    {
        throw quxlang::semantic_compilation_error("Variable type could not be resolved.");
    }

    co_return var_type.value();
}
