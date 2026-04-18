// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_id.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/source_file_name.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_antestatal.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include "graph_dump_test_utils.hpp"

#include <cstddef>
#include <variant>

namespace
{
    auto make_test_source_bundle() -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle;

        quxlang::target_configuration x64;
        x64.target_output_config.cpu_type = quxlang::cpu::x86_64;
        x64.target_output_config.os_type = quxlang::os::linux;
        x64.target_output_config.binary_type = quxlang::binary::elf;
        x64.module_configurations["main"].source = "main_x64";
        x64.module_configurations["util"].source = "util_shared";

        quxlang::target_configuration arm64;
        arm64.target_output_config.cpu_type = quxlang::cpu::arm_64;
        arm64.target_output_config.os_type = quxlang::os::macos;
        arm64.target_output_config.binary_type = quxlang::binary::macho;
        arm64.module_configurations["main"].source = "main_arm64";
        arm64.module_configurations["util"].source = "util_shared";

        bundle.targets["x64"] = x64;
        bundle.targets["arm64"] = arm64;

        bundle.module_sources["main_x64"].files["main.qx"] = quxlang::source_file{.contents = "::main VAR I32;"};
        bundle.module_sources["main_arm64"].files["main.qx"] = quxlang::source_file{.contents = "::main VAR I64;"};
        bundle.module_sources["util_shared"].files["util.qx"] = quxlang::source_file{.contents = "::util VAR I32;"};

        return bundle;
    }

    auto make_single_main_source_bundle(std::string contents) -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle;

        quxlang::target_configuration x64;
        x64.target_output_config.cpu_type = quxlang::cpu::x86_64;
        x64.target_output_config.os_type = quxlang::os::linux;
        x64.target_output_config.binary_type = quxlang::binary::elf;
        x64.module_configurations["main"].source = "main_x64";

        bundle.targets["x64"] = x64;
        bundle.module_sources["main_x64"].files["main.qx"] = quxlang::source_file{.contents = std::move(contents)};

        return bundle;
    }

    auto make_x64_graph(quxlang::source_bundle const& bundle) -> quxlang::compiler_querygraph
    {
        return quxlang::compiler_querygraph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                            quxlang::tests::current_test_graph_dump_path());
    }

    /// Builds an I32 constexpr value with the requested low byte.
    auto test_i32_value(std::byte low_byte) -> quxlang::constexpr_value
    {
        return quxlang::antestatal_value(quxlang::antestatal_primitive{.value = {low_byte, std::byte{0}, std::byte{0}, std::byte{0}}});
    }

    /// Checks that a constexpr value is the expected I32 primitive.
    auto assert_i32_value(quxlang::constexpr_value const& value, std::byte low_byte) -> void
    {
        auto const& antestatal = quxlang::constexpr_value_as_antestatal(value);
        ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(antestatal));
        ASSERT_EQ(quxlang::as< quxlang::antestatal_primitive >(antestatal).value, (std::vector{low_byte, std::byte{0}, std::byte{0}, std::byte{0}}));
    }
} // namespace

TEST(querygraph_queries, source_bundle_returns_injected_bundle)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::source_bundle_query >(std::monostate{});

    ASSERT_TRUE(resolved.targets.contains("x64"));
    ASSERT_TRUE(resolved.targets.contains("arm64"));
    ASSERT_TRUE(resolved.module_sources.contains("main_x64"));
    ASSERT_EQ(resolved.module_sources.at("main_x64").files.at("main.qx").get().contents, "::main VAR I32;");
}

TEST(querygraph_queries, machine_info_returns_injected_output_info)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::machine_info_query >(std::monostate{});

    ASSERT_EQ(resolved.cpu_type, quxlang::cpu::x86_64);
    ASSERT_EQ(resolved.os_type, quxlang::os::linux);
    ASSERT_EQ(resolved.binary_type, quxlang::binary::elf);
}

TEST(querygraph_queries, module_source_name_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph x64_graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());

    quxlang::compiler_querygraph arm64_graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());

    ASSERT_EQ(x64_graph.make_request< quxlang::module_source_name_query >("main"), "main_x64");
    ASSERT_EQ(arm64_graph.make_request< quxlang::module_source_name_query >("main"), "main_arm64");
}

TEST(querygraph_queries, module_source_name_map_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::module_source_name_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at("main"), "main_arm64");
    ASSERT_EQ(resolved.at("util"), "util_shared");
}

TEST(querygraph_queries, module_options_map_uses_configured_target)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    auto graph = make_x64_graph(bundle);
    auto answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});

    auto resolved = graph.make_request< quxlang::module_options_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at(answer), "7");
}

