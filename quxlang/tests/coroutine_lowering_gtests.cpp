// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>
#include <quxlang/data/coroutine_types.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/coroutine_lowering.hpp>
#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>
#include <quxlang/vmir2/state_engine.hpp>
#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>

#include <quxlang/vmir2/coroutine_output_initialization.hpp>
#include <quxlang/vmir2/local_borrow_analysis.hpp>

#include <algorithm>

namespace quxlang::vmir2
{
    /** Supplies a logical suspension whose value and destruction responsibility survive one activation. */
    class coroutine_lowering_test : public testing::Test
    {
      protected:
        type_symbol integer = int_type{.bits = 32, .has_sign = false};
        type_symbol frame_type = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "FRAME"};
        functanoid_routine3 logical;
        coroutine_frame_info frame;

        /** Defines a complete two-block input independently of the language generator. */
        void SetUp() override
        {
            logical.local_types = {{.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = make_mref(integer)}, {.type = integer}};
            logical.parameters.named.emplace("__FRAME", routine_parameter{.type = make_mref(frame_type), .local_index = local_index(1)});
            logical.parameters.named.emplace("CONTROLLER", routine_parameter{.type = make_mref(integer), .local_index = local_index(2)});
            logical.coroutine = coroutine_body{
                .frame = local_index(1), .controller = local_index(2),
                .continuations = {{.resume = block_index(1), .awaiter = local_index(3)}},
            };
            logical.blocks.push_back(executable_block{
                .instructions = {load_const_int{.target = local_index(3), .value = "42"}, co_save{.continuation = continuation_index(0)}},
                .terminator = co_suspend{.continuation = continuation_index(0)},
            });
            slot_state full{.stage = slot_stage::full, .storage_valid = true};
            logical.blocks.push_back(executable_block{
                .entry_state = {{local_index(1), full}, {local_index(2), full}, {local_index(3), full}},
                .terminator = co_complete{},
            });
            frame.fields = {{.name = "__CONTINUATION", .type = integer}, {.name = "__LOCAL3", .type = storage{.storable_types = {integer}}}};
            frame.local_fields.emplace(local_index(3), 1);
        }

