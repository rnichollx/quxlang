// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/convertible_by_call_spec.hpp>
#include <quxlang/queries/specs/declaroids_spec.hpp>
#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_initialize_spec.hpp>
#include <quxlang/queries/specs/ensig_tempars_spec.hpp>
#include <quxlang/queries/specs/enum_info_spec.hpp>
#include <quxlang/queries/specs/exists_spec.hpp>
#include <quxlang/queries/specs/extern_linksymbol_spec.hpp>
#include <quxlang/queries/specs/flagset_info_spec.hpp>
#include <quxlang/queries/specs/functanoid_directly_instantiated_functanoids_spec.hpp>
#include <quxlang/queries/specs/functanoid_required_class_layouts_spec.hpp>
#include <quxlang/queries/specs/functanoid_required_type_placements_spec.hpp>
#include <quxlang/queries/specs/functanoid_return_type_spec.hpp>
#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_3(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< convertible_by_call_spec >(convertible_by_call_impl);
    graph.register_handler_function< declaroids_spec >(declaroids_impl);
    graph.register_handler_function< ensig_argument_initialize_spec >(ensig_argument_initialize_impl);
    graph.register_handler_function< ensig_initialize_spec >(ensig_initialize_impl);
    graph.register_handler_function< ensig_tempars_spec >(ensig_tempars_impl);
    graph.register_handler_function< enum_info_spec >(enum_info_impl);
    graph.register_handler_function< exists_spec >(exists_impl);
    graph.register_handler_function< extern_linksymbol_spec >(extern_linksymbol_impl);
    graph.register_handler_function< flagset_info_spec >(flagset_info_impl);
    graph.register_handler_function< functanoid_directly_instantiated_functanoids_spec >(functanoid_directly_instantiated_functanoids_impl);
    graph.register_handler_function< functanoid_required_class_layouts_spec >(functanoid_required_class_layouts_impl);
    graph.register_handler_function< functanoid_required_type_placements_spec >(functanoid_required_type_placements_impl);
    graph.register_handler_function< functanoid_return_type_spec >(functanoid_return_type_impl);
    graph.register_handler_function< functanoid_sigtype_spec >(functanoid_sigtype_impl);
}
