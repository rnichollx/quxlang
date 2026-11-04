// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CO_COROUTINE_CLEANUP_HEADER_GUARD
#define QUXLANG_CO_COROUTINE_CLEANUP_HEADER_GUARD

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/queries/coroutine_destructor.hpp>
#include <quxlang/queries/type_destruction_capabilities.hpp>
#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>
#include <quxlang/vmir2/coroutine_output_initialization.hpp>
#include <quxlang/vmir2/lifetime_state_difference.hpp>

#include <optional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace quxlang
{
    /** Generates complete coroutine destruction and expands owned cleanup obligations. */
    template < codegen_co_base CoroutineBaseType >
    class coroutine_cleanup_lowering
    {
        using generator_type = co_vmir_generator2< CoroutineBaseType >;
        using block_index = vmir2::block_index;
        using local_index = vmir2::local_index;
        template < typename T >
        using co_type = typename CoroutineBaseType::template cosubroutine< T >;

        generator_type& generator;
        vmir2::functanoid_routine3 lifetime_description;
        std::map< local_index, value_index > local_values;
        std::size_t indexed_values = 1;
        using tail_key = std::tuple< vmir2::lifetime_action_kind, bool, local_index, vmir2::slot_state const*, block_index,
            std::optional< vmir2::exception_catcher >, std::optional< value_index >, std::optional< value_index >,
            std::optional< value_index >, std::optional< block_index > >;
        std::map< tail_key, block_index > cleanup_tails;
        std::map< local_index, block_index > exceptional_cleanup_termination;
        std::optional< vmir2::lifetime_transition_graph > transition_graph;
        std::optional< vmir2::coroutine_output_initialization > output_initialization;
        using sequence_key = std::tuple< vmir2::lifetime_transition_graph::sequence, block_index,
            std::optional< vmir2::exception_catcher >, std::optional< value_index >, std::optional< value_index >,
            std::optional< value_index >, std::optional< block_index > >;
        std::map< sequence_key, block_index > cleanup_sequences;
        using terminal_key = std::tuple< vmir2::vm_terminator, std::optional< vmir2::exception_catcher >,
            std::optional< value_index >, std::optional< value_index >, std::optional< block_index > >;
        std::map< terminal_key, block_index > terminal_blocks;
        std::optional< vmir2::state_map > normal_exit_state;
        std::optional< vmir2::state_map > exceptional_exit_state;

        std::set< block_index > resolved_cleanup_blocks;

        /** Terminates a second exception after suspendable unwinding has acquired the first exception. */
        auto cleanup_termination(block_index source, vmir2::exception_catcher const& catcher) -> block_index
        {
            std::map< local_index, block_index >::const_iterator existing = exceptional_cleanup_termination.find(catcher.exception);
            if (existing != exceptional_cleanup_termination.end())
            {
                return existing->second;
            }
            block_index terminal = generator.generate_subblock(source, "coroutine_cleanup_exception_termination");
            typename generator_type::codegen_block& block = generator.state.blocks.at(terminal);
            block.entry_state.clear();
            block.current_state.clear();
            block.catcher.reset();
            block.catch_exception.reset();
            block.catch_region.reset();
            block.coroutine_handler_root.reset();
            block.coroutine_handler_exception.reset();
            block.lookup_values.clear();
            block.lookup_tombstones.clear();
            generator.set_terminator(terminal, vmir2::panic{.message = "Exception during coroutine exception cleanup"});
            exceptional_cleanup_termination.emplace(catcher.exception, terminal);
            return terminal;
        }

        /** Recognizes a generated complete or base-subobject coroutine destructor. */
        static auto is_coroutine_destructor(type_symbol const& symbol) -> bool
        {
            if (!symbol.template type_is< subsymbol >()) return false;
            std::string const& name = symbol.template get_as< subsymbol >().name;
            return name == "__COROUTINE_DESTRUCTOR" || name == "__COROUTINE_SUBOBJECT_DESTRUCTOR";
        }

        /** Indexes newly created locals without rescanning earlier generated values. */
        auto value_for(local_index local) -> value_index
        {
            while (indexed_values < generator.state.genvalues.size())
            {
                typename generator_type::codegen_value const& value = generator.state.genvalues.at(indexed_values);
                if (value.template type_is< typename generator_type::codegen_local >())
                {
                    local_values.emplace(value.template get_as< typename generator_type::codegen_local >().local_index, value_index(indexed_values));
                }
                ++indexed_values;
            }
            return local_values.at(local);
        }

        /** Adds newly generated local types to the shared semantic lifetime description. */
        auto refresh_lifetimes() -> co_type< void >
        {
            while (lifetime_description.local_types.size() < generator.state.locals.size())
            {
                vmir2::local_type const local = generator.state.locals.at(lifetime_description.local_types.size());
                lifetime_description.local_types.push_back(local);
                if (local.type.template type_is< storage >() || local.type.template type_is< aligned_storage >()) continue;
                if (!generator.state.non_trivial_dtors.contains(local.type))
                {
                    std::optional< type_symbol > const destructor = co_await generator.co_select_default_destructor_entry(local.type, false);
                    if (destructor.has_value()) generator.state.non_trivial_dtors.emplace(local.type, *destructor);
                }
                std::map< type_symbol, type_symbol >::const_iterator selected = generator.state.non_trivial_dtors.find(local.type);
                if (selected != generator.state.non_trivial_dtors.end())
                {
                    lifetime_description.non_trivial_dtors.insert_or_assign(selected->first, selected->second);
                }
            }
        }

        /** Selects the cleanup function associated with one live logical object. */
        auto destructor_for(vmir2::state_map const& state, local_index local) const -> std::optional< type_symbol >
        {
            vmir2::slot_state const& slot = state.at(local);
            if (slot.destruction_started) return std::nullopt;
            if (slot.nontrivial_dtor.has_value()) return slot.nontrivial_dtor->func;
            std::map< type_symbol, type_symbol >::const_iterator destructor = generator.state.non_trivial_dtors.find(generator.state.locals.at(local).type);
            if (destructor == generator.state.non_trivial_dtors.end()) return std::nullopt;
            return destructor->second;
        }

        /** Emits a selected owning destruction while retaining the object until its operation finishes. */
        auto destroy_object(block_index& block, local_index local) -> co_type< void >
        {
            std::optional< type_symbol > const destructor = destructor_for(generator.state.blocks.at(block).current_state, local);
            if (destructor.has_value() && is_coroutine_destructor(*destructor))
            {
                value_index object = value_for(local);
                type_symbol const& type = generator.state.locals.at(local).type;
                value_index reference = is_ref(type) ? generator.copy_ref_value(block, object) : generator.create_reference(block, object, make_mref(type));
                value_index result = co_await generator.co_coroutine_call(block, *destructor,
                    codegen_invocation_args{.named{{"THIS", reference}}}, "CO_CALL_DTOR", local);
                if (generator.current_type(block, result) != type_symbol(void_type{}))
                {
                    throw semantic_compilation_error("Coroutine destruction must complete with VOID");
                }
                generator.emit(block, vmir2::end_lifetime{.of = local, .initialization_rollback = generator.state.expanding_lifetime_cleanup});
                co_return;
            }
            generator.emit(block, vmir2::destroy{.of = local, .initialization_rollback = generator.state.expanding_lifetime_cleanup});
        }

        /** Destroys the initialized prefix of a partial array through a bounded reverse loop. */
        auto destroy_array_prefix(block_index& block, local_index initializer) -> co_type< void >
        {
            array_initializer_type const array = generator.state.locals.at(initializer).type.template get_as< array_initializer_type >();
            type_symbol const& index_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});
            block_index condition = generator.generate_subblock(block, "coroutine_prefix_cleanup_condition");
            block_index element = generator.generate_subblock(block, "coroutine_prefix_cleanup_element");
            block_index complete = generator.generate_subblock(block, "coroutine_prefix_cleanup_complete");
            generator.generate_jump(block, condition);
            value_index count = generator.create_local_value(index_type);
            generator.emit(condition, vmir2::array_init_index{.initializer = initializer, .result = generator.get_local_index(count)});
            value_index nonzero = co_await generator.co_generate_binary(condition, "!=", count, generator.create_small_uint_value(condition, 0, index_type));
            generator.generate_branch(nonzero, condition, element, complete);
            value_index value = generator.create_local_value(array.element_type);
            generator.emit(element, vmir2::array_deinit_previous{.initializer = initializer, .target = generator.get_local_index(value)});
            co_await refresh_lifetimes();
            co_await destroy_object(element, generator.get_local_index(value));
            generator.generate_jump(element, condition);
            block = complete;
            generator.emit(block, vmir2::end_lifetime{.of = initializer, .initialization_rollback = true});
        }

        /** Retains a slot and the semantic object bindings on which its lifetime depends. */
        void retain_slot(vmir2::state_map& target, vmir2::state_map const& source, local_index local)
        {
            std::vector< local_index > pending{local};
            std::set< local_index > visited;
            while (!pending.empty())
            {
                local_index next = pending.back();
                pending.pop_back();
                if (!visited.insert(next).second) continue;
                vmir2::state_map::const_iterator entry = source.find(next);
                if (entry == source.end()) continue;
                vmir2::state_map::const_iterator existing = target.find(next);
                if (existing == target.end() || existing->second != entry->second) target[next] = entry->second;
                if (entry->second.delegate_of.has_value()) pending.push_back(*entry->second.delegate_of);
                if (entry->second.array_delegate_of_initializer.has_value()) pending.push_back(*entry->second.array_delegate_of_initializer);
                if (entry->second.storage_origin.has_value()) pending.push_back(*entry->second.storage_origin);
                if (entry->second.lifecycle_phase.has_value())
                {
                    pending.push_back(entry->second.lifecycle_phase->on_value);
                    if (entry->second.lifecycle_phase->subobject_context.has_value())
                    {
                        pending.push_back(entry->second.lifecycle_phase->subobject_context->enclosing_object);
                    }
                }
                if (entry->second.nontrivial_dtor.has_value())
                {
                    vmir2::dtor_spec const& destructor = *entry->second.nontrivial_dtor;
                    for (local_index argument : destructor.args.positional) pending.push_back(argument);
                    for (std::pair< std::string const, local_index > const& argument : destructor.args.named) pending.push_back(argument.second);
                    if (destructor.subobject_context.has_value()) pending.push_back(destructor.subobject_context->enclosing_object);
                }
            }
        }

        /** Emits a shared sequence with a selected continuation and lexical exception context. */
        auto cleanup_sequence(vmir2::lifetime_transition_graph::sequence sequence,
            vmir2::state_map const& source, block_index destination, block_index next,
            std::optional< vmir2::exception_catcher > catcher) -> co_type< block_index >
        {
            if (sequence == 0) co_return next;
            typename generator_type::codegen_block const& continuation = generator.state.blocks.at(next);
            typename generator_type::coroutine_handler_context context{
                continuation.coroutine_handler_root, continuation.coroutine_handler_exception,
                continuation.catch_exception, continuation.catch_region};
            sequence_key sequence_identity{sequence, next, catcher, context.root, context.exception, context.catch_exception, context.catch_region};
            typename std::map< sequence_key, block_index >::const_iterator completed = cleanup_sequences.find(sequence_identity);
            if (completed != cleanup_sequences.end()) co_return completed->second;
            vmir2::lifetime_transition_graph::node const sequence_node = transition_graph->at(sequence);
            if (!sequence_node.action.has_value())
            {
                next = co_await cleanup_sequence(sequence_node.second, source, destination, next, catcher);
                next = co_await cleanup_sequence(sequence_node.first, source, destination, next, catcher);
                cleanup_sequences.emplace(std::move(sequence_identity), next);
                co_return next;
            }
            if (sequence_node.redundant_end) co_return next;
            vmir2::lifetime_action const action = *sequence_node.action;
            typename std::map< local_index, typename generator_type::coroutine_handler_scope >::const_iterator handler = generator.state.coroutine_handler_scopes.find(action.slot);
            if (handler != generator.state.coroutine_handler_scopes.end()) context = handler->second.before;
            tail_key key{action.kind, action.completed, action.slot, &source.at(action.slot), next, catcher,
                context.root, context.exception, context.catch_exception, context.catch_region};
            typename std::map< tail_key, block_index >::const_iterator existing = cleanup_tails.find(key);
            if (existing != cleanup_tails.end())
            {
                cleanup_sequences.emplace(std::move(sequence_identity), existing->second);
                co_return existing->second;
            }
            generator.state.catcher = catcher;
            generator.state.catch_exception = context.catch_exception;
            generator.state.catch_region = context.catch_region;
            generator.state.coroutine_handler_root = context.root;
            generator.state.coroutine_handler_exception = context.exception;
            block_index node = generator.generate_subblock(destination, "coroutine_cleanup");
            block_index const entry = node;
            vmir2::state_map state = generator.state.blocks.at(next).entry_state;
            retain_slot(state, source, action.slot);
            retain_slot(state, source, generator.state.coroutine->frame);
            retain_slot(state, source, generator.state.coroutine->controller);
            if (context.root.has_value()) retain_slot(state, source, generator.get_local_index(*context.root));
            if (context.exception.has_value()) retain_slot(state, source, generator.get_local_index(*context.exception));
            generator.state.blocks.at(node).entry_state = state;
            generator.state.blocks.at(node).current_state = std::move(state);
            generator.state.coroutine_shielded = true;
            switch (action.kind)
            {
            case vmir2::lifetime_action_kind::require_asynchronous_cleanup:
                throw semantic_compilation_error("Synchronous cleanup cannot destroy completed async-only object " + to_string(generator.state.locals.at(action.slot).type));
            case vmir2::lifetime_action_kind::destroy_object:
                co_await destroy_object(node, action.slot);
                break;
            case vmir2::lifetime_action_kind::destroy_array_prefix:
                co_await destroy_array_prefix(node, action.slot);
                break;
            case vmir2::lifetime_action_kind::abort_initguard:
                generator.emit(node, vmir2::initguard_abort{.lock = action.slot});
                break;
            case vmir2::lifetime_action_kind::restore_phase:
                generator.emit(node, vmir2::struct_phase_leave{.saved_phase = action.slot, .completed = action.completed});
                break;
            case vmir2::lifetime_action_kind::end_lifetime:
                generator.emit(node, vmir2::end_lifetime{.of = action.slot, .initialization_rollback = true});
                break;
            }
            generator.generate_jump(node, next);
            cleanup_tails.emplace(std::move(key), entry);
            cleanup_sequences.emplace(std::move(sequence_identity), entry);
            co_return entry;
        }

        /** Reuses ordered cleanup graphs and their already generated continuation suffixes. */
        auto cleanup_successor(block_index source_block, vmir2::state_map const& source, block_index destination,
            vmir2::lifetime_transition_kind kind = vmir2::lifetime_transition_kind::control_flow,
            bool exceptional_cleanup = false) -> co_type< block_index >
        {
            co_await refresh_lifetimes();
            if (source.snapshot_root() == generator.state.blocks.at(destination).entry_state.snapshot_root()) co_return destination;
            vmir2::lifetime_transition_graph::plan actions = transition_graph->select(source,
                generator.state.blocks.at(destination).entry_state, kind);
            if (actions.empty()) co_return destination;
            std::optional< vmir2::exception_catcher > catcher = generator.state.blocks.at(source_block).catcher;
            if (exceptional_cleanup && catcher.has_value())
            {
                catcher->handler = cleanup_termination(source_block, *catcher);
                catcher->terminate_before_capture = true;
            }
            block_index next = co_await cleanup_sequence(actions.publication, source, destination, destination, catcher);
            next = co_await cleanup_sequence(actions.ordinary, source, destination, next, catcher);
            co_return co_await cleanup_sequence(actions.delegates, source, destination, next, catcher);
        }

        /** Restores lexical handler metadata before appending code to a transformed block. */
        void enter_block(block_index block)
        {
            typename generator_type::codegen_block const& source = generator.state.blocks.at(block);
            generator.state.catcher = source.catcher;
            generator.state.catch_exception = source.catch_exception;
            generator.state.catch_region = source.catch_region;
            generator.state.coroutine_handler_root = source.coroutine_handler_root;
            generator.state.coroutine_handler_exception = source.coroutine_handler_exception;
        }

        /** Routes one throwing instruction through an owned-exception boundary before asynchronous cleanup. */
        auto emit_invocation(block_index& block, vmir2::vm_instruction const& instruction) -> co_type< void >
        {
            std::optional< vmir2::exception_catcher > const catcher = generator.state.blocks.at(block).catcher;
            if (!catcher.has_value())
            {
                generator.emit(block, instruction);
                co_return;
            }
            if (catcher->terminate_before_capture)
            {
                generator.emit(block, instruction);
                co_return;
            }
            if (instruction.template type_is< vmir2::invoke >())
            {
                type_symbol const& callable = instruction.template get_as< vmir2::invoke >().what;
                if (callable.template type_is< instanciation_reference >() &&
                    co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(callable.template get_as< instanciation_reference >()))
                {
                    generator.emit(block, instruction);
                    co_return;
                }
            }
            vmir2::state_map const before = generator.state.blocks.at(block).current_state;
            vmir2::state_map failure = before;
            vmir2::codegen_state_engine(failure, generator.state.locals, generator.state.params).apply(instruction);
            vmir2::state_map const after = failure;
            vmir2::visit_lifetime_state_difference(after, before,
                [&](vmir2::state_map::value_type const& entry, vmir2::slot_state const* previous)
                {
                    if (!entry.second.alive() || (previous != nullptr && previous->alive())) return;
                    if (previous == nullptr) failure.erase(entry.first);
                    else failure[entry.first] = *previous;
                });
            block_index cleanup = co_await cleanup_successor(block, failure, catcher->handler,
                vmir2::lifetime_transition_kind::control_flow, true);
            enter_block(block);
            if (cleanup == catcher->handler)
            {
                generator.emit(block, instruction);
                co_return;
            }
            block_index capture = generator.generate_subblock(block, "coroutine_exception_capture");
            generator.state.blocks.at(capture).entry_state = failure;
            generator.state.blocks.at(capture).current_state = std::move(failure);
            generator.generate_jump(capture, cleanup);
            resolved_cleanup_blocks.insert(capture);
            generator.state.catcher = vmir2::exception_catcher{.exception = catcher->exception, .handler = capture};
            block_index call = generator.generate_subblock(block, "coroutine_cleanup_protected_call");
            generator.generate_jump(block, call);
            generator.emit(call, instruction);
            resolved_cleanup_blocks.insert(call);
            generator.state.catcher = catcher;
            block_index continuation = generator.generate_subblock(call, "coroutine_cleanup_call_complete");
            generator.generate_jump(call, continuation);
            block = continuation;
        }

        /** Registers a subobject before a user destructor body can suspend or throw. */
        auto register_subobject(block_index& block, value_index reference, type_symbol const& type, bool subobject,
            std::optional< vmir2::destructor_subobject_context > context = std::nullopt) -> co_type< bool >
        {
            std::optional< type_symbol > const destructor = co_await generator.co_select_default_destructor_entry(type, subobject);
            if (!destructor.has_value()) co_return false;
            if (is_coroutine_destructor(*destructor))
            {
                generator.emit(block, vmir2::defer_nontrivial_dtor{
                    .func = *destructor,
                    .on_value = generator.get_local_index(reference),
                    .args = vmir2::invocation_args{.named{{"THIS", generator.get_local_index(reference)}}},
                    .subobject_context = std::move(context),
                });
                co_return true;
            }
            generator.emit_deferred_destructor(block, reference, *destructor, std::move(context));
            co_return true;
        }

        /** Creates field and base bindings in construction order for reverse destruction. */
        auto bind_struct_subobjects(block_index& block, value_index object, type_symbol const& type, bool subobject) -> co_type< std::vector< value_index > >
        {
            std::vector< value_index > bindings;
            struct_inheritance_info const& inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(type);
            if (!subobject)
            {
                for (std::size_t index = 0; index < inheritance.virtual_base_order.size(); ++index)
                {
                    type_symbol const& base_type = inheritance.virtual_base_order.at(index);
                    value_index reference = generator.create_local_value(make_mref(base_type));
                    generator.emit(block, vmir2::inheritance_cast{
                        .source = generator.get_local_index(generator.copy_ref_value(block, object)),
                        .result = generator.get_local_index(reference),
                        .path = generator.canonical_virtual_base_path(inheritance, base_type),
                    });
                    if (co_await register_subobject(block, reference, base_type, true, vmir2::destructor_subobject_context{
                        .enclosing_object = generator.get_local_index(object),
                        .selector = vmir2::struct_init_virtual_base_selector{.virtual_base_ordinal = index},
                    })) bindings.push_back(reference);
                }
            }
            for (struct_base_declaration const& base : inheritance.direct_bases)
            {
                if (base.kind == inheritance_kind::virtual_) continue;
                value_index reference = generator.create_local_value(make_mref(base.base_type));
                struct_subobject_path path;
                path.steps.push_back(struct_subobject_path_step{.direct_base_ordinal = base.declaration_ordinal, .kind = base.kind, .base_type = base.base_type});
                generator.emit(block, vmir2::inheritance_cast{
                    .source = generator.get_local_index(generator.copy_ref_value(block, object)),
                    .result = generator.get_local_index(reference),
                    .path = std::move(path),
                });
                if (co_await register_subobject(block, reference, base.base_type, true, vmir2::destructor_subobject_context{
                    .enclosing_object = generator.get_local_index(object),
                    .selector = vmir2::struct_init_direct_base_selector{.direct_base_ordinal = base.declaration_ordinal},
                })) bindings.push_back(reference);
            }
            std::vector< struct_field > const& fields = co_await rpnx::querygraph::request< struct_field_list_query >(type);
            for (struct_field const& field : fields)
            {
                std::optional< type_symbol > const field_type = generator.storage_type_for_attached_field(field.type);
                if (!field_type.has_value()) continue;
                value_index reference = co_await generator.co_generate_dot_access(block, generator.copy_ref_value(block, object), field.name);
                if (field.type.template type_is< attached_type_reference >())
                {
                    reference = generator.attached_binding_carrier_value(block, reference, field.type);
                }
                if (co_await register_subobject(block, reference, *field_type, false)) bindings.push_back(reference);
            }
            co_return bindings;
        }

        /** Destroys a registered subobject through its selected synchronous or coroutine operation. */
        auto destroy_registered(block_index& block, value_index reference) -> co_type< void >
        {
            local_index const local = generator.get_local_index(reference);
            std::optional< vmir2::dtor_spec > const destructor = generator.state.blocks.at(block).current_state.at(local).nontrivial_dtor;
            if (!destructor.has_value()) throw compiler_bug("Coroutine subobject cleanup has no registered destructor");
            if (destructor->func.template type_is< subsymbol >())
            {
                subsymbol const& symbol = destructor->func.template get_as< subsymbol >();
                if (symbol.name == "__COROUTINE_DESTRUCTOR" || symbol.name == "__COROUTINE_SUBOBJECT_DESTRUCTOR")
                {
                    value_index result = co_await generator.co_coroutine_call(block, destructor->func,
                        codegen_invocation_args{.named{{"THIS", generator.copy_ref_value(block, reference)}}}, "CO_CALL_DTOR", local);
                    if (generator.current_type(block, result) != type_symbol(void_type{}))
                    {
                        throw semantic_compilation_error("Coroutine destruction must complete with VOID");
                    }
                    generator.emit(block, vmir2::end_lifetime{.of = local});
                    co_return;
                }
            }
            generator.emit(block, vmir2::destroy{.of = local});
        }

        /** Destroys array elements in reverse index order without unrolling their extent. */
        auto destroy_array(block_index& block, value_index object, array_type const& array) -> co_type< void >
        {
            type_symbol const& index_type = co_await rpnx::querygraph::request< uintpointer_type_query >({});
            value_index remaining = co_await generator.co_generate_typed_expr(block, array.element_count, index_type);
            block_index condition = generator.generate_subblock(block, "coroutine_array_cleanup_condition");
            block_index element = generator.generate_subblock(block, "coroutine_array_cleanup_element");
            block_index complete = generator.generate_subblock(block, "coroutine_array_cleanup_complete");
            generator.generate_jump(block, condition);
            value_index count = co_await generator.co_construct_copy(condition, remaining, index_type);
            value_index nonzero = co_await generator.co_generate_binary(condition, "!=", count, generator.create_small_uint_value(condition, 0, index_type));
            generator.generate_branch(nonzero, condition, element, complete);
            co_await generator.co_generate_unary_postfix(element, "--", generator.materialize_lookup_reference(element, remaining));
            value_index index = co_await generator.co_construct_copy(element, remaining, index_type);
            value_index reference = generator.create_local_value(make_mref(array.element_type));
            generator.emit(element, vmir2::access_array{
                .base_index = generator.get_local_index(generator.copy_ref_value(element, object)),
                .index_index = generator.get_local_index(index),
                .store_index = generator.get_local_index(reference),
            });
            if (co_await register_subobject(element, reference, array.element_type, false))
            {
                co_await destroy_registered(element, reference);
            }
            generator.generate_jump(element, condition);
            block = complete;
        }

        /** Destroys the active fusion payload before releasing boxed storage. */
        auto destroy_fusion(block_index& block, value_index object, type_symbol const& type) -> co_type< void >
        {
            typename generator_type::fusion_codegen_info const info = co_await generator.co_load_fusion_codegen_info(type);
            block_index complete = generator.generate_subblock(block, "coroutine_fusion_cleanup_complete");
            typename generator_type::fusion_dispatch_blocks const dispatch = generator.generate_fusion_dispatch(block, info, object, "coroutine_fusion_cleanup");
            if (dispatch.valueless.has_value()) generator.generate_jump(*dispatch.valueless, complete);
            for (std::size_t index = 0; index < dispatch.alternatives.size(); ++index)
            {
                block_index alternative = dispatch.alternatives.at(index);
                co_await generator.co_destroy_fusion_alternative(alternative, info, object, index);
                generator.generate_jump(alternative, complete);
            }
            block = complete;
        }

      public:
        /** Emits shared cleanup blocks and ordinary coroutine calls for every lifetime exit. */
        auto expand() -> co_type< void >
        {
            /** Restores source-flow validation when lifetime expansion leaves the generator. */
            struct expansion_scope
            {
                bool& active;
                bool previous;
                ~expansion_scope() { active = previous; }
            };
            expansion_scope scope{generator.state.expanding_lifetime_cleanup, generator.state.expanding_lifetime_cleanup};
            generator.state.expanding_lifetime_cleanup = true;
            lifetime_description.parameters = generator.state.params;
            lifetime_description.coroutine = generator.state.coroutine;
            co_await refresh_lifetimes();
            transition_graph.emplace(lifetime_description);
            std::map< local_index, type_symbol > outputs;
            for (vmir2::routine_parameter const& parameter : generator.state.params.positional)
            {
                if (parameter.type.template type_is< nvalue_slot >()) outputs.emplace(parameter.local_index, parameter.type);
            }
            for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : generator.state.params.named)
            {
                if (parameter.second.type.template type_is< nvalue_slot >()) outputs.emplace(parameter.second.local_index, parameter.second.type);
            }
            if (!outputs.empty())
            {
                vmir2::functanoid_routine3 ownership;
                ownership.local_types = generator.state.locals;
                for (typename generator_type::codegen_block const& block : generator.state.blocks)
                {
                    vmir2::executable_block snapshot;
                    snapshot.entry_state = block.entry_state;
                    for (vmir2::vm_instruction const& instruction : block.instructions)
                    {
                        if (instruction.template type_is< vmir2::struct_init_start >()) snapshot.instructions.push_back(instruction);
                    }
                    ownership.blocks.push_back(std::move(snapshot));
                }
                output_initialization.emplace(ownership, outputs);
            }
            for (std::size_t ordinal = 0; ordinal < generator.state.blocks.size(); ++ordinal)
            {
                block_index current(ordinal);
                enter_block(current);
                std::vector< vmir2::vm_instruction > instructions = std::move(generator.state.blocks.at(current).instructions);
                std::optional< vmir2::vm_terminator > terminator = std::move(generator.state.blocks.at(current).terminator);
                generator.state.blocks.at(current).instructions.clear();
                generator.state.blocks.at(current).terminator.reset();
                generator.state.blocks.at(current).current_state = generator.state.blocks.at(current).entry_state;
                for (vmir2::vm_instruction const& instruction : instructions)
                {
                    enter_block(current);
                    if (instruction.template type_is< vmir2::destroy >())
                    {
                        local_index const slot = instruction.template get_as< vmir2::destroy >().of;
                        std::optional< type_symbol > const destructor = destructor_for(generator.state.blocks.at(current).current_state, slot);
                        if (destructor.has_value() && is_coroutine_destructor(*destructor))
                        {
                            co_await destroy_object(current, slot);
                            continue;
                        }
                    }
                    if (!resolved_cleanup_blocks.contains(block_index(ordinal)) &&
                        (instruction.template type_is< vmir2::invoke >() || instruction.template type_is< vmir2::invoke_virtual >() ||
                         instruction.template type_is< vmir2::invoke_indirect >() || instruction.template type_is< vmir2::interface_invoke >()))
                    {
                        co_await emit_invocation(current, instruction);
                    }
                    else
                    {
                        generator.emit(current, instruction);
                    }
                }
                if (!terminator.has_value()) continue;
                if (resolved_cleanup_blocks.contains(block_index(ordinal)))
                {
                    generator.set_terminator(current, std::move(*terminator));
                    continue;
                }
                co_await refresh_lifetimes();
                vmir2::state_map const outgoing = generator.state.blocks.at(current).current_state;
                if (generator.state.coroutine_constructor_returns.contains(block_index(ordinal)) &&
                    (ordinal == 0 || generator.state.blocks.at(block_index(ordinal)).has_incoming_flow) &&
                    output_initialization.has_value() && !output_initialization->completed(outgoing))
                {
                    throw semantic_compilation_error("Coroutine constructor completion requires complete initialization of every output parameter");
                }
                bool terminal_cleanup = false;
                co_await rpnx::apply_visitor< co_type< void > >(*terminator, [&](auto& selected) -> co_type< void >
                {
                    using terminator_type = std::decay_t< decltype(selected) >;
                    if constexpr (std::is_same_v< terminator_type, vmir2::jump >)
                    {
                        selected.target = co_await cleanup_successor(current, outgoing, selected.target);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::branch >)
                    {
                        vmir2::state_map successor = outgoing;
                        vmir2::codegen_state_engine(successor, generator.state.locals, generator.state.params).apply(vmir2::end_lifetime{.of = selected.condition});
                        selected.target_true = co_await cleanup_successor(current, successor, selected.target_true);
                        selected.target_false = co_await cleanup_successor(current, successor, selected.target_false);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::tablebranch >)
                    {
                        vmir2::state_map successor = outgoing;
                        vmir2::codegen_state_engine(successor, generator.state.locals, generator.state.params).apply(vmir2::end_lifetime{.of = selected.index});
                        for (block_index& target : selected.targets) target = co_await cleanup_successor(current, successor, target);
                        selected.default_target = co_await cleanup_successor(current, successor, selected.default_target);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::policy_branch >)
                    {
                        for (block_index& target : selected.targets) target = co_await cleanup_successor(current, outgoing, target);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::runtime_constexpr >)
                    {
                        selected.target_constexpr = co_await cleanup_successor(current, outgoing, selected.target_constexpr);
                        selected.target_native = co_await cleanup_successor(current, outgoing, selected.target_native);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::initguard_try_acquire >)
                    {
                        vmir2::state_map acquired = outgoing;
                        acquired[selected.target_lock] = vmir2::slot_state{.stage = vmir2::slot_stage::full, .storage_valid = true};
                        selected.target_acquired = co_await cleanup_successor(current, acquired, selected.target_acquired);
                        selected.target_already_initialized = co_await cleanup_successor(current, outgoing, selected.target_already_initialized);
                    }
                    else if constexpr (std::is_same_v< terminator_type, vmir2::co_complete > || std::is_same_v< terminator_type, vmir2::ret > || std::is_same_v< terminator_type, vmir2::throw_exception >)
                    {
                        vmir2::lifetime_transition_kind kind = vmir2::lifetime_transition_kind::normal_exit;
                        if constexpr (std::is_same_v< terminator_type, vmir2::throw_exception >)
                        {
                            kind = vmir2::lifetime_transition_kind::exceptional_exit;
                        }
                        else if constexpr (std::is_same_v< terminator_type, vmir2::co_complete >)
                        {
                            if (selected.exceptional_exit || selected.exception.has_value() || selected.cancelled_function.has_value())
                            {
                                kind = vmir2::lifetime_transition_kind::exceptional_exit;
                            }
                        }
                        if (kind == vmir2::lifetime_transition_kind::normal_exit && output_initialization.has_value() && !output_initialization->completed(outgoing))
                        {
                            throw semantic_compilation_error("Coroutine completion requires complete initialization of every output parameter");
                        }
                        std::optional< vmir2::state_map >& exit_state = kind == vmir2::lifetime_transition_kind::normal_exit ? normal_exit_state : exceptional_exit_state;
                        if (!exit_state.has_value())
                        {
                            exit_state.emplace();
                            vmir2::codegen_state_engine engine(*exit_state, generator.state.locals, generator.state.params);
                            if (kind == vmir2::lifetime_transition_kind::normal_exit) engine.apply_normal_exit();
                            else engine.apply_exception_exit();
                            retain_slot(*exit_state, outgoing, generator.state.coroutine->frame);
                            retain_slot(*exit_state, outgoing, generator.state.coroutine->controller);
                        }
                        vmir2::state_map completed = *exit_state;
                        if constexpr (std::is_same_v< terminator_type, vmir2::throw_exception >) retain_slot(completed, outgoing, selected.frame);
                        else if constexpr (std::is_same_v< terminator_type, vmir2::co_complete >)
                        {
                            if (selected.exception.has_value()) retain_slot(completed, outgoing, *selected.exception);
                        }
                        if (outgoing.snapshot_root() != completed.snapshot_root())
                        {
                            enter_block(current);
                            terminator_type terminal = selected;
                            terminal.location.reset();
                            typename generator_type::codegen_block const& source = generator.state.blocks.at(current);
                            terminal_key key{vmir2::vm_terminator(terminal), source.catcher, std::nullopt, std::nullopt, std::nullopt};
                            typename std::map< terminal_key, block_index >::const_iterator previous = terminal_blocks.find(key);
                            block_index completion;
                            if (previous == terminal_blocks.end())
                            {
                                completion = generator.generate_subblock(current, "coroutine_cleanup_terminal");
                                generator.state.blocks.at(completion).entry_state = completed;
                                generator.state.blocks.at(completion).current_state = std::move(completed);
                                generator.state.blocks.at(completion).coroutine_handler_root.reset();
                                generator.state.blocks.at(completion).coroutine_handler_exception.reset();
                                generator.state.blocks.at(completion).catch_exception.reset();
                                generator.state.blocks.at(completion).catch_region.reset();
                                generator.set_terminator(completion, selected);
                                resolved_cleanup_blocks.insert(completion);
                                terminal_blocks.emplace(std::move(key), completion);
                            }
                            else completion = previous->second;
                            block_index cleanup = co_await cleanup_successor(current, outgoing, completion, kind);
                            generator.generate_jump(current, cleanup);
                            terminal_cleanup = true;
                        }
                    }
                    co_return;
                });
                if (!terminal_cleanup) generator.set_terminator(current, std::move(*terminator));
            }

            std::erase_if(generator.state.non_trivial_dtors, [](std::pair< type_symbol const, type_symbol > const& entry) { return is_coroutine_destructor(entry.second); });
            vmir2::lifetime_snapshot_transform< vmir2::state_map > destructor_markers;
            auto remove_coroutine_destructor = [](vmir2::state_map::value_type const& entry) -> std::optional< vmir2::state_map::value_type >
            {
                vmir2::slot_state state = entry.second;
                if (state.nontrivial_dtor.has_value() && is_coroutine_destructor(state.nontrivial_dtor->func)) state.nontrivial_dtor.reset();
                return vmir2::state_map::value_type{entry.first, std::move(state)};
            };
            for (typename generator_type::codegen_block& block : generator.state.blocks)
            {
                std::erase_if(block.instructions, [](vmir2::vm_instruction const& instruction)
                {
                    return instruction.template type_is< vmir2::defer_nontrivial_dtor >() && is_coroutine_destructor(instruction.template get_as< vmir2::defer_nontrivial_dtor >().func);
                });
                for (vmir2::state_map* state : {&block.entry_state, &block.current_state})
                {
                    *state = vmir2::state_map::from_snapshot(destructor_markers.apply(state->snapshot_root(), remove_coroutine_destructor));
                }
            }
            co_return;
        }

        /** Binds cleanup generation to the coroutine's existing typed VMIR builder. */
        explicit coroutine_cleanup_lowering(generator_type& generator) : generator(generator)
        {
        }

        /** Generates a complete-object or base-subobject coroutine destructor. */
        auto generate_destructor(instanciation_reference const& function, type_symbol const& object_type, bool subobject,
            std::optional< instanciation_reference > source_body = std::nullopt) -> co_type< vmir2::functanoid_routine3 >
        {
            co_await generator.co_begin_functanoid(function, dvalue_slot{object_type});
            block_index current(0);
            value_index receiver = generator.local_value_direct_lookup(current, "THIS").value();
            generator.emit(current, vmir2::begin_destruction{.of = generator.get_local_index(receiver)});
            value_index object = generator.create_reference(current, receiver, make_mref(object_type));
            for (std::pair< std::string const, value_index >& binding : generator.state.top_level_lookups)
            {
                if (binding.second == receiver) binding.second = object;
            }
            for (std::pair< std::string const, value_index >& binding : generator.state.top_level_lookups_weak)
            {
                if (binding.second == receiver) binding.second = object;
            }
            std::pair< value_index, block_index > exception_region = co_await generator.co_begin_coroutine_exception_region(current);
            generator.state.coroutine_shielded = true;
            type_symbol controller_type = remove_ref(generator.current_type(current, *generator.state.coroutine_controller));
            coroutine_destructor_selection selected;
            if (source_body.has_value()) selected = coroutine_destructor_selection{.operation = source_body, .is_coroutine = true, .requires_subobject_cleanup = true};
            else selected = co_await rpnx::querygraph::request< coroutine_destructor_query >(coroutine_destructor_request{.object_type = object_type, .controller_type = std::move(controller_type)});
            generator.state.is_noexcept = co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(function);
            class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(object_type);
            if (!subobject && kind == class_kind::struct_ &&
                (co_await rpnx::querygraph::request< struct_runtime_requirements_query >(object_type)).polymorphism != struct_polymorphism_kind::none)
            {
                value_index phase = generator.create_local_value(address_type{});
                generator.emit(current, vmir2::struct_phase_enter{
                    .on_value = generator.get_local_index(object),
                    .saved_phase = generator.get_local_index(phase),
                    .kind = struct_phase_kind::destruction,
                });
            }
            std::vector< value_index > subobjects;
            if (selected.requires_subobject_cleanup && kind == class_kind::struct_)
            {
                subobjects = co_await bind_struct_subobjects(current, object, object_type, subobject);
            }
            if (selected.operation.has_value())
            {
                if (selected.is_coroutine)
                {
                    std::optional< ast2_function_declaration > const& declaration = co_await rpnx::querygraph::request< function_declaration_query >(selected.operation->temploid);
                    if (!declaration.has_value()) throw compiler_bug("Selected coroutine destructor has no declaration");
                    if (declaration->header.controller_parameter->name.has_value())
                        generator.state.top_level_lookups[*declaration->header.controller_parameter->name] = *generator.state.coroutine_controller;
                    std::optional< type_symbol > previous_return = generator.state.declared_return_type;
                    generator.state.declared_return_type = co_await generator.co_lookup_declared_return_type(*selected.operation);
                    generator.state.completion = declaration->definition.body.completion;
                    block_index const lifecycle_exit = generator.generate_subblock(current, "coroutine_destructor_body_complete");
                    generator.state.coroutine_lifecycle_exit = lifecycle_exit;
                    co_await generator.co_generate_function_block(current, declaration->definition.body, "destructor_body");
                    if (!generator.has_terminator(current)) co_await generator.co_generate_coroutine_return(current, std::nullopt);
                    generator.validate_no_pending_gotos();
                    generator.state.coroutine_lifecycle_exit.reset();
                    generator.state.declared_return_type = std::move(previous_return);
                    current = lifecycle_exit;
                }
                else
                {
                    codegen_invocation_args arguments{.named{{"THIS", generator.copy_ref_value(current, object)}}};
                    if (selected.operation->params.named.contains("CONTROLLER"))
                        arguments.named.emplace("CONTROLLER", generator.copy_ref_value(current, *generator.state.coroutine_controller));
                    co_await generator.co_gen_call_functanoid(current, *selected.operation, std::move(arguments), allowed_adaptations::destination_rebinding, false);
                }
            }
            if (selected.requires_subobject_cleanup)
            {
                if (object_type.template type_is< array_type >())
                {
                    co_await destroy_array(current, object, object_type.template get_as< array_type >());
                }
                else if (kind == class_kind::union_ || kind == class_kind::variant)
                {
                    co_await destroy_fusion(current, object, object_type);
                }
                else
                {
                    for (std::vector< value_index >::const_reverse_iterator entry = subobjects.crbegin(); entry != subobjects.crend(); ++entry)
                    {
                        co_await destroy_registered(current, *entry);
                    }
                }
            }
            co_await generator.co_generate_coroutine_return(current, std::nullopt);
            co_await generator.co_finish_coroutine_exception_region(exception_region);
            co_await generator.co_generate_dtors();
            co_await expand();
            co_return generator.get_result();
        }
    };

    /** Runs coroutine cleanup expansion through the generator's existing semantic lifetime records. */
    template < codegen_co_base CoroutineBaseType >
    auto co_expand_coroutine_cleanup(co_vmir_generator2< CoroutineBaseType >& generator) -> typename CoroutineBaseType::template cosubroutine< void >
    {
        coroutine_cleanup_lowering< CoroutineBaseType > cleanup(generator);
        co_await cleanup.expand();
    }
}

#endif
