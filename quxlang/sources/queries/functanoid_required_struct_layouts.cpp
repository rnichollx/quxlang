// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_required_struct_layouts_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

rpnx::querygraph::coroutine< quxlang::functanoid_required_struct_layouts_spec > quxlang::functanoid_required_struct_layouts_impl(functanoid_requirement_input input)
{
    dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
        direct_dependencies_input{.symbol = input.functanoid, .set = input.dependencies});
    co_return dependencies.struct_layouts;
}
