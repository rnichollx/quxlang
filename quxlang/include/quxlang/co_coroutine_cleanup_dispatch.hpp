// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_CO_COROUTINE_CLEANUP_DISPATCH_HEADER_GUARD
#define QUXLANG_CO_COROUTINE_CLEANUP_DISPATCH_HEADER_GUARD

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/vmir2/lifetime_cleanup_program.hpp>
#include <limits>
#include <optional>
#include <vector>

namespace quxlang
{
    /** Emits the shared controls of an allocation-free nested cleanup dispatcher. */
    template < codegen_co_base CoroutineBaseType >
    class coroutine_cleanup_dispatch
    {
        using generator_type = co_vmir_generator2< CoroutineBaseType >;
        using block_index = vmir2::block_index;
        using local_index = vmir2::local_index;
        template < typename T > using co_type = typename CoroutineBaseType::template cosubroutine< T >;
        generator_type& generator;
        vmir2::lifetime_cleanup_program const& program;
        type_symbol word_type = int_type{.bits = 64, .has_sign = false};
        type_symbol byte_type = int_type{.bits = 8, .has_sign = false};
        type_symbol exception_type = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "EXCEPTION_PTR"};
        type_symbol handler_type = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "EXCEPTION_HANDLER"};
        type_symbol context_type = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "COROUTINE_CLEANUP_CONTEXT"};
        type_symbol cursor_type = array_type{.element_type = word_type, .element_count = expression_numeric_literal{.value = "8"}};
        type_symbol context_pointer = ptrref_type{.target = context_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        type_symbol exception_pointer = ptrref_type{.target = exception_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        type_symbol handler_pointer = ptrref_type{.target = handler_type, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        type_symbol borrowed_exception_pointer = ptrref_type{.target = exception_type, .ptr_class = pointer_class::instance, .qual = qualifier::constant};

        /** Produces a consumable pointer to one compiler-owned storage area. */
        value_index storage(block_index block, vmir2::coroutine_cleanup_area area, type_symbol const& element, qualifier access)
        {
            value_index result = generator.create_local_value(ptrref_type{.target = element, .ptr_class = pointer_class::array, .qual = access});
            generator.emit(block, vmir2::co_cleanup_storage{.area = area, .result = generator.get_local_index(result)});
            return result;
        }

        /** Reads a scalar while preserving its owned local. */
        value_index read(block_index block, value_index local)
        {
            type_symbol type = generator.current_type(block, local);
            value_index source = generator.create_reference(block, local, make_cref(type));
            value_index result = generator.create_local_value(type);
            generator.emit(block, vmir2::load_from_ref{.from_reference = generator.get_local_index(source), .to_value = generator.get_local_index(result)});
            return result;
        }

        /** Replaces a scalar control value using ordinary typed assignment. */
        void write(block_index block, value_index target, value_index value)
        {
            value_index reference = generator.create_reference(block, target, make_mref(generator.current_type(block, target)));
            generator.emit(block, vmir2::store_to_ref{.from_value = generator.get_local_index(value), .to_reference = generator.get_local_index(reference)});
        }

        /** Projects one field of the current lexical cleanup context. */
        value_index field(block_index block, std::string const& name, type_symbol const& type)
        {
            value_index pointer = read(block, current);
            value_index reference = generator.create_local_value(make_mref(context_type));
            generator.emit(block, vmir2::dereference_pointer{.from_pointer = generator.get_local_index(pointer), .to_reference = generator.get_local_index(reference)});
            value_index result = generator.create_local_value(make_mref(type));
            generator.emit(block, vmir2::access_field{.base_index = generator.get_local_index(reference), .store_index = generator.get_local_index(result), .field_name = name});
            return result;
        }

        /** Loads one context field into a scalar control binding. */
        void load_field(block_index block, std::string const& name, value_index target)
        {
            type_symbol type = generator.current_type(block, target);
            value_index source = field(block, name, type);
            value_index value = generator.create_local_value(type);
            generator.emit(block, vmir2::load_from_ref{.from_reference = generator.get_local_index(source), .to_value = generator.get_local_index(value)});
            write(block, target, value);
        }

      public:
        value_index depth;
        value_index current;
        value_index saved_exception;
        value_index exception;
        value_index handler_root;
        value_index handler_exception;

        /** Creates scalar controls whose physical construction is deferred until storage finalization. */
        coroutine_cleanup_dispatch(generator_type& selected, vmir2::lifetime_cleanup_program const& selected_program)
            : generator(selected), program(selected_program)
        {
            depth = generator.create_local_value(word_type);
            current = generator.create_local_value(context_pointer);
            saved_exception = generator.create_local_value(exception_pointer);
            exception = generator.create_local_value(exception_type);
            handler_root = generator.create_local_value(handler_pointer);
            handler_exception = generator.create_local_value(borrowed_exception_pointer);
            generator.state.coroutine->cleanup_aliases.emplace(generator.get_local_index(exception), generator.get_local_index(saved_exception));
        }

        /** Lists the scalar controls constructed once before the coroutine body. */
        std::vector< value_index > globals() const { return {depth, current, saved_exception, handler_root, handler_exception}; }

        /** Retains only execution parameters, shared controls, and the current borrowed capture object. */
        vmir2::state_map control_state() const
        {
            vmir2::state_map result;
            result[generator.state.coroutine->frame] = vmir2::slot_state{.stage = vmir2::slot_stage::full, .storage_valid = true};
            result[generator.state.coroutine->controller] = vmir2::slot_state{.stage = vmir2::slot_stage::full, .storage_valid = true};
            for (value_index value : globals()) result[generator.get_local_index(value)] = vmir2::slot_state{
                .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true};
            result[generator.get_local_index(exception)] = vmir2::slot_state{
                .stage = vmir2::slot_stage::full, .storage_valid = true, .is_projection = true, .persistent_storage = true, .retained_storage = true};
            return result;
        }

        /** Rebinds dynamic exception and handler views after entering or restoring a context. */
        void restore_context(block_index block)
        {
            generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(exception)});
            value_index reference = field(block, "pending", exception_type);
            value_index pointer = generator.create_local_value(exception_pointer);
            generator.emit(block, vmir2::cast_ptrref{.source_index = generator.get_local_index(reference), .target_index = generator.get_local_index(pointer)});
            write(block, saved_exception, pointer);
            generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(exception), .state = control_state().at(generator.get_local_index(exception))});
            load_field(block, "handler", handler_root);
            load_field(block, "handler_exception", handler_exception);
        }

        /** Starts a nested cursor without destroying any parked source lifetime. */
        auto enter(block_index& block, vmir2::lifetime_cleanup_program::transition const& route, std::size_t destination,
            std::size_t failure_target, std::size_t failure_destination, value_index root, value_index active_exception,
            value_index propagating, bool unwinding, std::uint64_t local_count) -> co_type< void >
        {
            value_index stack = storage(block, vmir2::coroutine_cleanup_area::contexts, context_type, qualifier::mut);
            value_index guard = generator.create_local_value(bool_type{});
            generator.emit(block, vmir2::load_const_bool{.target = generator.get_local_index(guard), .value = unwinding});
            value_index selected = co_await generator.co_gen_call_functum(block,
                subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "COROUTINE_CLEANUP_PUSH"},
                codegen_invocation_args{.named = {
                    {"RECORD", stack}, {"COUNT", generator.create_reference(block, depth, make_mref(word_type))},
                    {"INPUT", generator.create_small_uint_value(block, route.source.has_value() ? *route.source + 1 : 0, word_type)},
                    {"OUTPUT", generator.create_small_uint_value(block, route.target.has_value() ? *route.target + 1 : 0, word_type)},
                    {"KIND", generator.create_small_uint_value(block, static_cast< std::uint64_t >(route.kind), word_type)},
                    {"DESTINATION", generator.create_small_uint_value(block, destination, word_type)},
                    {"VALUE", generator.create_small_uint_value(block, failure_target, word_type)},
                    {"OTHER", generator.create_small_uint_value(block, failure_destination, word_type)},
                    {"STATE", root}, {"EXCEPTION", active_exception}, {"ARG", propagating}, {"GUARD", guard},
                    {"SIZE", generator.create_small_uint_value(block, local_count, word_type)},
                }});
            write(block, current, selected);
            restore_context(block);
            generator.emit(block, vmir2::lifetime_transfer{.state = control_state()});
        }

        /** Returns the next statically typed action index from the active cursor. */
        auto next(block_index& block) -> co_type< value_index >
        {
            value_index nodes = storage(block, vmir2::coroutine_cleanup_area::program, word_type, qualifier::constant);
            value_index retired = storage(block, vmir2::coroutine_cleanup_area::retirement, byte_type, qualifier::constant);
            co_return co_await generator.co_gen_call_functum(block,
                subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "COROUTINE_CLEANUP_NEXT"},
                codegen_invocation_args{.named = {{"RECORD", nodes}, {"STATE", field(block, "cursor", cursor_type)}, {"GUARD", retired}}});
        }

        /** Reads a context flag without modifying its stored value. */
        value_index unwinding(block_index block)
        {
            value_index reference = field(block, "unwinding", bool_type{});
            value_index result = generator.create_local_value(bool_type{});
            generator.emit(block, vmir2::load_from_ref{.from_reference = generator.get_local_index(reference), .to_value = generator.get_local_index(result)});
            return result;
        }

        /** Transfers the pending capture into its owned context and restarts exceptional cleanup. */
        auto retry_exception(block_index& block) -> co_type< void >
        {
            value_index pointer = read(block, current);
            value_index reference = generator.create_local_value(make_mref(context_type));
            generator.emit(block, vmir2::dereference_pointer{.from_pointer = generator.get_local_index(pointer), .to_reference = generator.get_local_index(reference)});
            co_await generator.co_gen_call_functum(block,
                subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "COROUTINE_CLEANUP_RETRY"},
                codegen_invocation_args{.named = {{"ARG", reference}}});
        }

        /** Borrows the failure owned by the active context for terminal exception transfer. */
        value_index owned_exception(block_index block)
        {
            return field(block, "exception", exception_type);
        }

        /** Publishes the active handler remaining after one handler activation has ended. */
        void handlers(block_index block, value_index root, value_index active_exception)
        {
            write(block, handler_root, root);
            write(block, handler_exception, active_exception);
            value_index root_reference = field(block, "handler", handler_pointer);
            value_index root_value = read(block, handler_root);
            generator.emit(block, vmir2::store_to_ref{.from_value = generator.get_local_index(root_value), .to_reference = generator.get_local_index(root_reference)});
            value_index exception_reference = field(block, "handler_exception", borrowed_exception_pointer);
            value_index exception_value = read(block, handler_exception);
            generator.emit(block, vmir2::store_to_ref{.from_value = generator.get_local_index(exception_value), .to_reference = generator.get_local_index(exception_reference)});
        }

        /** Reads the selected continuation before releasing its context. */
        value_index destination(block_index block)
        {
            value_index reference = field(block, "destination", word_type);
            value_index result = generator.create_local_value(word_type);
            generator.emit(block, vmir2::load_from_ref{.from_reference = generator.get_local_index(reference), .to_value = generator.get_local_index(result)});
            return result;
        }

        /** Releases completed exception capture and restores the parent's physical context pointer. */
        auto leave(block_index& block) -> co_type< void >
        {
            generator.emit(block, vmir2::lifetime_import{.slot = generator.get_local_index(exception)});
            value_index stack = storage(block, vmir2::coroutine_cleanup_area::contexts, context_type, qualifier::mut);
            value_index selected = co_await generator.co_gen_call_functum(block,
                subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "COROUTINE_CLEANUP_POP"},
                codegen_invocation_args{.named = {{"RECORD", stack}, {"COUNT", generator.create_reference(block, depth, make_mref(word_type))}}});
            write(block, current, selected);
        }

        /** Freezes the final shared program after all nested cleanup routes have been discovered. */
        static_snapshot_ref freeze()
        {
            std::vector< vmir2::lifetime_cleanup_program::encoded_node > records = program.encode();
            std::size_t columns = static_cast< std::size_t >(vmir2::lifetime_cleanup_program::column::count);
            if (records.size() > std::numeric_limits< std::size_t >::max() / columns) throw compiler_bug("Coroutine cleanup program extent overflow");
            std::size_t words = records.size() * columns;
            antestatal_array values;
            values.elements.reserve(words);
            for (vmir2::lifetime_cleanup_program::encoded_node const& record : records)
            {
                for (std::uint64_t word : record)
                {
                    antestatal_primitive value;
                    value.value.reserve(8);
                    for (std::size_t byte = 0; byte < 8; ++byte) value.value.push_back(static_cast< std::byte >((word >> (byte * 8)) & 255));
                    values.elements.emplace_back(std::move(value));
                }
            }
            static_snapshot_ref table{.functanoid = generator.ctx, .name = "__COROUTINE_CLEANUP_NODES", .snapshot_id = generator.state.next_static_snapshot_id++};
            type_symbol type = array_type{.element_type = word_type, .element_count = expression_numeric_literal{.value = std::to_string(words)}};
            generator.state.static_snapshots.emplace(table, vmir2::localdata_entry{.type = std::move(type), .value = std::move(values)});
            return table;
        }
    };
}
#endif
