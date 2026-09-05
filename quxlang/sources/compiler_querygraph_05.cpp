// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/queries/specs/class_builtin_spec.hpp>
#include <quxlang/queries/specs/class_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_default_dtor_spec.hpp>
#include <quxlang/queries/specs/struct_field_declaration_list_spec.hpp>
#include <quxlang/queries/specs/public_struct_field_declaration_list_spec.hpp>
#include <quxlang/queries/specs/struct_field_list_spec.hpp>
#include <quxlang/queries/specs/struct_layout_spec.hpp>
#include <quxlang/queries/specs/struct_direct_bases_spec.hpp>
#include <quxlang/queries/specs/struct_inheritance_info_spec.hpp>
#include <quxlang/queries/specs/struct_member_lookup_spec.hpp>
#include <quxlang/queries/specs/struct_conversion_spec.hpp>
#include <quxlang/queries/specs/struct_virtual_slots_spec.hpp>
#include <quxlang/queries/specs/struct_constructor_forms_spec.hpp>
#include <quxlang/queries/specs/struct_runtime_requirements_spec.hpp>
#include <quxlang/queries/specs/struct_runtime_info_spec.hpp>
#include <quxlang/queries/specs/fusion_layout_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_assignment_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_copy_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_default_dtor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp>
#include <quxlang/queries/specs/class_requires_gen_swap_spec.hpp>

auto quxlang::detail::register_compiler_querygraph_handlers_3(compiler_querygraph& querygraph) -> void
{
    auto& graph = querygraph.raw_graph();
    graph.register_handler_function< class_builtin_spec >(class_builtin_impl);
    graph.register_handler_function< class_default_ctor_spec >(class_default_ctor_impl);
    graph.register_handler_function< class_default_dtor_spec >(class_default_dtor_impl);
    graph.register_handler_function< struct_field_declaration_list_spec >(struct_field_declaration_list_impl);
    graph.register_handler_function< public_struct_field_declaration_list_spec >(public_struct_field_declaration_list_impl);
    graph.register_handler_function< struct_field_list_spec >(struct_field_list_impl);
    graph.register_handler_function< struct_layout_spec >(struct_layout_impl);
    graph.register_handler_function< struct_direct_bases_spec >(struct_direct_bases_impl);
    graph.register_handler_function< struct_inheritance_info_spec >(struct_inheritance_info_impl);
    graph.register_handler_function< struct_member_lookup_spec >(struct_member_lookup_impl);
    graph.register_handler_function< struct_conversion_spec >(struct_conversion_impl);
    graph.register_handler_function< struct_virtual_slots_spec >(struct_virtual_slots_impl);
    graph.register_handler_function< struct_constructor_forms_spec >(struct_constructor_forms_impl);
    graph.register_handler_function< struct_runtime_requirements_spec >(struct_runtime_requirements_impl);
    graph.register_handler_function< struct_runtime_info_spec >(struct_runtime_info_impl);
    graph.register_handler_function< fusion_layout_spec >(fusion_layout_impl);
    graph.register_handler_function< class_requires_gen_assignment_spec >(class_requires_gen_assignment_impl);
    graph.register_handler_function< class_requires_gen_copy_ctor_spec >(class_requires_gen_copy_ctor_impl);
    graph.register_handler_function< class_requires_gen_default_ctor_spec >(class_requires_gen_default_ctor_impl);
    graph.register_handler_function< class_requires_gen_default_dtor_spec >(class_requires_gen_default_dtor_impl);
    graph.register_handler_function< class_requires_gen_move_ctor_spec >(class_requires_gen_move_ctor_impl);
}