        /** Evaluates the represented live state at the end of an ordinary driver block. */
        static auto exit_state(functanoid_routine3 const& routine, executable_block const& block) -> state_map
        {
            state_map state = block.entry_state;
            codegen_state_engine engine(state, routine.local_types, routine.parameters);
            for (vm_instruction const& instruction : block.instructions) engine.apply(instruction);
            return state;
        }
    };

    TEST_F(coroutine_lowering_test, driver_has_external_controller_and_direct_continuation_dispatch)
    {
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        EXPECT_FALSE(result.coroutine.has_value());
        EXPECT_TRUE(result.parameters.positional.empty());
        EXPECT_EQ(result.parameters.named.size(), 3U);
        EXPECT_EQ(result.parameters.named.at("THIS").local_index, local_index(1));
        EXPECT_EQ(result.parameters.named.at("CONTROLLER").local_index, local_index(2));
        EXPECT_EQ(result.parameters.named.at("RETURN").type, type_symbol(nvalue_slot{.target = bool_type{}}));
        EXPECT_EQ(result.local_types.at(result.parameters.named.at("RETURN").local_index).type, type_symbol(bool_type{}));
        tablebranch const& dispatch = result.blocks.front().terminator->as< tablebranch >();
        ASSERT_EQ(dispatch.targets.size(), 3U);
        executable_block const& continuation = result.blocks.at(dispatch.targets[2]);
        EXPECT_EQ(continuation.terminator->as< jump >().target, block_index(2));
        for (executable_block const& block : result.blocks)
        {
            ASSERT_TRUE(block.terminator.has_value());
            EXPECT_FALSE(block.terminator->type_is< co_suspend >());
            EXPECT_FALSE(block.terminator->type_is< co_complete >());
            for (vm_instruction const& instruction : block.instructions) EXPECT_FALSE(instruction.type_is< co_save >());
            EXPECT_NO_THROW(static_cast< void >(exit_state(result, block)));
        }
    }

    TEST_F(coroutine_lowering_test, suspended_exit_preserves_owned_values_and_completed_exit_cleans_them)
    {
        logical.non_trivial_dtors.emplace(integer, submember{.of = absolute_module_reference{"TEST"}, .name = "DESTROY_VALUE"});
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        executable_block const& suspend = result.blocks.at(result.blocks.at(1).terminator->as< jump >().target);
        ASSERT_TRUE(suspend.terminator->as< ret >().retain_storage);
        state_map suspended = exit_state(result, suspend);
        std::vector< lifetime_action > retained = codegen_state_engine::plan_transition(result, suspended, {}, lifetime_transition_kind::retained_exit);
        EXPECT_TRUE(std::none_of(retained.begin(), retained.end(), [](lifetime_action const& action) { return action.slot == local_index(3); }));

        executable_block const& before_completion = result.blocks.at(2);
        executable_block const& completed = result.blocks.at(before_completion.terminator->as< jump >().target);
        std::vector< lifetime_action > cleanup = codegen_state_engine::plan_transition(result, exit_state(result, before_completion), completed.entry_state);
        EXPECT_TRUE(std::any_of(cleanup.begin(), cleanup.end(), [](lifetime_action const& action)
        {
            return action.slot == local_index(3) && action.kind == lifetime_action_kind::destroy_object;
        }));
        EXPECT_FALSE(completed.entry_state.contains(local_index(3)));
    }

    TEST_F(coroutine_lowering_test, acquired_initguard_enters_persistent_storage_before_suspension)
    {
        logical.local_types.at(3).type = initguard_lock_type{};
        frame.fields.at(1).type = storage{.storable_types = {initguard_lock_type{}}};
        slot_state full{.stage = slot_stage::full, .storage_valid = true};
        state_map entry{{local_index(1), full}, {local_index(2), full}};
        state_map acquired = entry;
        acquired.emplace(local_index(3), full);
        type_symbol guarded = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "GUARDED"};
        logical.blocks = {
            executable_block{.entry_state = entry, .terminator = initguard_try_acquire{
                .symbol = guarded, .class_ = access_class::global, .target_lock = local_index(3),
                .target_acquired = block_index(1), .target_already_initialized = block_index(3)}},
            executable_block{.entry_state = acquired, .instructions = {co_save{.continuation = continuation_index(0)}},
                .terminator = co_suspend{.continuation = continuation_index(0)}},
            executable_block{.entry_state = acquired, .instructions = {initguard_abort{.lock = local_index(3)}}, .terminator = co_complete{}},
            executable_block{.entry_state = entry, .terminator = co_complete{}},
        };
        logical.coroutine->continuations[0].resume = block_index(2);
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        initguard_try_acquire const& acquire = result.blocks.at(1).terminator->as< initguard_try_acquire >();
        EXPECT_NE(acquire.target_lock, local_index(3));
        EXPECT_EQ(acquire.target_already_initialized, block_index(4));
        executable_block const& captured = result.blocks.at(acquire.target_acquired);
        EXPECT_TRUE(captured.entry_state.at(acquire.target_lock).alive());
        state_map stored = exit_state(result, captured);
        EXPECT_TRUE(stored.at(local_index(3)).alive());
        EXPECT_TRUE(stored.at(local_index(3)).retained_storage);
        EXPECT_FALSE(stored.at(acquire.target_lock).alive());
        EXPECT_EQ(captured.terminator->as< jump >().target, block_index(2));
        executable_block const& suspended = result.blocks.at(result.blocks.at(2).terminator->as< jump >().target);
        std::vector< lifetime_action > cleanup = codegen_state_engine::plan_transition(result, exit_state(result, suspended), {}, lifetime_transition_kind::retained_exit);
        EXPECT_TRUE(std::none_of(cleanup.begin(), cleanup.end(), [](lifetime_action const& action) { return action.slot == local_index(3); }));
        for (executable_block const& block : result.blocks) EXPECT_NO_THROW(static_cast< void >(exit_state(result, block)));
    }

    TEST_F(coroutine_lowering_test, repeated_continuation_destinations_share_restoration_blocks)
    {
        logical.coroutine->continuations.resize(2000, logical.coroutine->continuations.front());
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        tablebranch const& dispatch = result.blocks.front().terminator->as< tablebranch >();
        ASSERT_EQ(dispatch.targets.size(), 2002U);
        for (std::size_t index = 3; index < dispatch.targets.size(); ++index) EXPECT_EQ(dispatch.targets[index], dispatch.targets[2]);
        EXPECT_LT(result.blocks.size(), 12U);
    }

    TEST_F(coroutine_lowering_test, vmir_adapter_separates_suspension_and_captures_hidden_lifetimes)
    {
        coroutine_vmir_analysis input = coroutine_analysis_input(logical);
        ASSERT_EQ(input.input.nodes.size(), 4U);
        EXPECT_FALSE(input.input.nodes.at(0).may_suspend);
        EXPECT_TRUE(input.input.nodes.at(2).may_suspend);
        EXPECT_TRUE(input.input.nodes.at(input.input.entry_node).may_suspend);
        coroutine_frame_analysis analysis = analyze_coroutine_frame(input.input);
        EXPECT_TRUE(analysis.objects.at(3).offset.has_value());
        EXPECT_FALSE(analysis.objects.at(1).offset.has_value());
        EXPECT_FALSE(analysis.objects.at(2).offset.has_value());
    }

    TEST_F(coroutine_lowering_test, completed_block_lifetime_does_not_persist_in_a_suspending_cycle)
    {
        logical.local_types.push_back({.type = integer});
        logical.blocks[0].instructions.insert(logical.blocks[0].instructions.begin(), {
            load_const_int{.target = local_index(4), .value = "7"}, end_lifetime{.of = local_index(4)},
        });
        logical.blocks[1].terminator = jump{.target = block_index(0)};
        coroutine_vmir_analysis input = coroutine_analysis_input(logical);
        coroutine_frame_analysis analysis = analyze_coroutine_frame(input.input);
        EXPECT_TRUE(analysis.objects.at(3).offset.has_value());
        EXPECT_FALSE(analysis.objects.at(4).offset.has_value());
    }

    TEST_F(coroutine_lowering_test, intrinsic_borrows_preserve_activation_local_storage)
    {
        logical.local_types.push_back({.type = integer});
        logical.local_types.push_back({.type = make_mref(integer)});
        logical.local_types.push_back({.type = integer});
        logical.blocks[0].instructions.insert(logical.blocks[0].instructions.begin(), {
            load_const_int{.target = local_index(4), .value = "7"},
            make_reference{.value_index = local_index(4), .reference_index = local_index(5)},
            load_from_ref{.from_reference = local_index(5), .to_value = local_index(6)},
            end_lifetime{.of = local_index(6)},
            end_lifetime{.of = local_index(4)},
        });
        logical.blocks[1].terminator = jump{.target = block_index(0)};
        coroutine_vmir_analysis input = coroutine_analysis_input(logical);
        EXPECT_TRUE(input.input.objects.at(4).lifetime_complete);
        EXPECT_TRUE(input.input.objects.at(4).requires_stable_address);
        EXPECT_FALSE(analyze_coroutine_frame(input.input).objects.at(4).offset.has_value());

        type_symbol call = instanciation_reference{
            .temploid = {.templexoid = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "ESCAPE"}},
            .params = {.named = {{"ARG", make_type_instantiation(make_mref(integer))}}},
        };
        logical.blocks[0].instructions[2] = invoke{.what = call, .args = {.named = {{"ARG", local_index(5)}}}};
        logical.blocks[0].instructions.erase(logical.blocks[0].instructions.begin() + 3);
        coroutine_vmir_analysis escaped = coroutine_analysis_input(logical);
        EXPECT_FALSE(escaped.input.objects.at(4).lifetime_complete);
        EXPECT_TRUE(analyze_coroutine_frame(escaped.input).objects.at(4).offset.has_value());
    }

    TEST_F(coroutine_lowering_test, cancellation_dispatches_only_eligible_claim_continuations)
    {
        type_symbol byte_integer = int_type{.bits = 8, .has_sign = false};
        logical.local_types.push_back({.type = byte_integer});
        logical.coroutine->cancellation_mode = coroutine_cancellation_mode::cooperative;
        logical.coroutine->cancellation_state = local_index(4);
        logical.coroutine->continuations[0].cancellation = block_index(2);
        slot_state full{.stage = slot_stage::full, .storage_valid = true};
        logical.blocks[0].entry_state = {{local_index(1), full}, {local_index(2), full}, {local_index(4), full}};
        logical.blocks[1].entry_state.emplace(local_index(4), full);
        logical.blocks.push_back(executable_block{.entry_state = logical.blocks[1].entry_state, .terminator = co_suspend{.continuation = continuation_index(0)}});
        frame.fields.push_back({.name = "__LOCAL4", .type = storage{.storable_types = {byte_integer}}});
        frame.local_fields.emplace(local_index(4), 2);

        functanoid_routine3 result = lower_coroutine_try_cancel(logical, frame);
        tablebranch const& entry = result.blocks[0].terminator->as< tablebranch >();
        EXPECT_TRUE(result.blocks.at(entry.default_target).instructions.front().as< load_const_bool >().value);
        executable_block const& attempt = result.blocks.at(entry.targets[1]);
        tablebranch const& attempts = attempt.terminator->as< tablebranch >();
        EXPECT_FALSE(result.blocks.at(attempts.default_target).instructions.front().as< load_const_bool >().value);
        executable_block const& active = result.blocks.at(attempts.targets[1]);
        tablebranch const& active_states = active.terminator->as< tablebranch >();
        executable_block const& cancellation = result.blocks.at(active_states.targets[2]);
        EXPECT_EQ(cancellation.terminator->as< jump >().target, block_index(3));
        for (executable_block const& block : result.blocks) EXPECT_NO_THROW(static_cast< void >(exit_state(result, block)));

        logical.coroutine->continuations[0].shielded = true;
        functanoid_routine3 shielded = lower_coroutine_try_cancel(logical, frame);
        tablebranch const& shielded_entry = shielded.blocks[0].terminator->as< tablebranch >();
        tablebranch const& shielded_attempts = shielded.blocks.at(shielded_entry.targets[1]).terminator->as< tablebranch >();
        tablebranch const& shielded_states = shielded.blocks.at(shielded_attempts.targets[1]).terminator->as< tablebranch >();
        EXPECT_EQ(shielded_states.targets[2], shielded_attempts.default_target);
    }

    TEST_F(coroutine_lowering_test, completed_array_element_uses_saved_address_instead_of_current_prefix)
    {
        type_symbol array = array_type{.element_type = integer, .element_count = expression_numeric_literal{.value = "2"}};
        type_symbol initializer = array_initializer_type{.element_type = integer};
        logical.local_types.at(3).type = array;
        logical.local_types.push_back({.type = initializer});
        logical.local_types.push_back({.type = integer});
        logical.coroutine->continuations[0].awaiter = local_index(4);
        logical.blocks[0].instructions = {
            array_init_start{.source = local_index(3), .initializer = local_index(4)},
            array_init_element{.initializer = local_index(4), .target = local_index(5)},
            load_const_int{.target = local_index(5), .value = "42"},
            co_save{.continuation = continuation_index(0)},
        };
        state_map state;
        codegen_state_engine engine(state, logical.local_types, logical.parameters);
        engine.apply_entry();
        for (vm_instruction const& instruction : logical.blocks[0].instructions) engine.apply(instruction);
        logical.blocks[1].entry_state = state;
        frame.fields.at(1).type = storage{.storable_types = {array}};
        frame.fields.push_back({.name = "__LOCAL4", .type = storage{.storable_types = {initializer}}});
        frame.fields.push_back({.name = "__LOCAL5", .type = storage{.storable_types = {make_mref(integer)}}});
        frame.local_fields.emplace(local_index(4), 2);
        frame.local_fields.emplace(local_index(5), 3);
        frame.saved_array_element_addresses.insert(local_index(5));

        coroutine_vmir_analysis input = coroutine_analysis_input(logical);
        EXPECT_TRUE(input.saved_array_element_addresses.contains(local_index(5)));
        EXPECT_TRUE(analyze_coroutine_frame(input.input).objects.at(5).offset.has_value());
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        tablebranch const& dispatch = result.blocks[0].terminator->as< tablebranch >();
        executable_block const& resume = result.blocks.at(dispatch.targets[2]);
        EXPECT_TRUE(resume.instructions.empty());
        retained_storage_binding const& binding = result.retained_bindings.at(local_index(5));
        EXPECT_EQ(binding.kind, retained_storage_kind::completed_array_reference);
        EXPECT_EQ(binding.field_name, "__LOCAL5");
        ASSERT_TRUE(binding.saved_reference.has_value());
        EXPECT_EQ(result.local_types.at(*binding.saved_reference).type, make_mref(integer));
        EXPECT_TRUE(resume.entry_state.at(*binding.saved_reference).alive());
        for (executable_block const& block : result.blocks) EXPECT_NO_THROW(static_cast< void >(exit_state(result, block)));
    }

    TEST_F(coroutine_lowering_test, distinct_continuations_share_live_storage_without_product_expansion)
    {
        constexpr std::size_t locals = 128;
        constexpr std::size_t suspensions = 256;
        slot_state full{.stage = slot_stage::full, .storage_valid = true};
        state_map live = logical.blocks[1].entry_state;
        for (std::size_t index = 1; index < locals; ++index)
        {
            local_index local(logical.local_types.size());
            logical.local_types.push_back({.type = integer});
            logical.blocks[0].instructions.insert(logical.blocks[0].instructions.end() - 1, load_const_int{.target = local, .value = "1"});
            live.emplace(local, full);
            frame.local_fields.emplace(local, frame.fields.size());
            frame.fields.push_back({.name = "__LOCAL" + std::to_string(local), .type = storage{.storable_types = {integer}}});
        }
        logical.blocks.resize(1);
        logical.coroutine->continuations.clear();
        for (std::size_t index = 0; index < suspensions; ++index)
        {
            logical.coroutine->continuations.push_back({.resume = block_index(index + 1), .awaiter = local_index(3)});
            if (index != 0)
            {
                logical.blocks.push_back({
                    .entry_state = live,
                    .instructions = {co_save{.continuation = continuation_index(index)}},
                    .terminator = co_suspend{.continuation = continuation_index(index)},
                });
            }
        }
        logical.blocks.push_back({.entry_state = live, .terminator = co_complete{}});
        coroutine_vmir_analysis analysis = coroutine_analysis_input(logical);
        std::size_t events = 0;
        for (coroutine_frame_object const& object : analysis.input.objects) events += object.events.size();
        EXPECT_LT(analysis.snapshot_nodes, 32 * (locals + suspensions));
        EXPECT_LT(events, 16 * (locals + suspensions));
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        std::size_t instructions = 0;
        lifetime_snapshot_graph< state_map > states;
        for (executable_block const& block : result.blocks)
        {
            instructions += block.instructions.size();
            states.append(block.entry_state.snapshot_root());
        }
        EXPECT_EQ(result.retained_bindings.size(), locals);
        EXPECT_LT(instructions, 12 * (locals + suspensions));
        EXPECT_LT(states.nodes().size(), 32 * (locals + suspensions));
        RecordProperty("live_locals", locals);
        RecordProperty("suspensions", suspensions);
        RecordProperty("analysis_snapshot_nodes", analysis.snapshot_nodes);
        RecordProperty("lifetime_events", events);
        RecordProperty("driver_instructions", instructions);
        RecordProperty("driver_snapshot_nodes", states.nodes().size());
    }

    TEST_F(coroutine_lowering_test, completion_sites_share_output_publication_and_capture_teardown)
    {
        constexpr std::size_t outputs = 64;
        constexpr std::size_t returns = 128;
        for (std::size_t index = 0; index < outputs; ++index)
        {
            local_index local(index + 3);
            if (index != 0)
            {
                logical.local_types.push_back({.type = integer});
                logical.blocks[0].instructions.insert(logical.blocks[0].instructions.end() - 1, load_const_int{.target = local, .value = "1"});
                frame.local_fields.emplace(local, frame.fields.size());
                frame.fields.push_back({.name = "__OUTPUT" + std::to_string(index), .type = storage{.storable_types = {make_mref(integer)}}});
            }
            logical.parameters.named.emplace("OUTPUT" + std::to_string(index), routine_parameter{.type = nvalue_slot{.target = integer}, .local_index = local});
            frame.borrowed_parameters.emplace(local, nvalue_slot{.target = integer});
        }
        frame.fields.at(1).type = storage{.storable_types = {make_mref(integer)}};
        state_map state;
        codegen_state_engine engine(state, logical.local_types, logical.parameters);
        engine.apply_entry();
        for (vm_instruction const& instruction : logical.blocks[0].instructions) engine.apply(instruction);
        logical.blocks.resize(1);
        for (std::size_t index = 0; index < returns; ++index) logical.blocks.push_back({.entry_state = state, .terminator = co_complete{}});
        coroutine_output_initialization validation(logical, frame.borrowed_parameters);
        for (std::size_t index = 0; index < returns; ++index) EXPECT_TRUE(validation.completed(logical.blocks[index + 1].entry_state));
        EXPECT_LT(validation.snapshot_visits(), 8 * (outputs + returns));
        state[local_index(3)].stage = slot_stage::dead;
        EXPECT_FALSE(validation.completed(state));
        state[local_index(3)].stage = slot_stage::full;
        state[local_index(3)].construction_pending = true;
        EXPECT_FALSE(validation.completed(state));
        functanoid_routine3 result = lower_coroutine_resume(logical, frame);
        std::size_t completion_blocks = 0;
        std::size_t publication_instructions = 0;
        std::size_t instructions = 0;
        for (executable_block const& block : result.blocks)
        {
            if (block.dbg_name == std::optional< std::string >("COROUTINE_COMPLETE")) ++completion_blocks;
            instructions += block.instructions.size();
            for (vm_instruction const& instruction : block.instructions)
            {
                if (instruction.type_is< complete_initialization >()) ++publication_instructions;
            }
            EXPECT_NO_THROW(static_cast< void >(exit_state(result, block)));
        }
        EXPECT_EQ(completion_blocks, 1U);
        EXPECT_EQ(publication_instructions, outputs);
        EXPECT_LT(instructions, 16 * (outputs + returns));
        RecordProperty("output_snapshot_visits", validation.snapshot_visits());
        RecordProperty("completion_driver_instructions", instructions);
    }

    TEST_F(coroutine_lowering_test, output_summary_requires_every_nested_delegate)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(5, local_type{.type = integer});
        state_map state;
        state[local_index(0)] = slot_state{
            .stage = slot_stage::partial,
            .storage_valid = true,
            .struct_delegates = std::vector< struct_init_delegate >{
                {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(1)},
                {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(2)},
            },
        };
        state[local_index(1)] = slot_state{
            .stage = slot_stage::full, .storage_valid = true,
            .delegate_of = local_index(0), .struct_delegate_selector = struct_init_field_selector{.field_ordinal = 0},
        };
        state[local_index(2)] = slot_state{
            .stage = slot_stage::partial,
            .storage_valid = true,
            .struct_delegates = std::vector< struct_init_delegate >{
                {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)},
                {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(4)},
            },
            .delegate_of = local_index(0), .struct_delegate_selector = struct_init_field_selector{.field_ordinal = 1},
        };
        for (std::size_t index = 3; index < 5; ++index)
        {
            state[local_index(index)] = slot_state{
                .stage = slot_stage::full, .storage_valid = true,
                .delegate_of = local_index(2), .struct_delegate_selector = struct_init_field_selector{.field_ordinal = index - 3},
            };
        }
        routine.blocks.push_back({.entry_state = state});
        coroutine_output_initialization validation(routine, {{local_index(0), nvalue_slot{.target = frame_type}}});
        EXPECT_TRUE(validation.completed(state));
        state[local_index(4)].stage = slot_stage::dead;
        EXPECT_FALSE(validation.completed(state));
        state[local_index(4)].stage = slot_stage::full;
        state[local_index(0)].construction_pending = true;
        EXPECT_FALSE(validation.completed(state));
        state.clear();
        EXPECT_FALSE(validation.completed(state));
    }
    TEST_F(coroutine_lowering_test, escaped_construction_delegates_retain_their_containing_storage)
    {
        instanciation_reference constructor{
            .temploid = {.templexoid = submember{.of = frame_type, .name = "CONSTRUCTOR"}},
            .params = {.named = {{"THIS", make_type_instantiation(nvalue_slot{.target = integer})}}},
        };
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = frame_type}, {.type = integer},
            {.type = array_type{.element_type = integer, .element_count = expression_numeric_literal{.value = "4"}}}, {.type = array_initializer_type{.element_type = integer}}, {.type = integer}};
        routine.blocks.push_back({.instructions = {
            struct_init_start{.on_value = local_index(1), .delegates = {{.selector = struct_init_field_selector{}, .value = local_index(2)}}},
            invoke{.what = constructor, .args = {.named = {{"THIS", local_index(2)}}}},
            array_init_start{.source = local_index(3), .initializer = local_index(4)},
            array_init_element{.initializer = local_index(4), .target = local_index(5)},
            invoke{.what = constructor, .args = {.named = {{"THIS", local_index(5)}}}},
        }});
        local_borrow_analysis unknown = analyze_local_borrows(routine);
        EXPECT_TRUE(unknown.escaped_storage.at(1));
        EXPECT_TRUE(unknown.escaped_storage.at(3));
        invocation_borrow_summary summary{.nonescaping_named = {"THIS"}};
        local_borrow_analysis bounded = analyze_local_borrows(routine, {{constructor, summary}});
        EXPECT_FALSE(bounded.escaped_storage.at(1));
        EXPECT_FALSE(bounded.escaped_storage.at(3));
    }

    TEST_F(coroutine_lowering_test, publishing_a_borrowed_pointer_is_an_escape)
    {
        functanoid_routine3 routine;
        type_symbol reference = make_mref(integer);
        routine.local_types = {{.type = void_type{}}, {.type = reference}, {.type = reference}};
        routine.parameters.named["ARG"] = {.type = reference, .local_index = local_index(1)};
        routine.parameters.named["RETURN"] = {.type = nvalue_slot{.target = reference}, .local_index = local_index(2)};
        routine.blocks.push_back({.instructions = {copy_reference{.from_index = local_index(1), .to_index = local_index(2)}}});
        local_borrow_analysis result = analyze_local_borrows(routine);
        EXPECT_TRUE(result.escaped_values.at(1));
    }

    TEST_F(coroutine_lowering_test, incomplete_borrow_proofs_publish_no_result)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}, {.type = make_mref(integer)}};
        routine.blocks.push_back({.instructions = {make_reference{.value_index = local_index(1), .reference_index = local_index(2)}}});
        std::uint64_t remaining = 2;
        local_borrow_analysis exhausted = analyze_local_borrows(routine, {}, &remaining);
        EXPECT_FALSE(exhausted.complete);
        EXPECT_TRUE(exhausted.escaped_storage.empty());
        EXPECT_TRUE(exhausted.escaped_values.empty());
        remaining = 1000;
        local_borrow_analysis memory_limited = analyze_local_borrows(routine, {}, &remaining, 0);
        EXPECT_FALSE(memory_limited.complete);
        EXPECT_TRUE(memory_limited.escaped_values.empty());
    }

}