TEST(querygraph_queries, module_options_map_resolves_overloaded_option_symbols)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::holder CLASS { .answer OPTION NUMBER DEFAULT_VALUE(5); }");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    bundle.targets.at("x64").module_configurations.at("main").option_values["holder::.answer"] = "9";
    auto graph = make_x64_graph(bundle);
    auto module_answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});
    auto holder = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "holder"});
    auto holder_answer = quxlang::type_symbol(quxlang::submember{holder, "answer"});

    auto resolved = graph.make_request< quxlang::module_options_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at(module_answer), "7");
    ASSERT_EQ(resolved.at(holder_answer), "9");
}

TEST(querygraph_queries, module_sources_returns_source_files_for_logical_module)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::module_sources_query >("util");

    ASSERT_TRUE(resolved.files.contains("util.qx"));
    ASSERT_EQ(resolved.files.at("util.qx").get().contents, "::util VAR I32;");
}

TEST(querygraph_queries, source_file_index_assigns_deterministic_global_ids)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::source_file_index_query >(std::monostate{});

    ASSERT_EQ(resolved.file_to_id.size(), 3);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qx"}), 0);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qx"}), 1);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "util_shared", .relative_path = "util.qx"}), 2);
}

TEST(querygraph_queries, source_file_id_is_unique_across_modules)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto arm_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qx"});
    auto x64_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qx"});

    ASSERT_NE(arm_id, x64_id);
}

TEST(querygraph_queries, source_file_name_round_trips_from_id)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::source_file_name input{.source_module = "util_shared", .relative_path = "util.qx"};

    auto id = graph.make_request< quxlang::source_file_id_query >(input);
    auto output = graph.make_request< quxlang::source_file_name_query >(id);

    ASSERT_EQ(output, input);
}

TEST(querygraph_queries, option_declaration_resolves_as_option_symbol)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    auto graph = make_x64_graph(bundle);
    auto answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(answer), quxlang::symbol_kind::option);
}

TEST(querygraph_queries, option_number_and_bool_values_are_constexpr_literals)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::enabled OPTION BOOL DEFAULT_VALUE(FALSE);");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    bundle.targets.at("x64").module_configurations.at("main").option_values["enabled"] = "true";
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"answer"}},
        .context = context,
    });
    auto enabled = graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"enabled"}},
        .context = context,
    });

    ASSERT_EQ(answer, 7);
    ASSERT_TRUE(enabled);
}

TEST(querygraph_queries, constexpr_eval_v3_discards_primary_result_when_no_type_expected)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(quxlang::constexpr_input_v3{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = std::nullopt,
    });

    ASSERT_FALSE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_FALSE(result.deduced_type.has_value());
}

TEST(querygraph_queries, constexpr_eval_v3_auto_reports_deduced_primary_type)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(quxlang::constexpr_input_v3{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = quxlang::auto_temploidic{},
    });

    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_EQ(result.deduced_type, std::optional< quxlang::type_symbol >(quxlang::bool_type{}));
}

TEST(querygraph_queries, constexpr_eval_v3_scoped_typedef_resolves_type_expression)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_same_types{.lhs_type = quxlang::freebound_identifier{"T"}, .rhs_type = i32},
        .context = context,
        .expected_result_type = quxlang::bool_type{},
    };
    input.scoped_definitions["T"] = quxlang::scoped_typedef{.type = i32};

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    auto const& value = quxlang::constexpr_value_as_antestatal(result.values.at(quxlang::constexpr_primary_result_id));
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(value));
    ASSERT_EQ(quxlang::as< quxlang::antestatal_primitive >(value).value, (std::vector{std::byte{1}}));
}

TEST(querygraph_queries, constexpr_eval_v3_scoped_static_resolves_localdata)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto static_symbol = quxlang::static_local_ref{.functanoid = context, .name = "x", .generation = 1};

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"x"}},
        .context = context,
        .expected_result_type = i32,
    };
    input.scoped_definitions["x"] = quxlang::scoped_static{.symbol = static_symbol};
    input.statics[static_symbol] = quxlang::constexpr_static{
        .type = i32,
        .value = test_i32_value(std::byte{7}),
        .mutation_result_id = std::nullopt,
    };

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    assert_i32_value(result.values.at(quxlang::constexpr_primary_result_id), std::byte{7});
}

TEST(querygraph_queries, constexpr_eval_v3_static_mutation_result_id_returns_static_value)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto static_symbol = quxlang::static_local_ref{.functanoid = context, .name = "x", .generation = 1};

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = std::nullopt,
    };
    input.statics[static_symbol] = quxlang::constexpr_static{
        .type = i32,
        .value = test_i32_value(std::byte{9}),
        .mutation_result_id = 17,
    };

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_FALSE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_TRUE(result.values.contains(17));
    assert_i32_value(result.values.at(17), std::byte{9});
}

