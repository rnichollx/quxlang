// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_directly_instantiated_functanoids_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

rpnx::querygraph::coroutine< quxlang::functanoid_directly_instantiated_functanoids_spec > quxlang::functanoid_directly_instantiated_functanoids_impl(functanoid_requirement_input input)
{
    dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
        direct_dependencies_input{.symbol = input.functanoid, .set = input.dependencies});
    std::set< type_symbol > result;
    for (auto const& [functanoid, _] : dependencies.functanoids) result.insert(functanoid);
    co_return result;
}
