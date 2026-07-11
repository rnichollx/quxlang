// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_required_struct_layouts_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

rpnx::querygraph::coroutine< quxlang::functanoid_required_struct_layouts_spec > quxlang::functanoid_required_struct_layouts_impl(functanoid_requirement_input input)
{
    auto const& routine = co_await rpnx::querygraph::request< vm_procedure3_query >(input.functanoid);
    co_return vmir2::directly_required_struct_layouts(routine);
}