TEST(querygraph_queries, option_number_uses_default_when_unconfigured)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"answer"}},
        .context = context,
    });

    ASSERT_EQ(answer, 4);
}

TEST(querygraph_queries, option_string_value_is_codegen_literal)
{
    auto bundle = make_single_main_source_bundle("::message OPTION STRING DEFAULT_VALUE(\"fallback\");");
    bundle.targets.at("x64").module_configurations.at("main").option_values["message"] = "configured";
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto routine = graph.make_request< quxlang::constexpr_routine_query >(quxlang::constexpr_input2{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"message"}},
        .context = context,
        .type = quxlang::readonly_constant{.kind = quxlang::constant_kind::string},
    });

    std::vector< std::byte > expected;
    for (char const c : std::string("configured"))
    {
        expected.push_back(static_cast< std::byte >(c));
    }

    bool found_string_load = false;
    for (auto const& block : routine.blocks)
    {
        for (auto const& instruction : block.instructions)
        {
            if (quxlang::typeis< quxlang::vmir2::load_const_value >(instruction))
            {
                found_string_load = found_string_load || quxlang::as< quxlang::vmir2::load_const_value >(instruction).value == expected;
            }
        }
    }

    ASSERT_TRUE(found_string_load);
}

TEST(querygraph_queries, constexpr_routine_generated_ir_inherits_expression_location)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::source_location loc{.file_id = 7, .begin_index = 3, .end_index = std::optional< std::size_t >{5}};
    quxlang::expression expr = quxlang::expression_numeric_literal{.value = "42", .location = loc};

    auto routine = graph.make_request< quxlang::constexpr_routine_query >(quxlang::constexpr_input2{
        .expr = expr,
        .context = context,
        .type = quxlang::int_type{.bits = 64, .has_sign = false},
    });

    auto expect_location = [&](std::optional< quxlang::source_location > location)
    {
        ASSERT_TRUE(location.has_value());
        ASSERT_EQ(location->file_id, loc.file_id);
        ASSERT_EQ(location->begin_index, loc.begin_index);
        ASSERT_EQ(location->end_index, loc.end_index);
    };

    bool found_instruction = false;
    bool found_terminator = false;
    for (auto const& block : routine.blocks)
    {
        for (auto const& instruction : block.instructions)
        {
            expect_location(quxlang::vmir2::get_location(instruction));
            found_instruction = true;
        }

        if (block.terminator.has_value())
        {
            expect_location(quxlang::vmir2::get_location(*block.terminator));
            found_terminator = true;
        }
    }

    ASSERT_TRUE(found_instruction);
    ASSERT_TRUE(found_terminator);
}

TEST(querygraph_queries, option_default_from_copies_another_option)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::holder CLASS { .answer OPTION NUMBER DEFAULT_FROM(::answer); }");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "11";
    auto graph = make_x64_graph(bundle);
    auto holder = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "holder"});
    auto holder_answer = quxlang::type_symbol(quxlang::submember{holder, "answer"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{holder_answer},
        .context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"}),
    });

    ASSERT_EQ(answer, 11);
}

TEST(querygraph_queries, type_is_antestatal_accepts_conservative_shapes)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);

    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    quxlang::procedure_type proc;
    auto proc_type = quxlang::type_symbol(proc);
    auto proc_ptr = quxlang::type_symbol(quxlang::ptrref_type{.target = proc_type, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut});
    auto i32_array = quxlang::type_symbol(quxlang::array_type{.element_type = i32, .element_count = quxlang::expression_numeric_literal{"4"}});
    auto storage_type = quxlang::type_symbol(quxlang::storage{.storable_types = {i32}});

    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(i32));
    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(i32_array));
    ASSERT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(storage_type));
    ASSERT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(proc_type));
    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(proc_ptr));
}

TEST(querygraph_queries, global_static_i32_is_antestatal_and_materializes_value)
{
    auto bundle = make_single_main_source_bundle("::foo STATIC I32 := 4;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    ASSERT_TRUE(graph.make_request< quxlang::global_is_antestatal_static_query >(foo));

    auto value = graph.make_request< quxlang::antestatal_static_value_query >(foo);
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(value));
    auto const& bytes = quxlang::as< quxlang::antestatal_primitive >(value).value;
    ASSERT_GE(bytes.size(), 1);
    ASSERT_EQ(std::to_integer< std::uint32_t >(bytes.at(0)), 4);

}

TEST(querygraph_queries, nonstatic_global_is_not_antestatal_static)
{
    auto bundle = make_single_main_source_bundle("::foo VAR I32 := 4;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    ASSERT_FALSE(graph.make_request< quxlang::global_is_antestatal_static_query >(foo));
}
