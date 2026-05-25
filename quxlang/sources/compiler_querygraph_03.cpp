// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/argument_adaptation_is_better_fit_spec.hpp>
#include <quxlang/queries/specs/argument_adaptation_rank_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_class_conversion_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp>
#include <quxlang/queries/specs/argument_initialize_by_template_spec.hpp>
#include <quxlang/queries/specs/antestatal_static_value_spec.hpp>
#include <quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp>
#include <quxlang/queries/specs/bindable_by_reference_objectization_spec.hpp>
#include <quxlang/queries/specs/bindable_by_reference_requalification_spec.hpp>
#include <quxlang/queries/specs/bindable_by_temporary_materialization_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_1(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< argument_adaptation_is_better_fit_spec >(argument_adaptation_is_better_fit_impl);
    graph.register_handler_function< argument_adaptation_rank_spec >(argument_adaptation_rank_impl);
    graph.register_handler_function< argument_initialize_by_class_conversion_spec >(argument_initialize_by_class_conversion_impl);
    graph.register_handler_function< argument_initialize_by_intrinsic_spec >(argument_initialize_by_intrinsic_impl);
    graph.register_handler_function< argument_initialize_by_template_spec >(argument_initialize_by_template_impl);
    graph.register_handler_function< antestatal_static_value_spec >(antestatal_static_value_impl);
    graph.register_handler_function< asm_procedure_from_symbol_spec >(asm_procedure_from_symbol_impl);
    graph.register_handler_function< bindable_by_reference_objectization_spec >(bindable_by_reference_objectization_impl);
    graph.register_handler_function< bindable_by_reference_requalification_spec >(bindable_by_reference_requalification_impl);
    graph.register_handler_function< bindable_by_temporary_materialization_spec >(bindable_by_temporary_materialization_impl);
}
