// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/llvm-backend.hpp>
#include <quxlang/llvm_type_dependencies.hpp>
#include <quxlang/queries/specs/llvm_preoptimize_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

#include <deque>
#include <map>
#include <set>

rpnx::querygraph::coroutine< quxlang::llvm_preoptimize_spec > quxlang::llvm_preoptimize_impl(llvm_output_query_input input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(input.output_name);
    bool component_is_canonical = false;
    for (llvm_output_query_input const& identity : component_identities)
    {
        if (identity.output_name == input.output_name &&
            identity.stepping_index == input.stepping_index &&
            identity.component == input.component)
        {
            component_is_canonical = true;
            break;
        }
    }
    if (!component_is_canonical)
    {
        throw semantic_compilation_error(
            "LLVM component is not part of the configured output: " + llvm_output_component_name(input));
    }

    rpnx::cow< std::map< type_symbol, std::uint64_t > > const& ordinals = co_await rpnx::querygraph::request< output_llvm_type_ordinals_query >(input.output_name);

    llvm_component_catalog const& catalog = co_await rpnx::querygraph::request< output_llvm_catalog_query >({input.output_name, input.component});
    backend_llvm_options const& options = co_await rpnx::querygraph::request< output_llvm_backend_options_query >(input.output_name);
    std::set< type_symbol > imported;
    if (input.unit.type_is< std::monostate >())
    {
        imported = catalog.inlinable_functions;
    }
    else if (!build_type_compiles_procedures(*options.build_type))
    {
        throw semantic_compilation_error("Procedure partition requested for a whole-component build type");
    }
    else if (input.unit.type_is< type_symbol >())
    {
        type_symbol procedure = input.unit.get_as< type_symbol >();
        if (!catalog.inlinable_functions.contains(procedure) && (procedure != catalog.target_name || catalog.root_routine != llvm_backend::root_routine_emission::definition))
        {
            throw semantic_compilation_error("Procedure is not owned by this LLVM component: " + to_string(procedure));
        }
        if (*options.build_type == build_type::quick)
        {
            std::deque< std::pair< type_symbol, std::size_t > > pending{{procedure, 0}};
            std::set< type_symbol > seen{procedure};
            while (!pending.empty())
            {
                std::pair< type_symbol, std::size_t > current = std::move(pending.front());
                pending.pop_front();
                if (current.second == 2)
                {
                    continue;
                }
                dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{.symbol = current.first, .set = dependency_set::native});
                std::set< type_symbol > calls;
                for (std::pair< type_symbol const, std::optional< source_location > > const& edge : direct.functanoids)
                {
                    calls.insert(edge.first);
                }
                for (vmir_runtime_dependency dependency : direct.runtime_dependencies)
                {
                    llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure_from_dependency(dependency)};
                    calls.insert(catalog.runtime_procedures.at(reference));
                }
                for (type_symbol const& call : calls)
                {
                    if ((call == catalog.target_name || catalog.inlinable_functions.contains(call)) && seen.insert(call).second)
                    {
                        imported.insert(call);
                        pending.emplace_back(call, current.second + 1);
                    }
                }
            }
        }
    }
    machine_target_info const& machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
    std::vector< cpu_stepping_configuration > const& steppings = co_await rpnx::querygraph::request< output_steppings_query >(input.output_name);
    class_placement_info pointer_placement{.size = machine.pointer_size_bytes(), .alignment = machine.pointer_align()};
    llvm_backend::llvm_compilable_unit compilable;
    std::set< llvm_output_component > layout_components;
    for (llvm_output_query_input const& component : component_identities)
    {
        if (layout_components.insert(component.component).second)
        {
            llvm_component_catalog const& layout = co_await rpnx::querygraph::request< output_llvm_catalog_query >({input.output_name, component.component});
            compilable.struct_phase_assignment_capacity = std::max(compilable.struct_phase_assignment_capacity, layout.struct_phase_assignment_capacity);
        }
    }
    compilable.target_name = catalog.target_name;
    compilable.place_definitions_in_stepping_section = catalog.place_definitions_in_stepping_section;
    compilable.suffix_generated_function_symbols = catalog.suffix_generated_function_symbols;
    compilable.definitions_are_coalescible = catalog.definitions_are_coalescible;
    compilable.emit_process_entrypoint = catalog.emit_process_entrypoint;
    compilable.root_routine = catalog.root_routine;
    compilable.defines_compiler_builtin_objects = catalog.defines_compiler_builtin_objects;
    compilable.whole_module_output_kind = catalog.whole_module_output_kind;
    compilable.executable_entry_symbol = catalog.executable_entry_symbol;
    compilable.post_detect_functanoid = catalog.post_detect_functanoid;
    compilable.unit_test_objects = catalog.unit_test_objects;
    compilable.runtime_procedures = catalog.runtime_procedures;
    compilable.extern_procedures = catalog.extern_procedures;
    compilable.optional_extern_procedures = catalog.optional_extern_procedures;
    compilable.extern_procedure_libraries = catalog.extern_procedure_libraries;
    compilable.extern_procedure_versions = catalog.extern_procedure_versions;
    compilable.assembly_referenced_procedures = catalog.assembly_referenced_procedures;
    compilable.stepping_index = input.stepping_index;
    compilable.machine_target = llvm_backend::llvm_compilation_target_for_stepping(machine, *options.build_type, steppings.at(input.stepping_index));
    compilable.whole_module = !input.unit.type_is< type_symbol >();
    compilable.partitioned = !input.unit.type_is< std::monostate >();
    compilable.owns_support_data = compilable.whole_module;
    llvm_output_query_input component = input;
    component.unit = std::monostate{};
    compilable.support_symbol_suffix = "$" + llvm_output_component_name(component);
    compilable.type_index_ordinals = ordinals;
    if (input.component == llvm_output_component::early_init && catalog.place_definitions_in_stepping_section)
    {
        compilable.stepping_support = llvm_backend::cpu_stepping_support{.steppings = steppings, .attribute_detectors = catalog.attribute_detectors};
    }
    if (input.unit.type_is< llvm_support_data_unit >())
    {
        compilable.root_routine = llvm_backend::root_routine_emission::external_declaration;
    }
    if (input.unit.type_is< type_symbol >())
    {
        compilable.target_name = input.unit.get_as< type_symbol >();
        compilable.root_routine = llvm_backend::root_routine_emission::definition;
        compilable.emit_process_entrypoint = false;
        compilable.defines_compiler_builtin_objects = false;
        compilable.unit_test_objects = llvm_backend::unit_test_object_emission::external_declarations;
        compilable.stepping_support.reset();
    }
    std::set< type_symbol > routines;
    std::set< type_symbol > constants;
    std::set< type_symbol > objects;
    std::set< type_symbol > asm_interfaces;
    std::set< type_symbol > type_roots;
    if (compilable.owns_support_data)
    {
        routines = catalog.inlinable_functions;
        routines.insert(catalog.target_name);
        constants = catalog.antestatal_constants;
        asm_interfaces = catalog.asm_callable_interfaces;
        compilable.unit_tests = catalog.unit_tests;
        compilable.procedure_linksymbols = catalog.procedure_linksymbols;
        compilable.object_reference_types = catalog.object_reference_types;
        compilable.global_init_types = catalog.global_init_types;
        for (std::set< type_symbol > const* types : {&catalog.type_placements, &catalog.interface_slots, &catalog.enum_infos, &catalog.flagset_infos, &catalog.struct_layouts, &catalog.struct_runtime_infos, &catalog.union_infos, &catalog.variant_infos, &catalog.fusion_layouts})
        {
            type_roots.insert(types->begin(), types->end());
        }
    }
    else
    {
        std::set< type_symbol > bodies = imported;
        bodies.insert(compilable.target_name);
        routines = bodies;
        for (type_symbol const& body : bodies)
        {
            dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{.symbol = body, .set = dependency_set::native});
            for (std::set< type_symbol > const* types : {&direct.type_placements, &direct.struct_layouts, &direct.struct_runtime_infos, &direct.fusion_layouts})
            {
                type_roots.insert(types->begin(), types->end());
            }
            objects.insert(direct.global_roots.begin(), direct.global_roots.end());
            for (std::pair< type_symbol const, std::optional< source_location > > const& call : direct.functanoids)
            {
                if (call.first == catalog.target_name || catalog.inlinable_functions.contains(call.first))
                {
                    routines.insert(call.first);
                }
                if (catalog.asm_callable_interfaces.contains(call.first))
                {
                    asm_interfaces.insert(call.first);
                }
            }
            for (vmir_runtime_dependency dependency : direct.runtime_dependencies)
            {
                llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure_from_dependency(dependency)};
                routines.insert(catalog.runtime_procedures.at(reference));
            }
            for (type_symbol const& constant : direct.antestatal_globals)
            {
                if (catalog.antestatal_constants.contains(constant))
                {
                    constants.insert(constant);
                }
            }
            for (static_snapshot_ref const& snapshot : direct.static_snapshots)
            {
                constants.insert(snapshot);
                routines.insert(snapshot.functanoid);
            }
        }
        objects.insert(constants.begin(), constants.end());
        for (type_symbol const& object : objects)
        {
            if (catalog.object_reference_types.contains(object))
            {
                compilable.object_reference_types.emplace(object, catalog.object_reference_types.at(object));
            }
            if (catalog.global_init_types.contains(object))
            {
                compilable.global_init_types.emplace(object, catalog.global_init_types.at(object));
            }
        }
        for (std::set< type_symbol > const* symbols : {&routines, &asm_interfaces})
        {
            for (type_symbol const& symbol : *symbols)
            {
                if (catalog.procedure_linksymbols.contains(symbol))
                {
                    compilable.procedure_linksymbols.emplace(symbol, catalog.procedure_linksymbols.at(symbol));
                }
            }
        }
    }
    std::set< type_symbol > test_procedures;
    for (llvm_backend::unit_test_entry const& test : catalog.unit_tests)
    {
        test_procedures.insert(test.procedure_symbol);
        if (!compilable.owns_support_data && routines.contains(test.procedure_symbol))
        {
            compilable.unit_tests.push_back(test);
        }
    }
    for (type_symbol const& procedure : routines)
    {
        vmir2::functanoid_routine3 const* routine;
        if (test_procedures.contains(procedure))
        {
            routine = &(co_await rpnx::querygraph::request< unit_test_vmir_query >(procedure));
        }
        else
        {
            routine = &(co_await rpnx::querygraph::request< vm_procedure3_query >(procedure.get_as< instanciation_reference >()));
        }
        if (procedure != catalog.target_name || catalog.root_routine == llvm_backend::root_routine_emission::definition)
        {
            compilable.procedure_declarations.emplace(procedure, routine);
        }
        if (procedure == compilable.target_name)
        {
            compilable.target_code = routine;
        }
        else if (imported.contains(procedure))
        {
            compilable.inlinable_functions.emplace(procedure, *routine);
        }
        for (vmir2::routine_parameter const& parameter : routine->parameters.positional)
        {
            type_roots.insert(parameter.type);
        }
        for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine->parameters.named)
        {
            type_roots.insert(parameter.second.type);
        }
    }
    if (compilable.owns_support_data)
    {
        for (type_symbol const& symbol : catalog.asm_functions)
        {
            compilable.asm_functions.emplace(symbol, co_await rpnx::querygraph::request< asm_procedure_from_symbol_query >(symbol));
        }
    }
    for (type_symbol const& symbol : asm_interfaces)
    {
        asm_procedure const& procedure = co_await rpnx::querygraph::request< asm_procedure_from_symbol_query >(symbol);
        compilable.asm_callable_interfaces.emplace(symbol, *procedure.callable_interface);
        for (asm_argument_binding const& parameter : procedure.callable_interface->args)
        {
            type_roots.insert(parameter.type);
        }
        if (procedure.callable_interface->return_type.has_value())
        {
            type_roots.insert(*procedure.callable_interface->return_type);
        }
    }
    for (type_symbol const& symbol : constants)
    {
        if (symbol.type_is< static_snapshot_ref >())
        {
            static_snapshot_ref const& snapshot = symbol.get_as< static_snapshot_ref >();
            vmir2::functanoid_routine3 const& routine = *compilable.procedure_declarations.at(snapshot.functanoid);
            compilable.antestatal_constants.emplace(symbol, routine.static_snapshots.at(snapshot).value);
        }
        else
        {
            compilable.antestatal_constants.emplace(symbol, co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol));
        }
    }
    for (std::pair< type_symbol const, type_symbol > const& object : compilable.object_reference_types)
    {
        type_roots.insert(object.second);
    }
    std::set< type_symbol > seen_types;
    auto enqueue_type = [&](type_symbol const& type)
    {
        if (!seen_types.contains(type))
        {
            type_roots.insert(type);
        }
    };
    while (!type_roots.empty())
    {
        type_symbol type = *type_roots.begin();
        type_roots.erase(type_roots.begin());
        seen_types.insert(type);
        llvm_backend::visit_storage_type_dependencies(type, enqueue_type);
        if (catalog.type_placements.contains(type))
        {
            if (type.type_is< size_type >() || type.type_is< address_type >())
            {
                compilable.type_placements.emplace(type, std::cref(pointer_placement));
            }
            else
            {
                compilable.type_placements.emplace(type, co_await rpnx::querygraph::request< class_placement_info_query >(type));
            }
        }
        if (catalog.interface_slots.contains(type))
        {
            std::vector< interface_slot > const& slots = co_await rpnx::querygraph::request< interface_slot_list_query >(type);
            compilable.interface_slots.emplace(type, std::cref(slots));
            for (interface_slot const& slot : slots)
            {
                for (type_symbol const& parameter : slot.key.concrete_params.positional)
                {
                    enqueue_type(parameter);
                }
                for (std::pair< std::string const, type_symbol > const& parameter : slot.key.concrete_params.named)
                {
                    enqueue_type(parameter.second);
                }
                if (slot.key.concrete_return_type.has_value())
                {
                    enqueue_type(*slot.key.concrete_return_type);
                }
            }
        }
        if (catalog.enum_infos.contains(type))
        {
            compilable.enum_infos.emplace(type, co_await rpnx::querygraph::request< enum_info_query >(type));
        }
        if (catalog.flagset_infos.contains(type))
        {
            compilable.flagset_infos.emplace(type, co_await rpnx::querygraph::request< flagset_info_query >(type));
        }
        if (catalog.struct_layouts.contains(type))
        {
            struct_layout const& layout = co_await rpnx::querygraph::request< struct_layout_query >(type);
            compilable.struct_layouts.emplace(type, layout);
            for (struct_field_info const& field : layout.fields)
            {
                enqueue_type(field.type);
            }
            for (struct_base_layout_info const& base : layout.direct_bases)
            {
                enqueue_type(base.type);
            }
            for (struct_virtual_base_layout_info const& base : layout.virtual_bases)
            {
                enqueue_type(base.type);
            }
        }
        if (catalog.struct_runtime_infos.contains(type))
        {
            struct_runtime_info const& runtime = co_await rpnx::querygraph::request< struct_runtime_info_query >(type);
            compilable.struct_runtime_infos.emplace(type, runtime);
            for (struct_runtime_subobject const& subobject : runtime.subobjects)
            {
                enqueue_type(subobject.type);
            }
            for (struct_phase_descriptor_group const& group : runtime.descriptor_groups)
            {
                enqueue_type(group.phase.active_type);
            }
            for (struct_runtime_cast_record const& cast : runtime.cast_records)
            {
                enqueue_type(cast.target_type);
            }
        }
        if (catalog.union_infos.contains(type))
        {
            union_info const& info = co_await rpnx::querygraph::request< union_info_query >(type);
            compilable.union_infos.emplace(type, info);
            for (union_option_info const& option : info.options)
            {
                enqueue_type(option.type);
            }
        }
        if (catalog.variant_infos.contains(type))
        {
            variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(type);
            compilable.variant_infos.emplace(type, info);
            for (type_symbol const& alternative : info.alternatives)
            {
                enqueue_type(alternative);
            }
        }
        if (catalog.fusion_layouts.contains(type))
        {
            compilable.fusion_layouts.emplace(type, co_await rpnx::querygraph::request< fusion_layout_query >(type));
        }
    }
    compilable.source_index = &(co_await rpnx::querygraph::request< indexed_source_bundle_query >(std::monostate{}));
    llvm_backend::llvm_backend backend;
    if (input.component != llvm_output_component::early_init || !compilable.owns_support_data)
    {
        co_return backend.preoptimize(compilable);
    }

    std::map< type_symbol, type_symbol > const& compiler_builtin_manifest =
        co_await rpnx::querygraph::request< llvm_compiler_builtin_manifest_query >(input.output_name);
    llvm_backend::llvm_compilable_unit early_init_compilable = std::move(compilable);
    bool references_unit_test_object = false;
    for (std::pair< type_symbol const, type_symbol > const& object_type : compiler_builtin_manifest)
    {
        early_init_compilable.object_reference_types.insert_or_assign(object_type.first, object_type.second);
        early_init_compilable.global_init_types.insert_or_assign(
            object_type.first,
            initialization_type::init_compiler_builtin);
        references_unit_test_object =
            references_unit_test_object || llvm_backend::is_unit_test_object_symbol(object_type.first);
    }

    type_symbol post_detect_function_array = builtin_symbol{.name = "POST_DETECT_FUNCTION_ARRAY"};
    if (compiler_builtin_manifest.contains(post_detect_function_array) &&
        !early_init_compilable.post_detect_functanoid.has_value())
    {
        throw quxlang::semantic_compilation_error(
            "POST_DETECT_FUNCTION_ARRAY requires a concrete MODULE(RUNTIME)::POST_DETECT functanoid");
    }
    if (references_unit_test_object)
    {
        early_init_compilable.unit_test_objects = llvm_backend::unit_test_object_emission::definitions;
    }
    co_return backend.preoptimize(early_init_compilable);
}
