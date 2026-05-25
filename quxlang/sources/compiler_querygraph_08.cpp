// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/extern_linksymbol_spec.hpp>
#include <quxlang/queries/specs/flagset_info_spec.hpp>
#include <quxlang/queries/specs/functanoid_directly_instantiated_functanoids_spec.hpp>
#include <quxlang/queries/specs/functanoid_required_class_layouts_spec.hpp>
#include <quxlang/queries/specs/functanoid_required_type_placements_spec.hpp>
#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_6(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< extern_linksymbol_spec >(extern_linksymbol_impl);
    graph.register_handler_function< flagset_info_spec >(flagset_info_impl);
    graph.register_handler_function< functanoid_directly_instantiated_functanoids_spec >(functanoid_directly_instantiated_functanoids_impl);
    graph.register_handler_function< functanoid_required_class_layouts_spec >(functanoid_required_class_layouts_impl);
    graph.register_handler_function< functanoid_required_type_placements_spec >(functanoid_required_type_placements_impl);
    graph.register_handler_function< functanoid_return_type_spec >(functanoid_return_type_impl);
    graph.register_handler_function< functanoid_sigtype_spec >(functanoid_sigtype_impl);
}
