// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CO_COROUTINE_CLEANUP_HEADER_GUARD
#define QUXLANG_CO_COROUTINE_CLEANUP_HEADER_GUARD

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/co_coroutine_cleanup_dispatch.hpp>
#include <quxlang/co_coroutine_cleanup_storage.hpp>
#include <quxlang/queries/coroutine_destructor.hpp>
#include <quxlang/queries/type_destruction_capabilities.hpp>
#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>
#include <quxlang/vmir2/coroutine_output_initialization.hpp>
#include <quxlang/vmir2/lifetime_state_difference.hpp>

#include <optional>
#include <bit>
#include <limits>
#include <map>
#include <memory>
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

        /** Describes a shared exit from the active cleanup-context stack. */
        struct dynamic_destination
        {
            block_index block;
            block_index target;
            std::optional< local_index > exception_target;
        };

        std::optional< vmir2::lifetime_cleanup_program > cleanup_program;
        std::optional< coroutine_cleanup_dispatch< CoroutineBaseType > > cleanup_dispatch;
        std::optional< block_index > dispatch_entry;
        std::optional< block_index > dispatch_selection;
        std::optional< block_index > dispatch_completion;
        std::optional< block_index > dispatch_failure;
        std::optional< block_index > dispatch_panic;
        std::vector< dynamic_destination > dynamic_destinations;
        std::map< std::pair< block_index, std::optional< local_index > >, std::size_t > dynamic_destination_indices;
        std::map< vmir2::state_map::snapshot_node const*, std::pair< vmir2::state_map::snapshot, bool > > owned_summaries;
        std::uint64_t cleanup_work_remaining = 0;
        std::uint64_t cleanup_work_charged = 0;
        bool inline_cleanup = true;

        /** Abandons optional expansion between complete emitted cleanup actions. */
        struct cleanup_budget_exhausted {};
        std::size_t scanned_cleanup_nodes = 0;
        std::set< block_index > resolved_cleanup_blocks;
        std::map< std::size_t, block_index > dynamic_actions;
        using dynamic_action_key = std::tuple< std::uint8_t, local_index, vmir2::slot_stage, bool, bool, vmir2::dtor_spec const*, std::optional< vmir2::struct_lifecycle_phase > >;
        std::map< dynamic_action_key, block_index > dynamic_action_bodies;

        /** Identifies the selected suspendable cleanup independently of lexical objects and destinations. */
        using async_cleanup_key = std::tuple< std::uint8_t, type_symbol, std::optional< type_symbol >, bool,
            std::optional< vmir2::struct_init_selector >, std::optional< type_symbol > >;

        /** Retains one typed action body and its caller-provided physical object binding. */
        struct async_cleanup_shape
        {
            block_index entry;
            block_index completion;
            value_index object;
            value_index address;
            value_index identity;
            value_index continuation;
            std::optional< value_index > enclosing;
            std::optional< value_index > enclosing_address;
            std::vector< block_index > destinations;
            std::map< local_index, std::size_t > destination_indices;
        };

        std::map< async_cleanup_key, std::size_t > async_cleanup_indices;
        std::vector< std::unique_ptr< async_cleanup_shape > > async_cleanup_shapes;
        std::map< local_index, value_index > forwarded_cleanup_identities;
        std::map< local_index, std::size_t > generated_cleanup_owners;
        std::vector< std::set< std::size_t > > cleanup_dependencies;

        /** Establishes a mutable view of one logical storage cell without collapsing reference payloads. */
        static type_symbol cleanup_storage_reference(type_symbol type)
        {
            return ptrref_type{.target = std::move(type), .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }

        /** Adds the scalar bindings belonging to the active typed action. */
        vmir2::state_map async_cleanup_state(async_cleanup_shape const& shape)
        {
            vmir2::state_map state = cleanup_dispatch->control_state();
            for (value_index binding : {shape.address, shape.identity, shape.continuation})
                state[generator.get_local_index(binding)] = vmir2::slot_state{
                    .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true, .retained_storage = true};
            if (shape.enclosing_address.has_value())
                state[generator.get_local_index(*shape.enclosing_address)] = vmir2::slot_state{
                    .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true, .retained_storage = true};
            return state;
        }

        /** Retires a completed or abandoned invocation before its typed body can be reused. */
        void retire_async_cleanup_bindings(block_index block, async_cleanup_shape const& shape)
        {
            generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(shape.object)});
            if (shape.enclosing.has_value())
            {
                generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(*shape.enclosing)});
                generator.emit(block, vmir2::end_lifetime{.of = generator.get_local_index(*shape.enclosing_address)});
            }
            generator.emit(block, vmir2::end_lifetime{.of = generator.get_local_index(shape.address)});
            generator.emit(block, vmir2::end_lifetime{.of = generator.get_local_index(shape.identity)});
        }

        /** Creates one return path that releases an original captured alias after its shared action. */
        std::size_t async_cleanup_destination(block_index source, async_cleanup_shape& shape, local_index original)
        {
            std::map< local_index, std::size_t >::const_iterator known = shape.destination_indices.find(original);
            if (known != shape.destination_indices.end()) return known->second;
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            block_index block = dynamic_block(source, cleanup_dispatch->control_state(), "coroutine_cleanup_action_return");
            generator.emit(block, vmir2::lifetime_import{.slot = original});
            retire_dynamic_handler(block, original);
            generator.generate_jump(block, *dispatch_entry);
            resolved_cleanup_blocks.insert(block);
            std::size_t ordinal = shape.destinations.size();
            shape.destinations.push_back(block);
            shape.destination_indices.emplace(original, ordinal);
            return ordinal;
        }

        /** Captures the actual cell address used by a statically typed borrowed cleanup view. */
        void bind_async_cleanup_cell(block_index block, local_index original, value_index address, vmir2::slot_state state)
        {
            state.delegate_of.reset();
            state.struct_delegate_selector.reset();
            state.array_delegate_of_initializer.reset();
            state.storage_origin.reset();
            state.struct_delegates.reset();
            state.array_delegates.reset();
            state.array_element_delegates = {};
            state.persistent_storage = true;
            state.is_projection = true;
            state.retained_storage = true;
            state.owns_subobject_lifetime = false;
            generator.emit(block, vmir2::lifetime_import{.slot = original, .state = state});
            generator.emit(block, vmir2::make_reference{.value_index = original, .reference_index = generator.get_local_index(address),
                .storage_only = state.stage == vmir2::slot_stage::dead});
        }

        /** Generates the finite typed implementation selected by an asynchronous cleanup obligation. */
        auto create_async_cleanup_shape(block_index source, async_cleanup_key key, type_symbol const& type, vmir2::slot_state original,
            std::uint8_t operation, std::optional< type_symbol > enclosing_type) -> co_type< std::size_t >
        {
            std::size_t index = async_cleanup_shapes.size();
            std::unique_ptr< async_cleanup_shape > allocated = std::make_unique< async_cleanup_shape >();
            async_cleanup_shape& shape = *allocated;
            shape.object = generator.create_local_value(type);
            shape.address = generator.create_local_value(cleanup_storage_reference(type));
            shape.identity = generator.create_local_value(int_type{.bits = 64, .has_sign = false});
            shape.continuation = generator.create_local_value(int_type{.bits = 64, .has_sign = false});
            generator.state.coroutine->cleanup_aliases.emplace(generator.get_local_index(shape.object), generator.get_local_index(shape.address));
            forwarded_cleanup_identities.emplace(generator.get_local_index(shape.object), shape.identity);
            if (enclosing_type.has_value())
            {
                shape.enclosing = generator.create_local_value(*enclosing_type);
                shape.enclosing_address = generator.create_local_value(cleanup_storage_reference(*enclosing_type));
                generator.state.coroutine->cleanup_aliases.emplace(generator.get_local_index(*shape.enclosing),
                    generator.get_local_index(*shape.enclosing_address));
            }
            async_cleanup_shapes.push_back(std::move(allocated));
            cleanup_dependencies.emplace_back();
            async_cleanup_indices.emplace(std::move(key), index);
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            vmir2::state_map controls = async_cleanup_state(shape);
            shape.entry = dynamic_block(source, controls, "coroutine_cleanup_typed_action");
            block_index block = shape.entry;
            vmir2::slot_state selected = detached_cleanup_state(std::move(original));
            selected.persistent_storage = true;
            if (selected.nontrivial_dtor.has_value())
            {
                vmir2::dtor_spec destructor = *selected.nontrivial_dtor;
                destructor.args = vmir2::invocation_args{.named = {{"THIS", generator.get_local_index(shape.object)}}};
                if (destructor.subobject_context.has_value())
                    destructor.subobject_context->enclosing_object = generator.get_local_index(*shape.enclosing);
                selected.nontrivial_dtor = std::move(destructor);
            }
            if (shape.enclosing.has_value())
                import_cleanup_operand(block, generator.get_local_index(*shape.enclosing), !is_ref(*enclosing_type));
            generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(shape.object), .state = selected});
            vmir2::state_map failure_state = generator.state.blocks.at(block).current_state;
            vmir2::slot_state borrowed = selected;
            borrowed.is_projection = true;
            borrowed.owns_subobject_lifetime = false;
            borrowed.destruction_started = true;
            failure_state[generator.get_local_index(shape.object)] = std::move(borrowed);
            block_index failure = dynamic_block(block, failure_state, "coroutine_cleanup_typed_action_failure");
            retire_async_cleanup_bindings(failure, shape);
            generator.emit(failure, vmir2::end_lifetime{.of = generator.get_local_index(shape.continuation)});
            generator.emit(failure, vmir2::lifetime_transfer{.state = cleanup_dispatch->control_state()});
            generator.generate_jump(failure, *dispatch_failure);
            resolved_cleanup_blocks.insert(failure);
            vmir2::exception_catcher catcher{.exception = generator.get_local_index(cleanup_dispatch->exception), .handler = failure};
            generator.state.blocks.at(block).catcher = catcher;
            generator.state.catcher = catcher;
            generator.state.coroutine_shielded = true;
            std::size_t first_temporary = generator.state.locals.size();
            if (operation == 0) co_await destroy_object(block, generator.get_local_index(shape.object));
            else co_await destroy_array_prefix(block, generator.get_local_index(shape.object));
            for (std::size_t local = first_temporary; local < generator.state.locals.size(); ++local)
                generated_cleanup_owners.emplace(local_index(local), index);
            shape.completion = dynamic_block(block, async_cleanup_state(shape), "coroutine_cleanup_typed_action_complete");
            generator.generate_jump(block, shape.completion);
            retire_async_cleanup_bindings(shape.completion, shape);
            generator.set_terminator(shape.completion, vmir2::tablebranch{.index = generator.get_local_index(shape.continuation), .default_target = *dispatch_panic});
            resolved_cleanup_blocks.insert(shape.completion);
            co_return index;
        }

        /** Selects the canonical implementation and binds one original object to its shared storage view. */
        auto async_cleanup_wrapper(block_index source, local_index original, vmir2::slot_state const& state,
            std::uint8_t operation, std::optional< type_symbol > destructor) -> co_type< block_index >
        {
            type_symbol const type = generator.state.locals.at(original).type;
            std::optional< vmir2::struct_init_selector > selector;
            std::optional< type_symbol > enclosing_type;
            std::optional< local_index > enclosing;
            if (state.nontrivial_dtor.has_value() && state.nontrivial_dtor->subobject_context.has_value())
            {
                selector = state.nontrivial_dtor->subobject_context->selector;
                enclosing = state.nontrivial_dtor->subobject_context->enclosing_object;
                enclosing_type = generator.state.locals.at(*enclosing).type;
            }
            async_cleanup_key key{operation, type, std::move(destructor), state.construction_pending, selector, enclosing_type};
            typename decltype(async_cleanup_indices)::const_iterator known = async_cleanup_indices.find(key);
            std::size_t index = known != async_cleanup_indices.end() ? known->second :
                co_await create_async_cleanup_shape(source, std::move(key), type, state, operation, enclosing_type);
            typename decltype(generated_cleanup_owners)::const_iterator owner = generated_cleanup_owners.find(original);
            if (owner != generated_cleanup_owners.end()) cleanup_dependencies.at(owner->second).insert(index);
            async_cleanup_shape& shape = *async_cleanup_shapes.at(index);
            std::size_t destination = async_cleanup_destination(source, shape, original);
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            block_index block = dynamic_block(source, cleanup_dispatch->control_state(), "coroutine_cleanup_action_binding");
            block_index entry = block;
            bind_async_cleanup_cell(block, original, shape.address, state);
            if (enclosing.has_value())
            {
                bool storage_only = !is_ref(*enclosing_type);
                vmir2::slot_state borrowed{.stage = storage_only ? vmir2::slot_stage::dead : vmir2::slot_stage::full,
                    .storage_valid = true, .construction_pending = storage_only};
                bind_async_cleanup_cell(block, *enclosing, *shape.enclosing_address, borrowed);
            }
            generator.emit(block, vmir2::load_const_int{.target = generator.get_local_index(shape.identity),
                .value = std::to_string(static_cast< std::uint64_t >(original))});
            generator.emit(block, vmir2::load_const_int{.target = generator.get_local_index(shape.continuation), .value = std::to_string(destination)});
            generator.emit(block, vmir2::lifetime_transfer{.state = async_cleanup_state(shape)});
            generator.generate_jump(block, shape.entry);
            resolved_cleanup_blocks.insert(block);
            co_return entry;
        }

        /** Bounds simultaneously active shared actions and rejects mandatory owned cleanup cycles. */
        std::size_t async_cleanup_depth() const
        {
            std::vector< std::size_t > incoming(cleanup_dependencies.size());
            std::vector< std::size_t > depth(cleanup_dependencies.size(), 1);
            for (std::set< std::size_t > const& successors : cleanup_dependencies)
                for (std::size_t successor : successors) ++incoming.at(successor);
            std::vector< std::size_t > pending;
            for (std::size_t index = 0; index < incoming.size(); ++index)
                if (incoming[index] == 0) pending.push_back(index);
            std::size_t maximum = 0;
            std::size_t visited = 0;
            while (!pending.empty())
            {
                std::size_t current = pending.back();
                pending.pop_back();
                ++visited;
                maximum = std::max(maximum, depth[current]);
                for (std::size_t successor : cleanup_dependencies[current])
                {
                    depth[successor] = std::max(depth[successor], depth[current] + 1);
                    if (--incoming[successor] == 0) pending.push_back(successor);
                }
            }
            if (visited != cleanup_dependencies.size())
                throw semantic_compilation_error("Coroutine cleanup recursively owns an operation requiring the same asynchronous cleanup");
            return maximum;
        }

        /** Tests empty generated control edges once per immutable source subtree. */
        bool owns_departing_candidate(vmir2::state_map::snapshot const& source)
        {
            if (!source) return false;
            typename decltype(owned_summaries)::const_iterator known = owned_summaries.find(source.get());
            if (known != owned_summaries.end()) return known->second.second;
            bool result;
            if (source->value)
            {
                local_index local = source->value->first;
                vmir2::slot_state const& state = source->value->second;
                result = local != generator.state.coroutine->frame && local != generator.state.coroutine->controller &&
                    !state.persistent_storage && (state.alive() || state.storage_valid);
            }
            else result = owns_departing_candidate(source->left) || owns_departing_candidate(source->right);
            owned_summaries.emplace(source.get(), std::pair{source, result});
            return result;
        }

        /** Adds already initialized shared scalar controls while preserving the source's ownership tree. */
        vmir2::state_map dynamic_state(vmir2::state_map state)
        {
            for (value_index global : cleanup_dispatch->globals()) state[generator.get_local_index(global)] = vmir2::slot_state{
                .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true};
            return state;
        }

        /** Creates an empty block whose active lifetime snapshot is explicitly selected. */
        block_index dynamic_block(block_index source, vmir2::state_map state, std::string name)
        {
            generator.state.blocks.emplace_back();
            block_index block(generator.state.blocks.size() - 1);
            typename generator_type::codegen_block& selected = generator.state.blocks.back();
            selected.dbg_name = std::move(name);
            selected.has_incoming_flow = source == block_index(0) || generator.state.blocks.at(source).has_incoming_flow;
            selected.entry_state = state;
            selected.current_state = std::move(state);
            selected.catcher = generator.state.catcher;
            selected.catch_exception = generator.state.catch_exception;
            selected.catch_region = generator.state.catch_region;
            selected.coroutine_handler_root = generator.state.coroutine_handler_root;
            selected.coroutine_handler_exception = generator.state.coroutine_handler_exception;
            return block;
        }

        /** Produces the nullable pointer form of one lexical handler binding. */
        value_index dynamic_handler_address(block_index block, std::optional< value_index > value, bool exception)
        {
            if (value.has_value()) return generator.coroutine_handler_address(block, *value);
            type_symbol target = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = exception ? "EXCEPTION_PTR" : "EXCEPTION_HANDLER"};
            value_index result = generator.create_local_value(ptrref_type{.target = std::move(target), .ptr_class = pointer_class::instance,
                .qual = exception ? qualifier::constant : qualifier::mut});
            generator.emit(block, vmir2::load_const_zero{.target = generator.get_local_index(result)});
            return result;
        }

        /** Creates the common runtime cursor and its owned exception-retargeting boundary. */
        auto begin_dynamic_dispatch(block_index source) -> co_type< void >
        {
            if (cleanup_dispatch.has_value()) co_return;
            cleanup_program.emplace(lifetime_description);
            cleanup_dispatch.emplace(generator, *cleanup_program);
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            vmir2::state_map controls = cleanup_dispatch->control_state();
            dispatch_entry = dynamic_block(source, controls, "coroutine_cleanup_dispatch");
            dispatch_completion = dynamic_block(source, controls, "coroutine_cleanup_dispatch_complete");
            dispatch_failure = dynamic_block(source, controls, "coroutine_cleanup_dispatch_exception");
            dispatch_panic = dynamic_block(source, controls, "coroutine_cleanup_dispatch_terminate");
            generator.set_terminator(*dispatch_panic, vmir2::panic{.message = "Exception during coroutine exception cleanup"});
            block_index retry = dynamic_block(source, controls, "coroutine_cleanup_dispatch_retry");
            for (block_index resolved : {*dispatch_entry, *dispatch_completion, *dispatch_failure, *dispatch_panic, retry})
                resolved_cleanup_blocks.insert(resolved);
            block_index failure = *dispatch_failure;
            value_index active = cleanup_dispatch->unwinding(failure);
            generator.generate_branch(active, failure, *dispatch_panic, retry);
            co_await cleanup_dispatch->retry_exception(retry);
            generator.generate_jump(retry, *dispatch_entry);
            block_index select = *dispatch_entry;
            value_index action = co_await cleanup_dispatch->next(select);
            dispatch_selection = select;
            resolved_cleanup_blocks.insert(select);
            // The final table is published after nested cleanup has added every action.
            generator.set_terminator(select, vmir2::tablebranch{.index = generator.get_local_index(action), .default_target = *dispatch_panic});
            block_index complete = *dispatch_completion;
            value_index destination = cleanup_dispatch->destination(complete);
            generator.set_terminator(complete, vmir2::tablebranch{.index = generator.get_local_index(destination), .default_target = *dispatch_panic});
            co_return;
        }

        /** Releases one context and restores the exact logical state at its selected destination. */
        auto dynamic_destination_for(block_index source, block_index target, std::optional< local_index > exception_target) -> co_type< std::size_t >
        {
            std::pair key{target, exception_target};
            typename decltype(dynamic_destination_indices)::const_iterator existing = dynamic_destination_indices.find(key);
            if (existing != dynamic_destination_indices.end()) co_return existing->second;
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            block_index block = dynamic_block(source, cleanup_dispatch->control_state(), "coroutine_cleanup_dispatch_exit");
            block_index entry = block;
            std::optional< value_index > captured;
            type_symbol exception_type = generator.current_type(block, cleanup_dispatch->exception);
            if (exception_target.has_value())
            {
                value_index reference = cleanup_dispatch->owned_exception(block);
                captured = co_await generator.co_gen_call_ctor(block, exception_type, codegen_invocation_args{.named = {{"OTHER", reference}}});
            }
            co_await cleanup_dispatch->leave(block);
            vmir2::state_map destination = generator.state.blocks.at(target).entry_state;
            if (destination.contains(generator.get_local_index(cleanup_dispatch->exception))) cleanup_dispatch->restore_context(block);
            if (exception_target.has_value())
            {
                vmir2::slot_state borrowed{.stage = vmir2::slot_stage::full, .storage_valid = true, .is_projection = true, .persistent_storage = true, .retained_storage = true};
                generator.emit(block, vmir2::lifetime_import{.slot = *exception_target, .state = borrowed});
                value_index output = generator.create_reference(block, value_for(*exception_target), make_mref(exception_type));
                value_index input = generator.create_reference(block, *captured, make_tref(exception_type));
                co_await generator.co_generate_binary(block, ":=", output, input);
                generator.emit(block, vmir2::destroy{.of = generator.get_local_index(*captured)});
            }
            generator.emit(block, vmir2::lifetime_transfer{.state = dynamic_state(destination)});
            generator.generate_jump(block, target);
            std::size_t index = dynamic_destinations.size();
            resolved_cleanup_blocks.insert(block);
            dynamic_destinations.push_back({.block = entry, .target = target, .exception_target = exception_target});
            dynamic_destination_indices.emplace(std::move(key), index);
            co_return index;
        }

        /** Parks one source ownership snapshot and enters its bounded runtime cleanup route. */
        auto dynamic_successor(block_index source_block, vmir2::state_map const& source, block_index target,
            vmir2::lifetime_transition_kind kind, bool exceptional_cleanup) -> co_type< block_index >
        {
            co_await begin_dynamic_dispatch(source_block);
            typename generator_type::codegen_block const& origin = generator.state.blocks.at(source_block);
            std::optional< vmir2::exception_catcher > catcher = origin.catcher;
            std::optional< value_index > root = origin.coroutine_handler_root;
            std::optional< value_index > active_exception = origin.coroutine_handler_exception;
            std::size_t route = cleanup_program->append(source, generator.state.blocks.at(target).entry_state, kind);
            std::size_t destination = co_await dynamic_destination_for(source_block, target, std::nullopt);
            std::size_t failure_target = 0;
            std::size_t failure_destination = destination;
            if (catcher.has_value() && !exceptional_cleanup)
            {
                std::size_t failed = cleanup_program->append(source, generator.state.blocks.at(catcher->handler).entry_state,
                    vmir2::lifetime_transition_kind::control_flow);
                std::optional< std::size_t > root_index = cleanup_program->transitions().at(failed).target;
                failure_target = root_index.has_value() ? *root_index + 1 : 0;
                failure_destination = co_await dynamic_destination_for(source_block, catcher->handler, catcher->exception);
            }
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = root;
            generator.state.coroutine_handler_exception = active_exception;
            block_index entry = dynamic_block(source_block, dynamic_state(source), "coroutine_cleanup_dispatch_enter");
            block_index block = entry;
            value_index handler_pointer = dynamic_handler_address(block, root, false);
            value_index exception_pointer = dynamic_handler_address(block, active_exception, true);
            std::uint64_t count = source.snapshot_root() ? source.snapshot_root()->maximum_key + 1 : 0;
            std::optional< value_index > captured_exception;
            if (exceptional_cleanup && catcher.has_value()) captured_exception = value_for(catcher->exception);
            value_index propagating = dynamic_handler_address(block, captured_exception, true);
            co_await cleanup_dispatch->enter(block, cleanup_program->transitions().at(route), destination, failure_target,
                failure_destination, handler_pointer, exception_pointer, propagating, exceptional_cleanup || !catcher.has_value(), count);
            generator.generate_jump(block, *dispatch_entry);
            resolved_cleanup_blocks.insert(block);
            co_return entry;
        }

        /** Borrows a retained operand without changing its physical ownership metadata. */
        void import_cleanup_operand(block_index block, local_index local, bool storage_only)
        {
            if (local == generator.state.coroutine->frame || local == generator.state.coroutine->controller) return;
            vmir2::slot_state state{
                .stage = storage_only ? vmir2::slot_stage::dead : vmir2::slot_stage::full,
                .storage_valid = true,
                .construction_pending = storage_only,
                .is_projection = true,
                .persistent_storage = true,
                .retained_storage = true,
            };
            generator.state.coroutine->cleanup_retained.insert(local);
            lifetime_description.coroutine->cleanup_retained.insert(local);
            generator.emit(block, vmir2::lifetime_import{.slot = local, .state = std::move(state)});
        }

        /** Retains address operands needed by lifecycle restoration and registered destruction. */
        void import_cleanup_dependencies(block_index block, local_index local, vmir2::slot_state const& selected)
        {
            std::set< local_index > arguments;
            if (selected.nontrivial_dtor.has_value())
            {
                for (local_index argument : selected.nontrivial_dtor->args.positional) arguments.insert(argument);
                for (std::pair< std::string const, local_index > const& argument : selected.nontrivial_dtor->args.named) arguments.insert(argument.second);
            }
            for (local_index argument : arguments)
                if (argument != local) import_cleanup_operand(block, argument, false);
            auto address = [&](local_index operand)
            {
                if (operand == local || arguments.contains(operand)) return;
                import_cleanup_operand(block, operand, !is_ref(generator.state.locals.at(operand).type));
            };
            if (selected.nontrivial_dtor.has_value() && selected.nontrivial_dtor->subobject_context.has_value())
                address(selected.nontrivial_dtor->subobject_context->enclosing_object);
            if (selected.lifecycle_phase.has_value())
            {
                address(selected.lifecycle_phase->on_value);
                if (selected.lifecycle_phase->subobject_context.has_value()) address(selected.lifecycle_phase->subobject_context->enclosing_object);
            }
        }

        /** Removes lexical parent ownership while retaining the selected physical object's state. */
        static vmir2::slot_state detached_cleanup_state(vmir2::slot_state state)
        {
            state.delegate_of.reset();
            state.struct_delegate_selector.reset();
            state.array_delegate_of_initializer.reset();
            state.storage_origin.reset();
            state.struct_delegates.reset();
            state.array_delegates.reset();
            state.array_element_delegates = {};
            state.destroy_delegate = false;
            state.is_projection = false;
            state.persistent_storage = false;
            state.retained_storage = true;
            state.owns_subobject_lifetime = true;
            return state;
        }

        /** Publishes a handler's surviving parent before subsequent cleanup can suspend. */
        void retire_dynamic_handler(block_index block, local_index local)
        {
            typename std::map< local_index, typename generator_type::coroutine_handler_scope >::const_iterator scope =
                generator.state.coroutine_handler_scopes.find(local);
            if (scope == generator.state.coroutine_handler_scopes.end()) return;
            if (scope->second.parent.root.has_value()) import_cleanup_operand(block, generator.get_local_index(*scope->second.parent.root), false);
            if (scope->second.parent.exception.has_value()) import_cleanup_operand(block, generator.get_local_index(*scope->second.parent.exception), false);
            cleanup_dispatch->handlers(block, dynamic_handler_address(block, scope->second.parent.root, false),
                dynamic_handler_address(block, scope->second.parent.exception, true));
        }

        /** Generates one typed action body shared by every compatible immutable source version. */
        auto dynamic_action_body(block_index source, local_index local, vmir2::slot_state const& original, std::uint8_t operation) -> co_type< block_index >
        {
            vmir2::slot_state selected = detached_cleanup_state(original);
            if (operation == 5) selected.destruction_started = true;
            dynamic_action_key key{operation, local, selected.stage, selected.construction_pending,
                selected.destruction_started, selected.nontrivial_dtor.has_value() ? &*selected.nontrivial_dtor : nullptr,
                selected.lifecycle_phase};
            typename decltype(dynamic_action_bodies)::const_iterator previous = dynamic_action_bodies.find(key);
            if (previous != dynamic_action_bodies.end()) co_return previous->second;
            std::optional< type_symbol > asynchronous_destructor;
            if (operation == 0 && !selected.destruction_started)
            {
                if (selected.nontrivial_dtor.has_value()) asynchronous_destructor = selected.nontrivial_dtor->func;
                else
                {
                    std::map< type_symbol, type_symbol >::const_iterator selected_destructor = generator.state.non_trivial_dtors.find(generator.state.locals.at(local).type);
                    if (selected_destructor != generator.state.non_trivial_dtors.end()) asynchronous_destructor = selected_destructor->second;
                }
            }
            else if (operation == 1)
            {
                type_symbol const& element = generator.state.locals.at(local).type.template get_as< array_initializer_type >().element_type;
                std::map< type_symbol, type_symbol >::const_iterator selected_destructor = generator.state.non_trivial_dtors.find(element);
                if (selected_destructor != generator.state.non_trivial_dtors.end()) asynchronous_destructor = selected_destructor->second;
            }
            if (asynchronous_destructor.has_value() && is_coroutine_destructor(*asynchronous_destructor))
            {
                block_index entry = co_await async_cleanup_wrapper(source, local, original, operation, asynchronous_destructor);
                dynamic_action_bodies.emplace(std::move(key), entry);
                co_return entry;
            }
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root = cleanup_dispatch->handler_root;
            generator.state.coroutine_handler_exception = cleanup_dispatch->handler_exception;
            block_index entry = dynamic_block(source, cleanup_dispatch->control_state(), "coroutine_cleanup_action");
            block_index block = entry;
            dynamic_action_bodies.emplace(std::move(key), entry);
            if (operation == 6)
            {
                generator.emit(block, vmir2::lifetime_import{.slot = local});
                generator.emit(block, vmir2::co_cleanup_mark{.slot = local, .status = 2});
                generator.generate_jump(block, *dispatch_entry);
                co_return entry;
            }
            if (operation == 4)
            {
                generator.set_terminator(block, vmir2::panic{.message = "Synchronous cleanup selected an async-only object"});
                co_return entry;
            }
            import_cleanup_dependencies(block, local, selected);
            generator.emit(block, vmir2::lifetime_import{.slot = local, .state = selected});
            vmir2::state_map failure_state = generator.state.blocks.at(block).current_state;
            vmir2::slot_state borrowed = selected;
            borrowed.persistent_storage = true;
            borrowed.is_projection = true;
            borrowed.owns_subobject_lifetime = false;
            failure_state[local] = std::move(borrowed);
            block_index failure = dynamic_block(block, failure_state, "coroutine_cleanup_action_exception");
            generator.emit(failure, vmir2::lifetime_transfer{.state = cleanup_dispatch->control_state()});
            generator.generate_jump(failure, *dispatch_failure);
            resolved_cleanup_blocks.insert(failure);
            vmir2::exception_catcher catcher{.exception = generator.get_local_index(cleanup_dispatch->exception), .handler = failure};
            generator.state.blocks.at(block).catcher = catcher;
            generator.state.catcher = catcher;
            generator.state.coroutine_shielded = true;
            switch (operation)
            {
            case 0: co_await destroy_object(block, local); break;
            case 1: co_await destroy_array_prefix(block, local); break;
            case 2: generator.emit(block, vmir2::initguard_abort{.lock = local}); break;
            case 3: generator.emit(block, vmir2::struct_phase_leave{.saved_phase = local}); break;
            case 5: generator.emit(block, vmir2::end_lifetime{.of = local, .initialization_rollback = true}); break;
            case 8: generator.emit(block, vmir2::struct_phase_leave{.saved_phase = local, .completed = true}); break;
            default: throw compiler_bug("Invalid coroutine cleanup action opcode");
            }
            retire_dynamic_handler(block, local);
            generator.generate_jump(block, *dispatch_entry);
            co_return entry;
        }

        /** Discovers each new source leaf once and emits its finite set of typed actions. */
        auto emit_dynamic_actions(block_index source) -> co_type< void >
        {
            if (!cleanup_program.has_value()) co_return;
            while (scanned_cleanup_nodes < cleanup_program->source_leaves().size())
            {
                std::size_t version = cleanup_program->source_leaves().at(scanned_cleanup_nodes++);
                vmir2::state_map::value_type const entry = *cleanup_program->graph().nodes().at(version).source->value;
                if (entry.first == generator.state.coroutine->frame || entry.first == generator.state.coroutine->controller) continue;
                if (!entry.second.persistent_storage && (entry.second.alive() || entry.second.storage_valid))
                {
                    generator.state.coroutine->cleanup_retained.insert(entry.first);
                    lifetime_description.coroutine->cleanup_retained.insert(entry.first);
                }
                std::uint64_t mask = cleanup_program->action_mask(version);
                for (std::uint8_t operation = 0; operation != 9; ++operation)
                {
                    bool selected = operation < 6 ? (mask & (std::uint64_t{1} << operation)) != 0 :
                        operation == 6 ? (mask & vmir2::lifetime_cleanup_program::release_alias) != 0 :
                        operation == 8 && (mask & vmir2::lifetime_cleanup_program::publication_action) != 0;
                    if (operation == 0 && (mask & vmir2::lifetime_cleanup_program::delegate_action) != 0) selected = true;
                    if (!selected) continue;
                    std::size_t code = (version + 1) * 16 + operation + 1;
                    dynamic_actions.emplace(code, co_await dynamic_action_body(source, entry.first, entry.second, operation));
                }
            }
        }

        /** Charges generated cleanup work without interrupting a partially emitted action. */
        void charge_cleanup(std::uint64_t amount)
        {
            if (amount > std::numeric_limits< std::uint64_t >::max() - cleanup_work_charged)
                throw compiler_bug("Coroutine cleanup work accounting overflow");
            cleanup_work_charged += amount;
            cleanup_work_remaining = amount >= cleanup_work_remaining ? 0 : cleanup_work_remaining - amount;
        }

        /** Establishes the fixed refinement allowance before generating any cleanup blocks. */
        void initialize_cleanup_budget()
        {
            std::uint64_t input = generator.state.locals.size() + generator.state.blocks.size();
            for (typename generator_type::codegen_block const& block : generator.state.blocks)
                input += block.instructions.size() + 1;
            std::uint64_t logarithm = std::max< std::uint64_t >(1, std::bit_width(input));
            constexpr std::uint64_t coefficient = 8;
            if (input > std::numeric_limits< std::uint64_t >::max() / coefficient / logarithm)
                throw compiler_bug("Coroutine cleanup work limit overflow");
            cleanup_work_remaining = input * logarithm * coefficient;
            generator.state.coroutine->cleanup_statistics.input_size = input;
            generator.state.coroutine->cleanup_statistics.work_limit = cleanup_work_remaining;
        }

        /** Publishes final dispatch tables after generated action cleanup reaches a fixed point. */
        void finalize_dynamic_dispatch()
        {
            if (!cleanup_dispatch.has_value()) return;
            vmir2::tablebranch& actions = generator.state.blocks.at(*dispatch_selection).terminator->template get_as< vmir2::tablebranch >();
            std::size_t extent = dynamic_actions.empty() ? 1 : dynamic_actions.rbegin()->first + 1;
            actions.targets.assign(extent, *dispatch_panic);
            actions.targets.front() = *dispatch_completion;
            for (std::pair< std::size_t const, block_index > const& action : dynamic_actions) actions.targets.at(action.first) = action.second;
            vmir2::tablebranch& exits = generator.state.blocks.at(*dispatch_completion).terminator->template get_as< vmir2::tablebranch >();
            exits.targets.reserve(dynamic_destinations.size());
            for (dynamic_destination const& destination : dynamic_destinations) exits.targets.push_back(destination.block);
            vmir2::coroutine_cleanup_statistics& statistics = generator.state.coroutine->cleanup_statistics;
            statistics.fallback_routes = cleanup_program->transitions().size();
            statistics.action_bodies = dynamic_action_bodies.size() + async_cleanup_shapes.size();
            for (std::unique_ptr< async_cleanup_shape > const& shape : async_cleanup_shapes)
            {
                vmir2::tablebranch& returns = generator.state.blocks.at(shape->completion).terminator->template get_as< vmir2::tablebranch >();
                returns.targets = shape->destinations;
            }
            statistics.program_nodes = cleanup_program->graph().nodes().size();
        }

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
                charge_cleanup(1);
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
        auto cleanup_sequence(vmir2::lifetime_transition_graph::sequence sequence, block_index source_block,
            vmir2::state_map const& source, block_index destination, block_index next,
            std::optional< vmir2::exception_catcher > catcher) -> co_type< block_index >
        {
            if (sequence == 0) co_return next;
            if (cleanup_work_remaining == 0) throw cleanup_budget_exhausted{};
            charge_cleanup(1);
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
                next = co_await cleanup_sequence(sequence_node.second, source_block, source, destination, next, catcher);
                next = co_await cleanup_sequence(sequence_node.first, source_block, source, destination, next, catcher);
                cleanup_sequences.emplace(std::move(sequence_identity), next);
                co_return next;
            }
            if (sequence_node.redundant_end) co_return next;
            charge_cleanup(64);
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
            std::size_t first_block = generator.state.blocks.size();
            std::size_t first_local = generator.state.locals.size();
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
            charge_cleanup(generator.state.locals.size() - first_local);
            for (std::size_t emitted = first_block; emitted < generator.state.blocks.size(); ++emitted)
                charge_cleanup(generator.state.blocks[emitted].instructions.size() + 1);
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
            if (cleanup_dispatch.has_value() && !owns_departing_candidate(source.snapshot_root())) co_return destination;
            if (!inline_cleanup) co_return co_await dynamic_successor(source_block, source, destination, kind, exceptional_cleanup);
            std::uint64_t previous_work = cleanup_work_remaining;
            std::optional< vmir2::lifetime_transition_graph::plan > planned = transition_graph->select_bounded(source,
                generator.state.blocks.at(destination).entry_state, kind, cleanup_work_remaining);
            cleanup_work_charged += previous_work - cleanup_work_remaining;
            if (!planned.has_value()) throw cleanup_budget_exhausted{};
            vmir2::lifetime_transition_graph::plan actions = *planned;
            if (actions.empty()) co_return destination;
            if (cleanup_work_remaining == 0) throw cleanup_budget_exhausted{};
            std::optional< vmir2::exception_catcher > catcher = generator.state.blocks.at(source_block).catcher;
            if (exceptional_cleanup && catcher.has_value())
            {
                catcher->handler = cleanup_termination(source_block, *catcher);
                catcher->terminate_before_capture = true;
            }
            block_index next = co_await cleanup_sequence(actions.publication, source_block, source, destination, destination, catcher);
            next = co_await cleanup_sequence(actions.ordinary, source_block, source, destination, next, catcher);
            co_return co_await cleanup_sequence(actions.delegates, source_block, source, destination, next, catcher);
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

        /** Emits explicit destruction and CFG cleanup using the selected expansion policy. */
        auto expand_routes() -> co_type< void >
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
            initialize_cleanup_budget();
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
            for (std::size_t ordinal = 0;; ++ordinal)
            {
                if (ordinal == generator.state.blocks.size())
                {
                    co_await refresh_lifetimes();
                    co_await emit_dynamic_actions(block_index(0));
                    if (ordinal == generator.state.blocks.size()) break;
                }
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

            finalize_dynamic_dispatch();
            generator.state.coroutine->cleanup_statistics.work_used = cleanup_work_charged;
            std::set< local_index > retirement_slots;
            if (cleanup_program.has_value())
            {
                for (std::size_t leaf : cleanup_program->source_leaves())
                {
                    vmir2::state_map::value_type const& entry = *cleanup_program->graph().nodes().at(leaf).source->value;
                    if (!entry.second.persistent_storage) retirement_slots.insert(entry.first);
                }
            }
            std::size_t depth = async_cleanup_depth();
            if (depth > (std::numeric_limits< std::size_t >::max() - 3) / 2)
                throw compiler_bug("Coroutine cleanup context extent overflow");
            // Each selected action can park its caller cursor and one failure-drain cursor.
            // The outer route and synchronous capture/return transitions require three more.
            std::size_t context_capacity = depth * 2 + 3;
            coroutine_cleanup_storage_lowering< CoroutineBaseType > storage(generator,
                cleanup_dispatch.has_value() ? cleanup_dispatch->globals() : std::vector< value_index >{},
                cleanup_dispatch.has_value() ? std::optional< static_snapshot_ref >(cleanup_dispatch->freeze()) : std::nullopt,
                forwarded_cleanup_identities, context_capacity, std::move(retirement_slots));
            co_await storage.lower();
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
                for (vmir2::vm_instruction& instruction : block.instructions)
                {
                    if (instruction.template type_is< vmir2::lifetime_transfer >())
                    {
                        vmir2::state_map& state = instruction.template get_as< vmir2::lifetime_transfer >().state;
                        state = vmir2::state_map::from_snapshot(destructor_markers.apply(state.snapshot_root(), remove_coroutine_destructor));
                    }
                    else if (instruction.template type_is< vmir2::lifetime_import >())
                    {
                        vmir2::slot_state& state = instruction.template get_as< vmir2::lifetime_import >().state;
                        if (state.nontrivial_dtor.has_value() && is_coroutine_destructor(state.nontrivial_dtor->func)) state.nontrivial_dtor.reset();
                    }
                }
                for (vmir2::state_map* state : {&block.entry_state, &block.current_state})
                {
                    *state = vmir2::state_map::from_snapshot(destructor_markers.apply(state->snapshot_root(), remove_coroutine_destructor));
                }
            }
            co_return;
        }

      public:
        /** Binds cleanup generation to the coroutine's existing typed VMIR builder. */
        explicit coroutine_cleanup_lowering(generator_type& generator) : generator(generator)
        {
        }

        /** Publishes a completed expansion or regenerates shared actions after budget exhaustion. */
        auto expand() -> co_type< void >
        {
            typename generator_type::codegen_state source = generator.state;
            std::uint64_t refinement_work = 0;
            {
                coroutine_cleanup_lowering candidate(generator);
                try
                {
                    co_await candidate.expand_routes();
                    co_return;
                }
                catch (cleanup_budget_exhausted const&)
                {
                    refinement_work = candidate.cleanup_work_charged;
                }
            }
            generator.state = std::move(source);
            coroutine_cleanup_lowering baseline(generator);
            baseline.inline_cleanup = false;
            co_await baseline.expand_routes();
            generator.state.coroutine->cleanup_statistics.work_used = refinement_work;
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
