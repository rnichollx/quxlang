// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_directly_instantiated_functanoids_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

rpnx::querygraph::coroutine< quxlang::functanoid_directly_instantiated_functanoids_spec > quxlang::functanoid_directly_instantiated_functanoids_impl(functanoid_requirement_input input)
{
    auto const& routine = co_await rpnx::querygraph::request< vm_procedure3_query >(input.functanoid);
    co_return vmir2::directly_instantiated_functanoids(routine);
}
