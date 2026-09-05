// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/output_llvm_type_ordinals_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
rpnx::querygraph::coroutine< quxlang::output_llvm_type_ordinals_spec > quxlang::output_llvm_type_ordinals_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& component_identities = co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));
    std::set< type_symbol > materialized_types;
    for (llvm_output_query_input const& identity : component_identities)
    {
        llvm_component_catalog const& component = co_await rpnx::querygraph::request< output_llvm_catalog_query >({identity.output_name, identity.component});
        materialized_types.insert(component.materialized_types.begin(), component.materialized_types.end());
    }

    co_return rpnx::cow< std::map< type_symbol, std::uint64_t > >(vmir2::assign_type_index_ordinals(std::move(materialized_types)));
}
