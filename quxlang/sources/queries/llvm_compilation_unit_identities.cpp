// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/llvm_compilation_unit_identities_spec.hpp>
rpnx::querygraph::coroutine< quxlang::llvm_compilation_unit_identities_spec > quxlang::llvm_compilation_unit_identities_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& components = co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));
    std::vector< llvm_output_query_input > result;
    for (llvm_output_query_input const& component : components)
    {
        llvm_component_catalog const& catalog = co_await rpnx::querygraph::request< output_llvm_catalog_query >({component.output_name, component.component});
        backend_llvm_options const& options = co_await rpnx::querygraph::request< output_llvm_backend_options_query >(component.output_name);
        if (!build_type_compiles_procedures(*options.build_type))
        {
            result.push_back(component);
            continue;
        }
        llvm_output_query_input unit = component;
        unit.unit = llvm_support_data_unit{};
        result.push_back(unit);
        if (catalog.root_routine == llvm_backend::root_routine_emission::definition && !catalog.asm_functions.contains(catalog.target_name))
        {
            unit.unit = catalog.target_name;
            result.push_back(unit);
        }
        for (type_symbol const& routine : catalog.inlinable_functions)
        {
            unit.unit = routine;
            result.push_back(unit);
        }
    }
    co_return result;
}
