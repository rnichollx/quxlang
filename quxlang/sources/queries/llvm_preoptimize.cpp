// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/llvm-backend.hpp>
#include <quxlang/queries/specs/llvm_preoptimize_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

#include <map>
#include <set>

rpnx::querygraph::coroutine< quxlang::llvm_preoptimize_spec > quxlang::llvm_preoptimize_impl(llvm_output_query_input input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(input.output_name);
    bool component_is_canonical = false;
    for (llvm_output_query_input const& identity : component_identities)
    {
        if (identity.output_name == input.output_name &&
            identity.stepping_index == input.stepping_index &&
            identity.component == input.component)
        {
            component_is_canonical = true;
            break;
        }
    }
    if (!component_is_canonical)
    {
        throw semantic_compilation_error(
            "LLVM component is not part of the configured output: " + llvm_output_component_name(input));
    }

    std::set< type_symbol > materialized_types;
    for (llvm_output_query_input const& identity : component_identities)
    {
        llvm_backend::llvm_compilable_unit const& component =
            co_await rpnx::querygraph::request< output_llvm_input_query >(identity);
        std::set< type_symbol > direct = vmir2::directly_materialized_type_indices(component.target_code, dependency_set::native);
        materialized_types.insert(direct.begin(), direct.end());
        for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine : component.inlinable_functions)
        {
            direct = vmir2::directly_materialized_type_indices(routine.second, dependency_set::native);
            materialized_types.insert(direct.begin(), direct.end());
        }
        for (std::pair< type_symbol const, antestatal_value > const& constant : component.antestatal_constants)
        {
            direct = vmir2::directly_materialized_type_indices(constant.second);
            materialized_types.insert(direct.begin(), direct.end());
        }
    }

    llvm_backend::llvm_compilable_unit compilable =
        co_await rpnx::querygraph::request< output_llvm_input_query >(input);
    compilable.type_index_ordinals = vmir2::assign_type_index_ordinals(std::move(materialized_types));
    llvm_backend::llvm_backend backend;
    if (input.component != llvm_output_component::early_init)
    {
        co_return backend.preoptimize(compilable);
    }

    std::map< type_symbol, type_symbol > const& compiler_builtin_manifest =
        co_await rpnx::querygraph::request< llvm_compiler_builtin_manifest_query >(input.output_name);
    llvm_backend::llvm_compilable_unit early_init_compilable = std::move(compilable);
    bool references_unit_test_object = false;
    for (std::pair< type_symbol const, type_symbol > const& object_type : compiler_builtin_manifest)
    {
        early_init_compilable.object_reference_types.insert_or_assign(object_type.first, object_type.second);
        early_init_compilable.global_init_types.insert_or_assign(
            object_type.first,
            initialization_type::init_compiler_builtin);
        references_unit_test_object =
            references_unit_test_object || llvm_backend::is_unit_test_object_symbol(object_type.first);
    }

    type_symbol post_detect_function_array = builtin_symbol{.name = "POST_DETECT_FUNCTION_ARRAY"};
    if (compiler_builtin_manifest.contains(post_detect_function_array) &&
        !early_init_compilable.post_detect_functanoid.has_value())
    {
        throw quxlang::semantic_compilation_error(
            "POST_DETECT_FUNCTION_ARRAY requires a concrete MODULE(RUNTIME)::POST_DETECT functanoid");
    }
    if (references_unit_test_object)
    {
        early_init_compilable.unit_test_objects = llvm_backend::unit_test_object_emission::definitions;
    }
    co_return backend.preoptimize(early_init_compilable);
}
