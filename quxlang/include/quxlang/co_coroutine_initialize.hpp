// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CO_COROUTINE_INITIALIZE_HEADER_GUARD
#define QUXLANG_CO_COROUTINE_INITIALIZE_HEADER_GUARD

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/data/coroutine_frame_info.hpp>
#include <quxlang/queries/type_is_async_only.hpp>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quxlang
{
    /** Constructs captured arguments with synchronous rollback and publishes the initial continuation. */
    template < codegen_co_base CoroutineBaseType >
    auto co_generate_coroutine_initialize(co_vmir_generator2< CoroutineBaseType >& generator,
        instanciation_reference const& generated, instanciation_reference const& coroutine,
        coroutine_frame_info const& frame_info) -> typename CoroutineBaseType::template cosubroutine< vmir2::functanoid_routine3 >
    {
        using generator_type = co_vmir_generator2< CoroutineBaseType >;
        using block_index = vmir2::block_index;
        using local_index = vmir2::local_index;

        /** Associates one logical parameter with its generated input and selected capture constructor. */
        struct parameter_capture
        {
            value_index source;
            type_symbol payload_type;
            std::string field_name;
            instanciation_reference constructor;
            std::optional< type_symbol > receiver_storage;
            bool initializes_storage = false;
        };

        vmir2::functanoid_routine3 const& logical = co_await rpnx::querygraph::request< vm_procedure3_query >(coroutine);
        co_await generator.co_generate_arg_info(generated);
        generator.generate_entry_block();
        block_index current(0);
        value_index frame = generator.local_value_direct_lookup(current, "THIS").value();

        std::map< local_index, value_index > parameter_values;
        for (std::size_t index = 1; index < generator.state.genvalues.size(); ++index)
        {
            typename generator_type::codegen_value const& value = generator.state.genvalues.at(index);
            if (value.template type_is< typename generator_type::codegen_local >())
            {
                parameter_values.emplace(value.template get_as< typename generator_type::codegen_local >().local_index, value_index(index));
            }
        }

        std::vector< std::pair< vmir2::routine_parameter, vmir2::routine_parameter > > parameters;
        if (logical.parameters.positional.size() != generator.state.params.positional.size())
        {
            throw compiler_bug("Coroutine INITIALIZE runtime parameter count differs from its body");
        }
        for (std::size_t index = 0; index < logical.parameters.positional.size(); ++index)
        {
            parameters.emplace_back(logical.parameters.positional.at(index), generator.state.params.positional.at(index));
        }
        for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : logical.parameters.named)
        {
            if (parameter.first == "CONTROLLER" || parameter.first == "__FRAME" || parameter.first == "RETURN") continue;
            std::string const name = parameter.first == "THIS" ? "RECEIVER" : parameter.first;
            parameters.emplace_back(parameter.second, generator.state.params.named.at(name));
        }

        std::vector< parameter_capture > captures;
        std::optional< parameter_capture > storage_receiver;
        bool requires_rollback = false;
        for (std::pair< vmir2::routine_parameter, vmir2::routine_parameter > const& parameter : parameters)
        {
            std::map< local_index, std::size_t >::const_iterator placement = frame_info.local_fields.find(parameter.first.local_index);
            if (placement == frame_info.local_fields.end())
            {
                throw compiler_bug("Coroutine frame omitted a captured runtime parameter");
            }
            struct_field const& field = frame_info.fields.at(placement->second);
            storage const& field_storage = field.type.template get_as< storage >();
            if (field_storage.storable_types.size() != 1)
            {
                throw compiler_bug("Coroutine parameter storage must have one semantic payload type");
            }
            type_symbol const& payload = *field_storage.storable_types.begin();
            if (co_await rpnx::querygraph::request< type_is_async_only_query >(payload))
            {
                throw semantic_compilation_error("Coroutine INITIALIZE requires synchronous parameter rollback for " + to_string(payload));
            }
            value_index source = parameter_values.at(parameter.second.local_index);
            type_symbol source_type = generator.current_type(current, source);
            bool const initializes_storage = parameter.first.type.template type_is< nvalue_slot >();
            bool const destroys_storage = parameter.first.type.template type_is< dvalue_slot >();
            std::optional< type_symbol > receiver_storage;
            if ((initializes_storage || destroys_storage) && remove_ref(source_type).template type_is< storage >())
            {
                receiver_storage = initializes_storage ? parameter.first.type.template get_as< nvalue_slot >().target : parameter.first.type.template get_as< dvalue_slot >().target;
                source_type = make_mref(*receiver_storage);
            }
            type_symbol constructor_symbol = co_await generator.co_select_constructor_entry(payload, false);
            instanciation_reference constructor = co_await generator.co_resolve_functum_instanciation(current, std::move(constructor_symbol),
                invotype{.named{{"THIS", nvalue_slot{payload}}, {"OTHER", source_type}}}, allowed_adaptations::source_rebinding);
            bool capture_noexcept = co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(constructor);
            if (receiver_storage.has_value() && !capture_noexcept)
                throw compiler_bug("Typed-storage coroutine receiver capture must be nonthrowing");
            requires_rollback = requires_rollback || !capture_noexcept;
            std::optional< instanciation_reference > const& source_destructor = co_await rpnx::querygraph::request< class_default_dtor_query >(generator.current_type(current, source));
            if (source_destructor.has_value())
            {
                requires_rollback = requires_rollback || !(co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(*source_destructor));
            }
            parameter_capture capture{.source = source, .payload_type = payload, .field_name = field.name, .constructor = std::move(constructor),
                .receiver_storage = std::move(receiver_storage), .initializes_storage = initializes_storage};
            if (capture.receiver_storage.has_value())
            {
                if (storage_receiver.has_value()) throw compiler_bug("Coroutine INITIALIZE has multiple typed-storage receivers");
                storage_receiver = std::move(capture);
            }
            else captures.push_back(std::move(capture));
        }

        type_symbol const continuation_type = int_type{.bits = 32, .has_sign = false};
        value_index continuation = co_await generator.co_generate_dot_access(current, generator.copy_ref_value(current, frame), "__CONTINUATION");
        co_await generator.co_generate_binary(current, ":=", continuation, generator.create_small_uint_value(current, 0, continuation_type));

        std::optional< value_index > exception;
        std::optional< value_index > cleanup_exception;
        std::optional< block_index > propagation;
        std::optional< block_index > rollback;
        std::optional< block_index > cleanup_failure;
        if (requires_rollback)
        {
            type_symbol const exception_type = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "EXCEPTION_PTR"};
            exception = co_await generator.co_gen_call_ctor(current, exception_type, {});
            cleanup_exception = co_await generator.co_gen_call_ctor(current, exception_type, {});
            propagation = generator.generate_subblock(current, "coroutine_initialize_propagate");
            typename generator_type::codegen_block& propagation_block = generator.state.blocks.at(*propagation);
            vmir2::state_map retained;
            for (value_index value : {frame, *exception, *cleanup_exception})
            {
                local_index local = generator.get_local_index(value);
                retained.emplace(local, propagation_block.entry_state.at(local));
            }
            propagation_block.entry_state = retained;
            propagation_block.current_state = std::move(retained);
            propagation_block.lookup_values.clear();
            propagation_block.lookup_tombstones.clear();
            cleanup_failure = generator.generate_subblock(*propagation, "coroutine_initialize_cleanup_failure");
            generator.set_terminator(*cleanup_failure, vmir2::panic{.message = "Exception during coroutine initialization rollback"});
            rollback = propagation;
        }

        for (parameter_capture const& capture : captures)
        {
            if (requires_rollback)
            {
                generator.state.catcher = vmir2::exception_catcher{.exception = generator.get_local_index(*exception), .handler = *rollback};
                block_index construction = generator.generate_subblock(current, "coroutine_initialize_argument");
                generator.generate_jump(current, construction);
                current = construction;
            }
            value_index storage_reference = co_await generator.co_generate_dot_access(current, generator.copy_ref_value(current, frame), capture.field_name);
            value_index destination = co_await generator.co_begin_storage_delegate(current, storage_reference, capture.payload_type, false);
            co_await generator.co_gen_call_functanoid(current, capture.constructor,
                codegen_invocation_args{.named{{"THIS", destination}, {"OTHER", capture.source}}}, allowed_adaptations::source_rebinding, false);

            if (requires_rollback)
            {
                generator.state.catcher = vmir2::exception_catcher{.exception = generator.get_local_index(*cleanup_exception), .handler = *cleanup_failure};
                block_index cleanup = generator.generate_subblock(*propagation, "coroutine_initialize_argument_rollback");
                block_index const cleanup_entry = cleanup;
                value_index rollback_storage = co_await generator.co_generate_dot_access(cleanup, generator.copy_ref_value(cleanup, frame), capture.field_name);
                value_index rollback_value = co_await generator.co_begin_storage_delegate(cleanup, rollback_storage, capture.payload_type, true);
                generator.emit(cleanup, vmir2::destroy{.of = generator.get_local_index(rollback_value)});
                generator.generate_jump(cleanup, *rollback);
                rollback = cleanup_entry;
            }
        }

        if (requires_rollback)
        {
            generator.state.catcher = vmir2::exception_catcher{.exception = generator.get_local_index(*exception), .handler = *rollback};
            block_index retirement = generator.generate_subblock(current, "coroutine_initialize_retire_arguments");
            generator.generate_jump(current, retirement);
            current = retirement;
        }
        for (typename std::vector< parameter_capture >::const_reverse_iterator capture = captures.crbegin(); capture != captures.crend(); ++capture)
        {
            if (generator.value_alive(current, capture->source))
            {
                generator.emit(current, vmir2::destroy{.of = generator.get_local_index(capture->source)});
            }
        }
        generator.state.catcher.reset();
        if (storage_receiver.has_value())
        {
            parameter_capture const& capture = *storage_receiver;
            value_index reference = generator.create_local_value(capture.payload_type);
            if (capture.initializes_storage)
            {
                value_index receiver = generator.create_local_value(*capture.receiver_storage);
                generator.emit(current, vmir2::storage_init_start{.on_storage = generator.get_local_index(capture.source),
                    .target_value = generator.get_local_index(receiver), .persistent = true});
                generator.emit(current, vmir2::make_reference{.value_index = generator.get_local_index(receiver),
                    .reference_index = generator.get_local_index(reference), .storage_only = true});
            }
            else
            {
                generator.emit(current, vmir2::storage_pun{.from_storage = generator.get_local_index(capture.source),
                    .as_type = *capture.receiver_storage, .to_reference = generator.get_local_index(reference)});
            }
            value_index storage_reference = co_await generator.co_generate_dot_access(current, generator.copy_ref_value(current, frame), capture.field_name);
            value_index destination = co_await generator.co_begin_storage_delegate(current, storage_reference, capture.payload_type, false);
            co_await generator.co_gen_call_functanoid(current, capture.constructor,
                codegen_invocation_args{.named{{"THIS", destination}, {"OTHER", reference}}}, allowed_adaptations::source_rebinding, false);
        }
        if (logical.coroutine->cancellation_state.has_value())
        {
            struct_field const& field = frame_info.fields.at(frame_info.local_fields.at(*logical.coroutine->cancellation_state));
            type_symbol const& payload = *field.type.template get_as< storage >().storable_types.begin();
            value_index storage_reference = co_await generator.co_generate_dot_access(current, generator.copy_ref_value(current, frame), field.name);
            value_index destination = co_await generator.co_begin_storage_delegate(current, storage_reference, payload, false);
            generator.emit(current, vmir2::load_const_int{.target = generator.get_local_index(destination), .value = "0"});
        }
        continuation = co_await generator.co_generate_dot_access(current, generator.copy_ref_value(current, frame), "__CONTINUATION");
        co_await generator.co_generate_binary(current, ":=", continuation, generator.create_small_uint_value(current, 1, continuation_type));
        generator.generate_return(current);

        if (requires_rollback)
        {
            co_await generator.co_gen_call_functum(*propagation,
                subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "THROW_EXCEPTION_PTR"},
                codegen_invocation_args{.named{{"ARG", *exception}}});
            generator.set_terminator(*propagation, vmir2::unreachable{});
        }
        co_await generator.co_generate_dtors();
        co_return generator.get_result();
    }
}

#endif
