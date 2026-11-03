// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/ir2_constexpr_interpreter.hpp>
#include <quxlang/vmir2/lifetime_cleanup_program.hpp>
#include <quxlang/vmir2/state_engine.hpp>

namespace quxlang::vmir2
{
    /** Exercises storage ownership across independent interpreter activations. */
    class persistent_storage_test : public testing::Test
    {
      protected:
        type_symbol integer = int_type{.bits = 32, .has_sign = false};
        type_symbol storage_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "STORAGE"};
        type_symbol first_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "FIRST"};
        type_symbol second_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "SECOND"};
        ir2_constexpr_interpreter interpreter;
    };

    TEST_F(persistent_storage_test, complete_payload_survives_return_and_can_be_reused)
    {
        storage storage_type{.storable_types = {integer}};
        functanoid_routine3 initialize;
        initialize.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                load_const_int{.target = local_index(2), .value = "42"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = {
            {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer},
            {.type = make_cref(integer)}, {.type = integer},
        };
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_rebind{.on_storage = local_index(1), .target_value = local_index(2), .owns_lifetime = true},
                make_reference{.value_index = local_index(2), .reference_index = local_index(3)},
                load_from_ref{.from_reference = local_index(3), .to_value = local_index(4)},
                constexpr_set_result{.target = local_index(4)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));

        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
    }

    TEST_F(persistent_storage_test, partial_struct_rebinds_initialized_and_uninitialized_members)
    {
        type_symbol record = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "RECORD"};
        storage storage_type{.storable_types = {record}};
        interpreter.add_struct_definition(record, {{.name = "FIRST", .type = integer}, {.name = "SECOND", .type = integer}});
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = record}, {.type = integer}, {.type = integer},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                struct_init_start{.on_value = local_index(2), .delegates = {
                    {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)},
                    {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(4)},
                }},
                load_const_int{.target = local_index(3), .value = "10"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = {
            {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = record}, {.type = integer}, {.type = integer},
            {.type = integer}, {.type = integer}, {.type = make_cref(record)}, {.type = make_cref(integer)}, {.type = integer},
            {.type = make_cref(record)}, {.type = make_cref(integer)}, {.type = integer}, {.type = integer},
        };
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_rebind{
                    .on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::partial, .owns_lifetime = true,
                    .struct_delegates = std::vector< struct_init_delegate >{
                        {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(5)},
                        {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(6)},
                    },
                    .struct_delegate_stages = {slot_stage::full, slot_stage::dead},
                },
                load_const_int{.target = local_index(6), .value = "32"},
                struct_init_finish{.on_value = local_index(2)},
                make_reference{.value_index = local_index(2), .reference_index = local_index(7)},
                access_field{.base_index = local_index(7), .store_index = local_index(8), .field_name = "FIRST"},
                load_from_ref{.from_reference = local_index(8), .to_value = local_index(9)},
                make_reference{.value_index = local_index(2), .reference_index = local_index(10)},
                access_field{.base_index = local_index(10), .store_index = local_index(11), .field_name = "SECOND"},
                load_from_ref{.from_reference = local_index(11), .to_value = local_index(12)},
                int_add{.a = local_index(9), .b = local_index(12), .result = local_index(13)},
                constexpr_set_result{.target = local_index(13)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));

        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
    }

    TEST_F(persistent_storage_test, uninitialized_retained_payload_cannot_be_punned)
    {
        storage storage_type{.storable_types = {integer}};
        functanoid_routine3 initialize;
        initialize.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 access;
        access.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = make_cref(integer)}};
        access.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_pun{.from_storage = local_index(1), .as_type = integer, .to_reference = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(access));

        interpreter.exec3(first_symbol);
        EXPECT_THROW(interpreter.exec3(second_symbol), constexpr_logic_execution_error);
    }

    TEST_F(persistent_storage_test, partial_array_retains_its_initialized_prefix)
    {
        type_symbol array = array_type{.element_type = integer, .element_count = expression_numeric_literal{.value = "2"}};
        type_symbol initializer = array_initializer_type{.element_type = integer};
        storage array_storage{.storable_types = {array}};
        storage initializer_storage{.storable_types = {initializer}};
        type_symbol initializer_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "INITIALIZER"};
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(array_storage)}, {.type = array},
            {.type = make_mref(initializer_storage)}, {.type = initializer}, {.type = integer},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                get_object_ref{.symbol = initializer_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(3)},
                storage_init_start{.on_storage = local_index(3), .target_value = local_index(4), .persistent = true, .owns_lifetime = true},
                array_init_start{.source = local_index(2), .initializer = local_index(4)},
                array_init_element{.initializer = local_index(4), .target = local_index(5)},
                load_const_int{.target = local_index(5), .value = "10"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = initialize.local_types;
        complete.local_types.push_back({.type = integer});
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                get_object_ref{.symbol = initializer_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(3)},
                storage_rebind{.on_storage = local_index(3), .target_value = local_index(4), .owns_lifetime = true},
                storage_rebind{
                    .on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::partial, .owns_lifetime = true,
                    .array_delegates = invocation_args{.named = {{"INIT", local_index(4)}}},
                },
                array_init_index{.initializer = local_index(4), .result = local_index(6)},
                constexpr_set_result{.target = local_index(6)},
                array_init_element{.initializer = local_index(4), .target = local_index(5)},
                load_const_int{.target = local_index(5), .value = "32"},
                array_init_finish{.initializer = local_index(4)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));

        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 1);
    }

    TEST_F(persistent_storage_test, owned_retained_binding_is_released_on_normal_exit)
    {
        storage storage_type{.storable_types = {integer}};
        functanoid_routine3 initialize;
        initialize.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                load_const_int{.target = local_index(2), .value = "42"},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));

        interpreter.exec3(first_symbol);
        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
    }

    TEST_F(persistent_storage_test, retained_array_prefix_can_be_destroyed_in_reverse_order)
    {
        type_symbol array = array_type{.element_type = integer, .element_count = expression_numeric_literal{.value = "3"}};
        type_symbol initializer = array_initializer_type{.element_type = integer};
        storage array_storage{.storable_types = {array}};
        storage initializer_storage{.storable_types = {initializer}};
        type_symbol initializer_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "INITIALIZER"};
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(array_storage)}, {.type = array},
            {.type = make_mref(initializer_storage)}, {.type = initializer}, {.type = integer}, {.type = integer},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                get_object_ref{.symbol = initializer_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(3)},
                storage_init_start{.on_storage = local_index(3), .target_value = local_index(4), .persistent = true, .owns_lifetime = true},
                array_init_start{.source = local_index(2), .initializer = local_index(4)},
                array_init_element{.initializer = local_index(4), .target = local_index(5)},
                load_const_int{.target = local_index(5), .value = "10"},
                array_init_element{.initializer = local_index(4), .target = local_index(6)},
                load_const_int{.target = local_index(6), .value = "32"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 rollback;
        rollback.local_types = initialize.local_types;
        rollback.local_types.insert(rollback.local_types.end(), {
            {.type = make_cref(integer)}, {.type = integer}, {.type = make_cref(integer)}, {.type = integer}, {.type = integer},
        });
        rollback.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                get_object_ref{.symbol = initializer_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(3)},
                storage_rebind{.on_storage = local_index(3), .target_value = local_index(4), .owns_lifetime = true},
                storage_rebind{.on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::partial, .owns_lifetime = true,
                    .array_delegates = invocation_args{.named = {{"INIT", local_index(4)}}}},
                array_deinit_previous{.initializer = local_index(4), .target = local_index(5)},
                make_reference{.value_index = local_index(5), .reference_index = local_index(7)},
                load_from_ref{.from_reference = local_index(7), .to_value = local_index(8)},
                end_lifetime{.of = local_index(5)},
                array_deinit_previous{.initializer = local_index(4), .target = local_index(6)},
                make_reference{.value_index = local_index(6), .reference_index = local_index(9)},
                load_from_ref{.from_reference = local_index(9), .to_value = local_index(10)},
                end_lifetime{.of = local_index(6)},
                int_sub{.a = local_index(8), .b = local_index(10), .result = local_index(11)},
                constexpr_set_result{.target = local_index(11)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(rollback));

        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 22);
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 22);
    }

    TEST_F(persistent_storage_test, destruction_transfer_disarms_registered_cleanup)
    {
        storage storage_type{.storable_types = {integer}};
        type_symbol destructor = submember{.of = absolute_module_reference{"TEST"}, .name = "UNREACHABLE_DESTRUCTOR"};
        functanoid_routine3 initialize;
        initialize.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                load_const_int{.target = local_index(2), .value = "42"},
                defer_nontrivial_dtor{.func = destructor, .on_value = local_index(2), .args = {.named = {{"THIS", local_index(2)}}}},
                begin_destruction{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));

        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
    }

    TEST_F(persistent_storage_test, nested_partial_struct_delegates_are_restored)
    {
        type_symbol inner = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "INNER"};
        type_symbol outer = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "OUTER"};
        interpreter.add_struct_definition(inner, {{.name = "FIRST", .type = integer}, {.name = "SECOND", .type = integer}});
        interpreter.add_struct_definition(outer, {{.name = "NESTED", .type = inner}});
        storage storage_type{.storable_types = {outer}};
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = outer}, {.type = inner}, {.type = integer}, {.type = integer},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                struct_init_start{.on_value = local_index(2), .delegates = {{.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)}}},
                struct_init_start{.on_value = local_index(3), .delegates = {
                    {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(4)},
                    {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(5)},
                }},
                load_const_int{.target = local_index(4), .value = "10"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = initialize.local_types;
        complete.local_types.insert(complete.local_types.end(), {{.type = make_cref(integer)}, {.type = integer}});
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_rebind{
                    .on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::partial, .owns_lifetime = true,
                    .struct_delegates = std::vector< struct_init_delegate >{{.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)}},
                    .struct_delegate_stages = {slot_stage::partial},
                    .nested_delegates = {{
                        .target_value = local_index(3), .stage = slot_stage::partial,
                        .struct_delegates = std::vector< struct_init_delegate >{
                            {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(4)},
                            {.selector = struct_init_field_selector{.field_ordinal = 1}, .value = local_index(5)},
                        },
                        .struct_delegate_stages = {slot_stage::full, slot_stage::dead},
                    }},
                },
                make_reference{.value_index = local_index(4), .reference_index = local_index(6)},
                load_from_ref{.from_reference = local_index(6), .to_value = local_index(7)},
                constexpr_set_result{.target = local_index(7)},
                load_const_int{.target = local_index(5), .value = "32"},
                struct_init_finish{.on_value = local_index(3)},
                struct_init_finish{.on_value = local_index(2)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));

        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 10);
    }

    TEST_F(persistent_storage_test, storage_borrow_preserves_the_later_constructed_destination)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}, {.type = make_mref(integer)}, {.type = integer}};
        routine.blocks.push_back(executable_block{
            .instructions = {
                make_reference{.value_index = local_index(1), .reference_index = local_index(2), .storage_only = true},
                load_const_int{.target = local_index(1), .value = "42"},
                load_from_ref{.from_reference = local_index(2), .to_value = local_index(3)},
                constexpr_set_result{.target = local_index(3)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(routine));
        interpreter.exec3(first_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
    }

    TEST_F(persistent_storage_test, storage_borrow_does_not_permit_an_uninitialized_read)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}, {.type = make_mref(integer)}, {.type = integer}};
        routine.blocks.push_back(executable_block{
            .instructions = {
                make_reference{.value_index = local_index(1), .reference_index = local_index(2), .storage_only = true},
                load_from_ref{.from_reference = local_index(2), .to_value = local_index(3)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(routine));
        EXPECT_THROW(interpreter.exec3(first_symbol), constexpr_logic_execution_error);
    }

    TEST_F(persistent_storage_test, reserved_owned_storage_is_released_before_construction)
    {
        storage storage_type{.storable_types = {integer}};
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}, {.type = make_mref(integer)}};
        routine.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                make_reference{.value_index = local_index(2), .reference_index = local_index(3), .storage_only = true},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(routine));
        interpreter.exec3(first_symbol);
        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
    }

    TEST_F(persistent_storage_test, externally_completed_destination_is_published_after_rebinding)
    {
        storage storage_type{.storable_types = {integer}};
        type_symbol construct_symbol = instanciation_reference{
            .temploid = {.templexoid = submember{.of = absolute_module_reference{"TEST"}, .name = "CONSTRUCT"}},
            .params = {.named = {{"DESTINATION", make_type_instantiation(make_mref(integer))}}},
        };
        functanoid_routine3 construct;
        construct.local_types = {{.type = void_type{}}, {.type = make_mref(integer)}, {.type = integer}};
        construct.parameters.named["DESTINATION"] = {.type = make_mref(integer), .local_index = local_index(1)};
        state_map construct_entry;
        codegen_state_engine(construct_entry, construct.local_types, construct.parameters).apply_entry();
        construct.blocks.push_back(executable_block{
            .entry_state = std::move(construct_entry),
            .instructions = {
                storage_rebind{.on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::dead, .construction_pending = true},
                load_const_int{.target = local_index(2), .value = "42"},
            },
            .terminator = ret{},
        });
        functanoid_routine3 initialize;
        initialize.local_types = {{.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = integer}, {.type = make_mref(integer)}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                make_reference{.value_index = local_index(2), .reference_index = local_index(3), .storage_only = true},
                invoke{.what = construct_symbol, .args = {.named = {{"DESTINATION", local_index(3)}}}},
            },
            .terminator = ret{.retain_storage = true},
        });
        state_map pending;
        codegen_state_engine pending_engine(pending, initialize.local_types, initialize.parameters);
        for (vm_instruction const& instruction : initialize.blocks.front().instructions) pending_engine.apply(instruction);
        initialize.blocks.front().terminator = jump{.target = block_index(1)};
        initialize.blocks.push_back(executable_block{.entry_state = pending, .terminator = ret{.retain_storage = true}});
        functanoid_routine3 complete;
        complete.local_types = initialize.local_types;
        complete.local_types.push_back({.type = integer});
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_rebind{.on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::dead, .owns_lifetime = true, .construction_pending = true},
                complete_initialization{.of = local_index(2)},
                make_reference{.value_index = local_index(2), .reference_index = local_index(3)},
                load_from_ref{.from_reference = local_index(3), .to_value = local_index(4)},
                constexpr_set_result{.target = local_index(4)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(construct_symbol, std::move(construct));
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
    }
    TEST_F(persistent_storage_test, base_lifecycle_phase_survives_resumption_and_restores_enclosing_type)
    {
        type_symbol base = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "BASE"};
        type_symbol derived = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "DERIVED"};
        type_symbol guard_symbol = submember{.of = absolute_module_reference{"TEST"}, .name = "PHASE"};
        storage object_storage{.storable_types = {derived}};
        storage phase_storage{.storable_types = {address_type{}}};
        destructor_subobject_context context{
            .enclosing_object = local_index(2), .selector = struct_init_direct_base_selector{.direct_base_ordinal = 0}};
        struct_lifecycle_phase phase{.on_value = local_index(3), .kind = struct_phase_kind::construction, .subobject_context = context};
        interpreter.add_struct_definition(base, {});
        interpreter.add_struct_layout(derived, struct_layout{
            .direct_bases = {{.subobject = {.nonvirtual_path = {0}}, .type = base, .declaration_ordinal = 0}},
        });
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(object_storage)}, {.type = derived}, {.type = base},
            {.type = make_mref(phase_storage)}, {.type = address_type{}}, {.type = make_cref(base)},
            {.type = bool_type{}}, {.type = bool_type{}}, {.type = make_cref(base)},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                struct_init_start{.on_value = local_index(2), .delegates = {
                    {.selector = struct_init_direct_base_selector{.direct_base_ordinal = 0}, .value = local_index(3)}}},
                get_object_ref{.symbol = guard_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(4)},
                storage_init_start{.on_storage = local_index(4), .target_value = local_index(5), .persistent = true, .owns_lifetime = true},
                struct_phase_enter{.on_value = local_index(3), .saved_phase = local_index(5), .subobject_context = context},
                struct_init_start{.on_value = local_index(3)},
                struct_init_finish{.on_value = local_index(3)},
                make_reference{.value_index = local_index(3), .reference_index = local_index(6)},
                struct_type_is{.source = local_index(6), .target_type = base, .result = local_index(7)},
                assert_instr{.condition = local_index(7), .expr_text = "the active type is the constructed base"},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = initialize.local_types;
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                storage_rebind{
                    .on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::partial, .owns_lifetime = true,
                    .struct_delegates = std::vector< struct_init_delegate >{{.selector = context.selector, .value = local_index(3)}},
                    .struct_delegate_stages = {slot_stage::full}},
                get_object_ref{.symbol = guard_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(4)},
                storage_rebind{.on_storage = local_index(4), .target_value = local_index(5), .owns_lifetime = true, .lifecycle_phase = phase},
                make_reference{.value_index = local_index(3), .reference_index = local_index(6)},
                struct_type_is{.source = local_index(6), .target_type = base, .result = local_index(7)},
                assert_instr{.condition = local_index(7), .expr_text = "suspension preserves the base phase"},
                struct_phase_leave{.saved_phase = local_index(5)},
                make_reference{.value_index = local_index(3), .reference_index = local_index(9)},
                struct_type_is{.source = local_index(9), .target_type = derived, .result = local_index(8)},
                constexpr_set_result{.target = local_index(8)},
                struct_init_finish{.on_value = local_index(2)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_TRUE(interpreter.get_cr_bool());
    }

    TEST_F(persistent_storage_test, retained_phase_guard_restores_on_exception_but_not_suspension)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}, {.type = address_type{}}};
        state_map current;
        current[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
        current[local_index(2)] = {
            .stage = slot_stage::full, .storage_valid = true,
            .lifecycle_phase = struct_lifecycle_phase{.on_value = local_index(1), .kind = struct_phase_kind::destruction},
            .retained_storage = true};
        EXPECT_TRUE(codegen_state_engine::plan_transition(routine, current, {}, lifetime_transition_kind::retained_exit).empty());
        std::vector< lifetime_action > actions = codegen_state_engine::plan_transition(routine, current, {}, lifetime_transition_kind::exceptional_exit);
        ASSERT_EQ(actions.size(), 3);
        EXPECT_EQ(actions[0].kind, lifetime_action_kind::restore_phase);
        EXPECT_EQ(actions[0].slot, local_index(2));
        EXPECT_EQ(actions[1].kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(actions[2].slot, local_index(1));
        EXPECT_FALSE(actions[0].completed);
        current[local_index(2)].lifecycle_phase = struct_lifecycle_phase{
            .on_value = local_index(1), .kind = struct_phase_kind::construction, .publish_on_normal_exit = true};
        routine.local_types[0].type = integer;
        current[local_index(0)] = {.stage = slot_stage::full, .storage_valid = true};
        state_map completed;
        completed[local_index(1)] = current.at(local_index(1));
        actions = codegen_state_engine::plan_transition(routine, current, completed, lifetime_transition_kind::normal_exit);
        ASSERT_EQ(actions.size(), 3);
        EXPECT_EQ(actions[0].kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(actions[0].slot, local_index(0));
        EXPECT_EQ(actions[1].kind, lifetime_action_kind::restore_phase);
        EXPECT_TRUE(actions[1].completed);
        EXPECT_EQ(actions[2].kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(actions[2].slot, local_index(2));
        lifetime_cleanup_program program(routine);
        lifetime_cleanup_program::cursor cursor{.transition_index = program.append(current, completed, lifetime_transition_kind::normal_exit)};
        for (lifetime_action const& expected : actions)
        {
            std::optional< lifetime_cleanup_program::selected_action > selected = program.next(cursor);
            ASSERT_TRUE(selected.has_value());
            EXPECT_EQ(selected->action.kind, expected.kind);
            EXPECT_EQ(selected->action.slot, expected.slot);
            EXPECT_EQ(selected->action.completed, expected.completed);
        }
        EXPECT_FALSE(program.next(cursor).has_value());
        actions = codegen_state_engine::plan_transition(routine, current, {}, lifetime_transition_kind::exceptional_exit);
        ASSERT_FALSE(actions.empty());
        EXPECT_FALSE(actions.front().completed);
        actions = codegen_state_engine::plan_transition(routine, current, {}, lifetime_transition_kind::control_flow);
        ASSERT_FALSE(actions.empty());
        EXPECT_FALSE(actions.front().completed);
    }

    TEST_F(persistent_storage_test, retained_reservation_survives_an_edge_until_ownership_ends)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}};
        state_map current;
        current[local_index(1)] = {.storage_valid = true, .retained_storage = true};
        EXPECT_TRUE(codegen_state_engine::plan_transition(routine, current, current).empty());
        std::vector< lifetime_action > actions = codegen_state_engine::plan_transition(routine, current, {});
        ASSERT_EQ(actions.size(), 1);
        EXPECT_EQ(actions[0].kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(actions[0].slot, local_index(1));
    }

    TEST_F(persistent_storage_test, nested_owned_members_survive_aggregate_publication)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}, {.type = integer}, {.type = integer}};
        struct_init_selector const field = struct_init_field_selector{.field_ordinal = 0};
        state_map current;
        current[local_index(1)] = {.stage = slot_stage::partial, .storage_valid = true,
            .struct_delegates = std::vector< struct_init_delegate >{{.selector = field, .value = local_index(2)}}};
        current[local_index(2)] = {.stage = slot_stage::partial, .storage_valid = true,
            .struct_delegates = std::vector< struct_init_delegate >{{.selector = field, .value = local_index(3)}},
            .delegate_of = local_index(1), .struct_delegate_selector = field};
        current[local_index(3)] = {.stage = slot_stage::full, .storage_valid = true,
            .nontrivial_dtor = dtor_spec{.func = first_symbol},
            .delegate_of = local_index(2), .struct_delegate_selector = field};
        state_map target;
        target[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
        EXPECT_TRUE(codegen_state_engine::plan_transition(routine, current, target).empty());
    }

    TEST_F(persistent_storage_test, retained_reservation_ends_the_previous_object_lifetime)
    {
        functanoid_routine3 routine;
        routine.local_types = {{.type = void_type{}}, {.type = integer}};
        state_map current;
        current[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true,
            .nontrivial_dtor = dtor_spec{.func = first_symbol}, .retained_storage = true};
        state_map target;
        target[local_index(1)] = {.storage_valid = true, .retained_storage = true};
        std::vector< lifetime_action > const actions = codegen_state_engine::plan_transition(routine, current, target);
        ASSERT_EQ(actions.size(), 2);
        EXPECT_EQ(actions[0].kind, lifetime_action_kind::destroy_object);
        EXPECT_EQ(actions[0].slot, local_index(1));
        EXPECT_EQ(actions[1].kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(actions[1].slot, local_index(1));
    }

    TEST_F(persistent_storage_test, routine_binding_survives_ownership_transfer_and_reuse)
    {
        type_symbol frame_type = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "FRAME"};
        storage payload_storage{.storable_types = {integer}};
        interpreter.add_struct_definition(frame_type, {{.name = "PAYLOAD", .type = payload_storage}});
        interpreter.add_zero_initialized_global(storage_symbol, frame_type);
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = make_mref(payload_storage)}, {.type = integer},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(2), .field_name = "PAYLOAD"},
                storage_init_start{.on_storage = local_index(2), .target_value = local_index(3), .persistent = true, .owns_lifetime = true},
                load_const_int{.target = local_index(3), .value = "42"},
            },
            .terminator = ret{.retain_storage = true},
        });
        type_symbol resume_symbol = instanciation_reference{
            .temploid = {.templexoid = submember{.of = absolute_module_reference{"TEST"}, .name = "RESUME"}},
            .params = {.named = {{"FRAME", make_type_instantiation(make_mref(frame_type))}}},
        };
        functanoid_routine3 resume;
        resume.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = integer}, {.type = make_cref(integer)}, {.type = integer},
        };
        resume.parameters.named["FRAME"] = {.type = make_mref(frame_type), .local_index = local_index(1)};
        resume.retained_bindings.emplace(local_index(2), retained_storage_binding{.frame = local_index(1), .field_name = "PAYLOAD"});
        state_map entry;
        entry[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
        entry[local_index(2)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
        state_map parked;
        parked[local_index(1)] = entry.at(local_index(1));
        slot_state borrowed = entry.at(local_index(2));
        borrowed.persistent_storage = true;
        slot_state accepted = entry.at(local_index(2));
        accepted.destruction_started = true;
        resume.blocks.push_back(executable_block{
            .entry_state = entry,
            .instructions = {
                make_reference{.value_index = local_index(2), .reference_index = local_index(3)},
                load_from_ref{.from_reference = local_index(3), .to_value = local_index(4)},
                constexpr_set_result{.target = local_index(4)},
                lifetime_transfer{.state = parked},
                lifetime_import{.slot = local_index(2), .state = borrowed},
                lifetime_import{.slot = local_index(2)},
                lifetime_transfer{.state = entry},
                make_reference{.value_index = local_index(2), .reference_index = local_index(3)},
                load_from_ref{.from_reference = local_index(3), .to_value = local_index(4)},
                constexpr_set_result{.target = local_index(4)},
                begin_destruction{.of = local_index(2)},
                end_lifetime{.of = local_index(2)},
                lifetime_import{.slot = local_index(2), .state = accepted},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        functanoid_routine3 drive;
        drive.local_types = {{.type = void_type{}}, {.type = make_mref(frame_type)}};
        drive.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                invoke{.what = resume_symbol, .args = {.named = {{"FRAME", local_index(1)}}}},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(resume_symbol, std::move(resume));
        interpreter.add_functanoid3(second_symbol, std::move(drive));
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
    }

    TEST_F(persistent_storage_test, captured_array_alias_transfer_preserves_completed_prefix)
    {
        type_symbol frame_type = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "ARRAY_FRAME"};
        type_symbol array = array_type{.element_type = integer, .element_count = expression_numeric_literal{.value = "2"}};
        type_symbol initializer = array_initializer_type{.element_type = integer};
        storage array_storage{.storable_types = {array}};
        storage initializer_storage{.storable_types = {initializer}};
        storage reference_storage{.storable_types = {make_mref(integer)}};
        interpreter.add_struct_definition(frame_type, {
            {.name = "ARRAY", .type = array_storage}, {.name = "INITIALIZER", .type = initializer_storage},
            {.name = "ELEMENT", .type = reference_storage}});
        interpreter.add_zero_initialized_global(storage_symbol, frame_type);
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = make_mref(array_storage)}, {.type = array},
            {.type = make_mref(initializer_storage)}, {.type = initializer}, {.type = integer},
            {.type = make_mref(reference_storage)}, {.type = make_mref(integer)},
        };
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(2), .field_name = "ARRAY"},
                storage_init_start{.on_storage = local_index(2), .target_value = local_index(3), .persistent = true, .owns_lifetime = true},
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(4), .field_name = "INITIALIZER"},
                storage_init_start{.on_storage = local_index(4), .target_value = local_index(5), .persistent = true, .owns_lifetime = true},
                array_init_start{.source = local_index(3), .initializer = local_index(5)},
                array_init_element{.initializer = local_index(5), .target = local_index(6)},
                load_const_int{.target = local_index(6), .value = "42"},
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(7), .field_name = "ELEMENT"},
                storage_init_start{.on_storage = local_index(7), .target_value = local_index(8), .persistent = true, .owns_lifetime = true},
                make_reference{.value_index = local_index(6), .reference_index = local_index(8)},
            },
            .terminator = ret{.retain_storage = true},
        });
        type_symbol resume_symbol = instanciation_reference{
            .temploid = {.templexoid = submember{.of = absolute_module_reference{"TEST"}, .name = "RESUME_ARRAY"}},
            .params = {.named = {{"FRAME", make_type_instantiation(make_mref(frame_type))}}},
        };
        functanoid_routine3 resume;
        resume.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = array}, {.type = initializer},
            {.type = integer}, {.type = make_mref(integer)}, {.type = integer}, {.type = make_cref(integer)},
        };
        resume.parameters.named["FRAME"] = {.type = make_mref(frame_type), .local_index = local_index(1)};
        resume.retained_bindings.emplace(local_index(2), retained_storage_binding{.frame = local_index(1), .field_name = "ARRAY"});
        resume.retained_bindings.emplace(local_index(3), retained_storage_binding{.frame = local_index(1), .field_name = "INITIALIZER"});
        resume.retained_bindings.emplace(local_index(4), retained_storage_binding{
            .frame = local_index(1), .field_name = "ELEMENT", .kind = retained_storage_kind::captured_alias_reference, .saved_reference = local_index(5)});
        resume.retained_bindings.emplace(local_index(5), retained_storage_binding{.frame = local_index(1), .field_name = "ELEMENT"});
        state_map entry;
        entry[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
        entry[local_index(2)] = {.stage = slot_stage::partial, .storage_valid = true,
            .array_delegates = invocation_args{.named = {{"INIT", local_index(3)}}}, .retained_storage = true};
        entry[local_index(3)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
        entry[local_index(4)] = {.stage = slot_stage::full, .storage_valid = true,
            .retained_storage = true, .array_delegate_of_initializer = local_index(3)};
        entry[local_index(5)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
        state_map parked;
        parked[local_index(1)] = entry.at(local_index(1));
        resume.blocks.push_back(executable_block{
            .entry_state = entry,
            .instructions = {
                lifetime_transfer{.state = parked},
                lifetime_import{.slot = local_index(3), .state = entry.at(local_index(3))},
                lifetime_import{.slot = local_index(5), .state = entry.at(local_index(5))},
                lifetime_import{.slot = local_index(4), .state = entry.at(local_index(4))},
                make_reference{.value_index = local_index(4), .reference_index = local_index(7)},
                array_init_index{.initializer = local_index(3), .result = local_index(6)},
                constexpr_set_result{.target = local_index(6)},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 drive;
        drive.local_types = {{.type = void_type{}}, {.type = make_mref(frame_type)}};
        drive.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                invoke{.what = resume_symbol, .args = {.named = {{"FRAME", local_index(1)}}}},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(resume_symbol, std::move(resume));
        interpreter.add_functanoid3(second_symbol, std::move(drive));
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 1);
    }

    TEST_F(persistent_storage_test, borrowed_destruction_retires_expired_payload_without_reopening_it)
    {
        type_symbol frame_type = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "BORROWED_FRAME"};
        storage payload_storage{.storable_types = {integer}};
        storage reference_storage{.storable_types = {make_mref(integer)}};
        for (bool reopen : {false, true})
        {
            ir2_constexpr_interpreter machine;
            machine.add_struct_definition(frame_type, {{.name = "PAYLOAD", .type = payload_storage}, {.name = "REFERENCE", .type = reference_storage}});
            machine.add_zero_initialized_global(storage_symbol, frame_type);
            functanoid_routine3 initialize;
            initialize.local_types = {
                {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = make_mref(payload_storage)}, {.type = integer},
                {.type = make_mref(reference_storage)}, {.type = make_mref(integer)},
            };
            initialize.blocks.push_back(executable_block{
                .instructions = {
                    get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                    access_field{.base_index = local_index(1), .store_index = local_index(2), .field_name = "PAYLOAD"},
                    storage_init_start{.on_storage = local_index(2), .target_value = local_index(3), .persistent = true, .owns_lifetime = true},
                    load_const_int{.target = local_index(3), .value = "42"},
                    get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                    access_field{.base_index = local_index(1), .store_index = local_index(4), .field_name = "REFERENCE"},
                    storage_init_start{.on_storage = local_index(4), .target_value = local_index(5), .persistent = true, .owns_lifetime = true},
                    make_reference{.value_index = local_index(3), .reference_index = local_index(5)},
                },
                .terminator = ret{.retain_storage = true},
            });
            type_symbol resume_symbol = instanciation_reference{
                .temploid = {.templexoid = submember{.of = absolute_module_reference{"TEST"}, .name = "DESTROY_BORROWED"}},
                .params = {.named = {{"FRAME", make_type_instantiation(make_mref(frame_type))}}},
            };
            type_symbol destroy_symbol = instanciation_reference{
                .temploid = {.templexoid = submember{.of = absolute_module_reference{"TEST"}, .name = "DESTROY_VALUE"}},
                .params = {.named = {{"THIS", make_type_instantiation(dvalue_slot{.target = integer})}}},
            };
            functanoid_routine3 destruction;
            destruction.local_types = {{.type = void_type{}}, {.type = integer}};
            destruction.parameters.named["THIS"] = {.type = dvalue_slot{.target = integer}, .local_index = local_index(1)};
            state_map destruction_entry;
            destruction_entry[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
            destruction.blocks.push_back(executable_block{
                .entry_state = destruction_entry,
                .instructions = {end_lifetime{.of = local_index(1)}}, .terminator = ret{}});
            functanoid_routine3 resume;
            resume.local_types = {
                {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = integer},
                {.type = make_mref(integer)}, {.type = make_mref(integer)}};
            resume.parameters.named["FRAME"] = {.type = make_mref(frame_type), .local_index = local_index(1)};
            resume.retained_bindings.emplace(local_index(2), retained_storage_binding{
                .frame = local_index(1), .field_name = "REFERENCE", .kind = retained_storage_kind::reference});
            resume.retained_bindings.emplace(local_index(3), retained_storage_binding{.frame = local_index(1), .field_name = "REFERENCE"});
            state_map entry;
            entry[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
            entry[local_index(2)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
            entry[local_index(3)] = {.stage = slot_stage::full, .storage_valid = true, .retained_storage = true};
            state_map retired = entry;
            retired[local_index(2)].stage = reopen ? slot_stage::full : slot_stage::dead;
            state_map accepted = entry;
            accepted[local_index(2)].destruction_started = true;
            resume.blocks.push_back(executable_block{
                .entry_state = entry,
                .instructions = {
                    begin_destruction{.of = local_index(2)},
                    make_reference{.value_index = local_index(2), .reference_index = local_index(4)},
                    invoke{.what = destroy_symbol, .args = {.named = {{"THIS", local_index(4)}}}},
                },
                .terminator = jump{.target = block_index(1)},
            });
            resume.blocks.push_back(executable_block{
                .entry_state = accepted,
                .instructions = {
                    end_lifetime{.of = local_index(2)},
                    lifetime_transfer{.state = retired},
                    end_lifetime{.of = local_index(2)},
                    end_lifetime{.of = local_index(3)},
                },
                .terminator = ret{},
            });
            functanoid_routine3 drive;
            drive.local_types = {{.type = void_type{}}, {.type = make_mref(frame_type)}};
            drive.blocks.push_back(executable_block{
                .instructions = {
                    get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                    invoke{.what = resume_symbol, .args = {.named = {{"FRAME", local_index(1)}}}},
                },
                .terminator = ret{},
            });
            machine.add_functanoid3(first_symbol, std::move(initialize));
            machine.add_functanoid3(destroy_symbol, std::move(destruction));
            machine.add_functanoid3(resume_symbol, std::move(resume));
            machine.add_functanoid3(second_symbol, std::move(drive));
            machine.exec3(first_symbol);
            if (reopen) EXPECT_THROW(machine.exec3(second_symbol), constexpr_logic_execution_error);
            else
            {
                EXPECT_NO_THROW(machine.exec3(second_symbol));
                EXPECT_NO_THROW(machine.exec3(first_symbol));
            }
        }
    }

    TEST_F(persistent_storage_test, terminating_exception_boundary_preserves_occupied_capture_until_panic)
    {
        type_symbol exception_pointer = subsymbol{.of = absolute_module_reference{"RUNTIME"}, .name = "EXCEPTION_PTR"};
        type_symbol frame_pointer = ptrref_type{.target = integer, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        interpreter.add_struct_definition(exception_pointer, {{.name = "frame", .type = frame_pointer}});
        functanoid_routine3 routine;
        routine.local_types = {
            {.type = void_type{}}, {.type = integer}, {.type = exception_pointer},
            {.type = frame_pointer}, {.type = frame_pointer}};
        routine.blocks.push_back(executable_block{
            .instructions = {
                load_const_int{.target = local_index(1), .value = "42"},
                struct_init_start{.on_value = local_index(2), .delegates = {
                    {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)}}},
                make_pointer_to{.of_index = local_index(1), .pointer_index = local_index(3)},
                struct_init_finish{.on_value = local_index(2)},
                make_pointer_to{.of_index = local_index(1), .pointer_index = local_index(4)},
            },
            .terminator = throw_exception{.frame = local_index(4)},
            .catcher = exception_catcher{.exception = local_index(2), .handler = block_index(1), .terminate_before_capture = true},
        });
        routine.blocks.push_back(executable_block{.terminator = panic{.message = "terminating exception boundary reached"}});
        interpreter.add_functanoid3(first_symbol, std::move(routine));
        try
        {
            interpreter.exec3(first_symbol);
            FAIL() << "The terminating exception boundary did not reach PANIC";
        }
        catch (constexpr_panic_failure const& failure)
        {
            EXPECT_EQ(std::string(failure.what()), "During constexpr evaluation: PANIC: terminating exception boundary reached");
        }
    }

    TEST_F(persistent_storage_test, borrowed_retained_views_preserve_partial_storage_and_phase_ownership)
    {
        type_symbol record = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "BORROWED_RECORD"};
        type_symbol frame_type = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "BORROWED_PHASE_FRAME"};
        storage object_storage{.storable_types = {record}};
        storage phase_storage{.storable_types = {address_type{}}};
        interpreter.add_struct_definition(record, {{.name = "VALUE", .type = integer}});
        interpreter.add_struct_definition(frame_type, {{.name = "OBJECT", .type = object_storage}, {.name = "PHASE", .type = phase_storage}});
        interpreter.add_zero_initialized_global(storage_symbol, frame_type);
        functanoid_routine3 initialize;
        initialize.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = make_mref(object_storage)},
            {.type = record}, {.type = integer}, {.type = make_mref(phase_storage)}, {.type = address_type{}}};
        initialize.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(2), .field_name = "OBJECT"},
                storage_init_start{.on_storage = local_index(2), .target_value = local_index(3), .persistent = true, .owns_lifetime = true},
                struct_init_start{.on_value = local_index(3), .delegates = {
                    {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(4)}}},
                load_const_int{.target = local_index(4), .value = "42"},
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                access_field{.base_index = local_index(1), .store_index = local_index(5), .field_name = "PHASE"},
                storage_init_start{.on_storage = local_index(5), .target_value = local_index(6), .persistent = true, .owns_lifetime = true},
                struct_phase_enter{.on_value = local_index(3), .saved_phase = local_index(6), .kind = struct_phase_kind::construction},
            },
            .terminator = ret{.retain_storage = true},
        });
        functanoid_routine3 complete;
        complete.local_types = {
            {.type = void_type{}}, {.type = make_mref(frame_type)}, {.type = record}, {.type = address_type{}},
            {.type = record}, {.type = address_type{}}, {.type = make_cref(record)}, {.type = make_cref(integer)}, {.type = integer}};
        complete.retained_bindings.emplace(local_index(2), retained_storage_binding{.frame = local_index(1), .field_name = "OBJECT"});
        complete.retained_bindings.emplace(local_index(3), retained_storage_binding{.frame = local_index(1), .field_name = "PHASE"});
        complete.retained_bindings.emplace(local_index(4), retained_storage_binding{.frame = local_index(1), .field_name = "OBJECT"});
        complete.retained_bindings.emplace(local_index(5), retained_storage_binding{.frame = local_index(1), .field_name = "PHASE"});
        slot_state object{.stage = slot_stage::partial, .storage_valid = true, .retained_storage = true};
        slot_state phase{.stage = slot_stage::full, .storage_valid = true,
            .lifecycle_phase = struct_lifecycle_phase{.on_value = local_index(2), .kind = struct_phase_kind::construction}, .retained_storage = true};
        slot_state borrowed_object{.stage = slot_stage::dead, .storage_valid = true, .construction_pending = true,
            .is_projection = true, .persistent_storage = true, .retained_storage = true};
        slot_state borrowed_phase{.stage = slot_stage::full, .storage_valid = true,
            .is_projection = true, .persistent_storage = true, .retained_storage = true};
        complete.blocks.push_back(executable_block{
            .instructions = {
                get_object_ref{.symbol = storage_symbol, .type = access_type::object, .class_ = access_class::global, .target_ref = local_index(1)},
                lifetime_import{.slot = local_index(2), .state = object},
                lifetime_import{.slot = local_index(3), .state = phase},
                lifetime_import{.slot = local_index(4), .state = borrowed_object},
                lifetime_import{.slot = local_index(5), .state = borrowed_phase},
                lifetime_import{.slot = local_index(4)},
                lifetime_import{.slot = local_index(5)},
                struct_phase_leave{.saved_phase = local_index(3), .completed = true},
                struct_init_finish{.on_value = local_index(2)},
                make_reference{.value_index = local_index(2), .reference_index = local_index(6)},
                access_field{.base_index = local_index(6), .store_index = local_index(7), .field_name = "VALUE"},
                load_from_ref{.from_reference = local_index(7), .to_value = local_index(8)},
                constexpr_set_result{.target = local_index(8)},
                end_lifetime{.of = local_index(2)},
            },
            .terminator = ret{},
        });
        interpreter.add_functanoid3(first_symbol, std::move(initialize));
        interpreter.add_functanoid3(second_symbol, std::move(complete));
        interpreter.exec3(first_symbol);
        interpreter.exec3(second_symbol);
        EXPECT_EQ(interpreter.get_cr_u64(), 42);
        EXPECT_NO_THROW(interpreter.exec3(first_symbol));
    }

    TEST_F(persistent_storage_test, partial_aggregate_publication_requires_every_initialized_member)
    {
        type_symbol record = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "PARTIAL"};
        type_symbol nested = subsymbol{.of = absolute_module_reference{"TEST"}, .name = "NESTED"};
        storage storage_type{.storable_types = {record}};
        for (bool complete : {false, true})
        {
            ir2_constexpr_interpreter machine;
            machine.add_struct_definition(record, {{.name = "NESTED", .type = nested}});
            machine.add_struct_definition(nested, {{.name = "VALUE", .type = integer}});
            functanoid_routine3 initialize;
            initialize.local_types = {
                {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = record}, {.type = nested}, {.type = integer},
            };
            initialize.blocks.push_back(executable_block{
                .instructions = {
                    get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                    storage_init_start{.on_storage = local_index(1), .target_value = local_index(2), .persistent = true, .owns_lifetime = true},
                    struct_init_start{.on_value = local_index(2), .delegates = {
                        {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(3)}}},
                    struct_init_start{.on_value = local_index(3), .delegates = {
                        {.selector = struct_init_field_selector{.field_ordinal = 0}, .value = local_index(4)}}},
                },
                .terminator = ret{.retain_storage = true},
            });
            if (complete) initialize.blocks[0].instructions.push_back(load_const_int{.target = local_index(4), .value = "42"});
            functanoid_routine3 publish;
            publish.local_types = {
                {.type = void_type{}}, {.type = make_mref(storage_type)}, {.type = record}, {.type = make_cref(record)},
                {.type = make_cref(nested)}, {.type = make_cref(integer)}, {.type = integer},
            };
            publish.blocks.push_back(executable_block{
                .instructions = {
                    get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(1)},
                    storage_rebind{.on_storage = local_index(1), .target_value = local_index(2), .stage = slot_stage::full,
                        .owns_lifetime = true, .construction_pending = true},
                    complete_initialization{.of = local_index(2), .finalize_partial = true},
                    make_reference{.value_index = local_index(2), .reference_index = local_index(3)},
                    access_field{.base_index = local_index(3), .store_index = local_index(4), .field_name = "NESTED"},
                    access_field{.base_index = local_index(4), .store_index = local_index(5), .field_name = "VALUE"},
                    load_from_ref{.from_reference = local_index(5), .to_value = local_index(6)},
                    constexpr_set_result{.target = local_index(6)},
                    end_lifetime{.of = local_index(2)},
                },
                .terminator = ret{},
            });
            machine.add_functanoid3(first_symbol, std::move(initialize));
            machine.add_functanoid3(second_symbol, std::move(publish));
            machine.exec3(first_symbol);
            if (complete)
            {
                machine.exec3(second_symbol);
                EXPECT_EQ(machine.get_cr_u64(), 42);
            }
            else EXPECT_THROW(machine.exec3(second_symbol), constexpr_logic_execution_error);
        }
    }

    TEST_F(persistent_storage_test, reference_construction_uses_reserved_storage_without_overwriting_live_values)
    {
        type_symbol const reference_type = make_cref(integer);
        storage const reference_storage{.storable_types = {reference_type}};
        type_symbol const copied_storage = submember{.of = absolute_module_reference{"TEST"}, .name = "COPY"};
        for (unsigned repeated_output = 0; repeated_output != 3; ++repeated_output)
        {
            ir2_constexpr_interpreter machine;
            functanoid_routine3 routine;
            routine.local_types = {
                {.type = void_type{}}, {.type = integer}, {.type = make_mref(reference_storage)}, {.type = reference_type},
                {.type = make_mref(reference_storage)}, {.type = reference_type}, {.type = reference_type}, {.type = integer},
            };
            routine.blocks.push_back(executable_block{
                .instructions = {
                    load_const_int{.target = local_index(1), .value = "42"},
                    get_object_ref{.symbol = storage_symbol, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(2)},
                    storage_init_start{.on_storage = local_index(2), .target_value = local_index(3), .persistent = true, .owns_lifetime = true},
                    make_reference{.value_index = local_index(1), .reference_index = local_index(3)},
                    get_object_ref{.symbol = copied_storage, .type = access_type::storage, .class_ = access_class::global, .target_ref = local_index(4)},
                    storage_init_start{.on_storage = local_index(4), .target_value = local_index(5), .persistent = true, .owns_lifetime = true},
                    copy_reference{.from_index = local_index(3), .to_index = local_index(5)},
                },
                .terminator = ret{},
            });
            std::vector< vm_instruction >& instructions = routine.blocks[0].instructions;
            if (repeated_output == 1) instructions.push_back(make_reference{.value_index = local_index(1), .reference_index = local_index(3)});
            if (repeated_output == 2) instructions.push_back(copy_reference{.from_index = local_index(3), .to_index = local_index(5)});
            instructions.push_back(copy_reference{.from_index = local_index(5), .to_index = local_index(6)});
            instructions.push_back(load_from_ref{.from_reference = local_index(6), .to_value = local_index(7)});
            instructions.push_back(constexpr_set_result{.target = local_index(7)});
            instructions.push_back(end_lifetime{.of = local_index(5)});
            instructions.push_back(end_lifetime{.of = local_index(3)});
            machine.add_functanoid3(first_symbol, std::move(routine));
            if (repeated_output == 0)
            {
                machine.exec3(first_symbol);
                EXPECT_EQ(machine.get_cr_u64(), 42);
            }
            else EXPECT_THROW(machine.exec3(first_symbol), compiler_bug);
        }
    }

    TEST_F(persistent_storage_test, asynchronous_output_cleanup_requires_completed_ownership)
    {
        functanoid_routine3 routine;
        local_index const output(1);
        routine.local_types = {{.type = void_type{}}, {.type = integer}};
        routine.parameters.named.emplace("RETURN", routine_parameter{.type = nvalue_slot{integer}, .local_index = output});
        routine.asynchronous_cleanup_slots.insert(output);

        state_map initialized{{output, slot_state{.stage = slot_stage::full, .storage_valid = true}}};
        state_map transferred;
        codegen_state_engine(transferred, routine.local_types, routine.parameters).apply_normal_exit();
        EXPECT_TRUE(codegen_state_engine::plan_transition(routine, initialized, transferred, lifetime_transition_kind::normal_exit).empty());

        state_map abandoned;
        codegen_state_engine(abandoned, routine.local_types, routine.parameters).apply_exception_exit();
        std::vector< lifetime_action > actions = codegen_state_engine::plan_transition(routine, initialized, abandoned, lifetime_transition_kind::exceptional_exit);
        ASSERT_EQ(actions.size(), 2);
        EXPECT_EQ(actions.front().kind, lifetime_action_kind::require_asynchronous_cleanup);
        EXPECT_EQ(actions.front().slot, output);

        initialized[output].stage = slot_stage::partial;
        actions = codegen_state_engine::plan_transition(routine, initialized, abandoned, lifetime_transition_kind::exceptional_exit);
        ASSERT_EQ(actions.size(), 1);
        EXPECT_EQ(actions.front().kind, lifetime_action_kind::end_lifetime);
    }

    TEST_F(persistent_storage_test, pending_destination_survives_internal_edges_and_rolls_back_on_failure)
    {
        functanoid_routine3 routine;
        local_index const output(1);
        routine.local_types = {{.type = void_type{}}, {.type = integer}};
        routine.non_trivial_dtors.emplace(integer, first_symbol);
        state_map completed{{output, slot_state{.stage = slot_stage::full, .storage_valid = true, .construction_pending = true, .retained_storage = true}}};
        state_map pending{{output, slot_state{.storage_valid = true, .construction_pending = true, .retained_storage = true}}};
        EXPECT_TRUE(codegen_state_engine::plan_transition(routine, completed, pending).empty());
        std::vector< lifetime_action > actions = codegen_state_engine::plan_transition(routine, completed, {}, lifetime_transition_kind::exceptional_exit);
        ASSERT_EQ(actions.size(), 2);
        EXPECT_EQ(actions.front().kind, lifetime_action_kind::destroy_object);
        EXPECT_EQ(actions.front().slot, output);
        EXPECT_EQ(actions.back().kind, lifetime_action_kind::end_lifetime);
    }

}
