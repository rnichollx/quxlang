// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/argument_adaptation_is_better_fit.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/global_is_serialoid_static.hpp>
#include <quxlang/queries/implementation_function_map.hpp>
#include <quxlang/queries/implementation_interface_type.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/output_list.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/interface_defaultable.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_id.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/source_file_name.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/type_is_antestatal.hpp>
#include <quxlang/queries/type_is_serialoid.hpp>
#include <quxlang/queries/type_is_stringlike.hpp>
#include <quxlang/queries/type_is_trivially_default_constructible.hpp>
#include <quxlang/queries/type_placement_info.hpp>
#include <quxlang/queries/user_deserialize_exists.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/vmir2/assembler.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include "graph_dump_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
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
        x64.outputs = std::map< std::string, quxlang::output_config >{
            {"app", quxlang::output_config{.type = quxlang::output_kind::executable, .module = "main", .main_functanoid = "main"}},
            {"util", quxlang::output_config{.type = quxlang::output_kind::shared_library, .module = "util", .main_functanoid = std::nullopt}},
        };

        quxlang::target_configuration arm64;
        arm64.target_output_config.cpu_type = quxlang::cpu::arm_64;
        arm64.target_output_config.os_type = quxlang::os::macos;
        arm64.target_output_config.binary_type = quxlang::binary::macho;
        arm64.module_configurations["main"].source = "main_arm64";
        arm64.module_configurations["util"].source = "util_shared";
        arm64.outputs = std::map< std::string, quxlang::output_config >{
            {"ios_app", quxlang::output_config{.type = quxlang::output_kind::executable, .module = "main", .main_functanoid = "main"}},
        };

        bundle.targets["x64"] = x64;
        bundle.targets["arm64"] = arm64;

        bundle.module_sources["main_x64"].files["main.qxs"] = quxlang::source_file{.contents = "::main VAR I32;"};
        bundle.module_sources["main_arm64"].files["main.qxs"] = quxlang::source_file{.contents = "::main VAR I64;"};
        bundle.module_sources["util_shared"].files["util.qxs"] = quxlang::source_file{.contents = "::util VAR I32;"};

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
        bundle.module_sources["main_x64"].files["main.qxs"] = quxlang::source_file{.contents = std::move(contents)};

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
    ASSERT_EQ(resolved.module_sources.at("main_x64").files.at("main.qxs").get().contents, "::main VAR I32;");
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

TEST(querygraph_queries, output_list_returns_configured_outputs_for_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph x64_graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
    quxlang::compiler_querygraph arm64_graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());

    std::set< std::string > x64_outputs = x64_graph.make_request< quxlang::output_list_query >(std::monostate{});
    std::set< std::string > arm64_outputs = arm64_graph.make_request< quxlang::output_list_query >(std::monostate{});

    ASSERT_EQ(x64_outputs, (std::set< std::string >{"app", "util"}));
    ASSERT_EQ(arm64_outputs, (std::set< std::string >{"ios_app"}));
}

TEST(querygraph_queries, output_list_returns_default_when_outputs_are_omitted)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::set< std::string > outputs = graph.make_request< quxlang::output_list_query >(std::monostate{});

    ASSERT_EQ(outputs, (std::set< std::string >{"default"}));
}

TEST(querygraph_queries, numeric_literal_prefers_unsigned_integer_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::type_symbol numeric_literal = quxlang::numeric_literal_reference{};
    quxlang::type_symbol unsigned_integer = quxlang::int_type{.bits = 64, .has_sign = false};
    quxlang::type_symbol signed_integer = quxlang::int_type{.bits = 64, .has_sign = true};

    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = numeric_literal,
        .better_to = unsigned_integer,
        .worse_to = signed_integer,
    }));
    ASSERT_FALSE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = numeric_literal,
        .better_to = signed_integer,
        .worse_to = unsigned_integer,
    }));
}

TEST(querygraph_queries, temporary_source_prefers_temporary_reference_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::type_symbol byte_type = quxlang::byte_type{};
    quxlang::type_symbol temporary_byte_ref = quxlang::make_tref(byte_type);
    quxlang::type_symbol const_byte_ref = quxlang::make_cref(byte_type);

    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = temporary_byte_ref,
        .worse_to = const_byte_ref,
    }));
    ASSERT_FALSE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = const_byte_ref,
        .worse_to = temporary_byte_ref,
    }));
    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = temporary_byte_ref,
        .worse_to = const_byte_ref,
        .adaptations = quxlang::allowed_adaptations::none,
    }));
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

    ASSERT_TRUE(resolved.files.contains("util.qxs"));
    ASSERT_EQ(resolved.files.at("util.qxs").get().contents, "::util VAR I32;");
}

TEST(querygraph_queries, source_file_index_assigns_deterministic_global_ids)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::source_file_index_query >(std::monostate{});

    ASSERT_EQ(resolved.file_to_id.size(), 3);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qxs"}), 0);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qxs"}), 1);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "util_shared", .relative_path = "util.qxs"}), 2);
}

TEST(querygraph_queries, source_file_id_is_unique_across_modules)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto arm_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qxs"});
    auto x64_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qxs"});

    ASSERT_NE(arm_id, x64_id);
}

TEST(querygraph_queries, source_file_name_round_trips_from_id)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::source_file_name input{.source_module = "util_shared", .relative_path = "util.qxs"};

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

TEST(querygraph_queries, enum_info_normalizes_values_defaults_and_reservations)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::choice ENUM BITS(8) [none = NULL, x, RESERVED FROM(2) TO(3), y = 4] ALLOW_UNKNOWN {
    ::zero STATIC choice := none;
}
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol choice = quxlang::subsymbol{main, "choice"};
    quxlang::type_symbol none = quxlang::subsymbol{choice, "none"};
    quxlang::type_symbol zero = quxlang::subsymbol{choice, "zero"};

    quxlang::enum_info info = graph.make_request< quxlang::enum_info_query >(choice);

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(choice), quxlang::symbol_kind::enum_);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(none), quxlang::symbol_kind::enum_value);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(zero), quxlang::symbol_kind::global_variable);
    EXPECT_EQ(info.bits, 8);
    EXPECT_EQ(info.storage_bytes, 1);
    EXPECT_TRUE(info.allow_unknown);
    ASSERT_EQ(info.values.size(), 3);
    ASSERT_EQ(info.reserved_ranges.size(), 1);
    EXPECT_EQ(info.reserved_ranges.at(0).from, 2);
    EXPECT_EQ(info.reserved_ranges.at(0).to, 3);
    ASSERT_TRUE(info.null_value_name.has_value());
    EXPECT_EQ(*info.null_value_name, "none");
    ASSERT_TRUE(info.default_value_name.has_value());
    EXPECT_EQ(*info.default_value_name, "none");

    std::map< std::string, quxlang::enum_value_info > values;
    for (quxlang::enum_value_info const& value : info.values)
    {
        values[value.name] = value;
    }

    EXPECT_EQ(values.at("none").value, 0);
    EXPECT_TRUE(values.at("none").is_null);
    EXPECT_TRUE(values.at("none").is_default);
    EXPECT_EQ(values.at("x").value, 1);
    EXPECT_FALSE(values.at("x").is_explicit);
    EXPECT_EQ(values.at("y").value, 4);
    EXPECT_TRUE(values.at("y").is_explicit);

    quxlang::type_placement_info placement = graph.make_request< quxlang::type_placement_info_query >(choice);
    EXPECT_EQ(placement.size, 1);
    EXPECT_EQ(placement.alignment, 1);
}

TEST(querygraph_queries, enum_info_rejects_conflicting_semantics)
{
    auto expect_bad_enum = [](std::string const& source)
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle(source);
        quxlang::compiler_querygraph graph = make_x64_graph(bundle);
        quxlang::type_symbol bad = quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "bad"};
        EXPECT_THROW(graph.make_request< quxlang::enum_info_query >(bad), std::logic_error);
    };

    expect_bad_enum("::bad ENUM [none = NULL, x DEFAULT];");
    expect_bad_enum("::bad ENUM [a DEFAULT, b DEFAULT];");
    expect_bad_enum("::bad ENUM [RESERVED FROM(1) TO(2), a = 1];");
}

TEST(querygraph_queries, flagset_info_allocates_implicit_bits_around_reserved_masks)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::permissions FLAGSET [read, write, RESERVED = 12, exec] {
    ::read_write STATIC permissions := read #|| write;
}
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol permissions = quxlang::subsymbol{main, "permissions"};
    quxlang::type_symbol read = quxlang::subsymbol{permissions, "read"};
    quxlang::type_symbol read_write = quxlang::subsymbol{permissions, "read_write"};

    quxlang::flagset_info info = graph.make_request< quxlang::flagset_info_query >(permissions);

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(permissions), quxlang::symbol_kind::flagset_);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(read), quxlang::symbol_kind::flagset_value);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(read_write), quxlang::symbol_kind::global_variable);
    EXPECT_EQ(info.bits, 5);
    EXPECT_EQ(info.storage_bytes, 1);
    ASSERT_EQ(info.values.size(), 3);
    ASSERT_EQ(info.reserved_masks.size(), 1);
    EXPECT_EQ(info.reserved_masks.at(0).mask, 12);
    EXPECT_EQ(info.reserved_bit_mask, 12);
    EXPECT_EQ(info.canonical_bit_mask, 19);

    std::map< std::string, quxlang::flagset_value_info > values;
    for (quxlang::flagset_value_info const& value : info.values)
    {
        values[value.name] = value;
    }

    EXPECT_EQ(values.at("read").mask, 1);
    EXPECT_EQ(values.at("write").mask, 2);
    EXPECT_EQ(values.at("exec").mask, 16);

    quxlang::type_placement_info placement = graph.make_request< quxlang::type_placement_info_query >(permissions);
    EXPECT_EQ(placement.size, 1);
    EXPECT_EQ(placement.alignment, 1);
}

TEST(querygraph_queries, flagset_info_rejects_overlapping_canonical_and_reserved_bits)
{
    auto expect_bad_flagset = [](std::string const& source)
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle(source);
        quxlang::compiler_querygraph graph = make_x64_graph(bundle);
        quxlang::type_symbol bad = quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "bad"};
        EXPECT_THROW(graph.make_request< quxlang::flagset_info_query >(bad), std::logic_error);
    };

    expect_bad_flagset("::bad FLAGSET [a = 3, b = 1];");
    expect_bad_flagset("::bad FLAGSET [RESERVED = 2, a = 3];");
    expect_bad_flagset("::bad FLAGSET [a = 0];");
}

TEST(querygraph_queries, interface_symbols_slots_and_implementation_map)
{
    auto bundle = make_single_main_source_bundle(R"(
::iface INTERFACE DEFAULTABLE {
    .value FUNCTION(%x I32): I32;
    .fallback FUNCTION(%x I32): I32 { RETURN x + 10; }
    .pick FUNCTION(%x I32): I32;
    .pick FUNCTION(%x I64): I64;
}
::impl IMPLEMENTATION(iface) {
    ::value FUNCTION(%x I32): I32 { RETURN x + 1; }
    ::fallback FUNCTION(%x I32): I32 { RETURN x + 2; }
    ::pick FUNCTION(%x I32): I32 { RETURN x + 3; }
    ::pick FUNCTION(%x I64): I64 { RETURN x + 4; }
}
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto iface = quxlang::type_symbol(quxlang::subsymbol{main, "iface"});
    auto impl = quxlang::type_symbol(quxlang::subsymbol{main, "impl"});

    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(iface), quxlang::symbol_kind::interface_);
    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(impl), quxlang::symbol_kind::implementation_);
    EXPECT_TRUE(graph.make_request< quxlang::interface_defaultable_query >(iface));
    EXPECT_EQ(graph.make_request< quxlang::implementation_interface_type_query >(impl), iface);

    auto placement = graph.make_request< quxlang::type_placement_info_query >(iface);
    auto machine = graph.make_request< quxlang::machine_info_query >(std::monostate{});
    EXPECT_EQ(placement.size, machine.pointer_size_bytes());
    EXPECT_EQ(placement.alignment, machine.pointer_align());

    auto slots = graph.make_request< quxlang::interface_slot_list_query >(iface);
    ASSERT_EQ(slots.size(), 4);
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto i64 = quxlang::type_symbol(quxlang::int_type{64, true});
    quxlang::interface_slot_key value_key{.name = "value", .concrete_params = quxlang::invotype{.positional = {i32}}, .concrete_return_type = i32};
    quxlang::interface_slot_key pick_i64_key{.name = "pick", .concrete_params = quxlang::invotype{.positional = {i64}}, .concrete_return_type = i64};

    auto has_value_slot = std::ranges::any_of(slots,
                                              [&](quxlang::interface_slot const& slot)
                                              {
                                                  return slot.key == value_key && !slot.declaration.has_default_body;
                                              });
    auto has_pick_i64_slot = std::ranges::any_of(slots,
                                                 [&](quxlang::interface_slot const& slot)
                                                 {
                                                     return slot.key == pick_i64_key;
                                                 });
    EXPECT_TRUE(has_value_slot);
    EXPECT_TRUE(has_pick_i64_slot);

    auto functions = graph.make_request< quxlang::implementation_function_map_query >(impl);
    ASSERT_EQ(functions.size(), slots.size());
    ASSERT_TRUE(functions.contains(value_key));
    EXPECT_NE(quxlang::to_string(functions.at(value_key)).find("impl::value"), std::string::npos);

    auto get_impl = quxlang::type_symbol(quxlang::subsymbol{impl, "GET_INTERFACE_IMPL"});
    auto get_impl_inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = get_impl,
        .parameters = {},
        .adaptations = quxlang::allowed_adaptations::destination_rebinding,
    });
    ASSERT_TRUE(get_impl_inst.has_value());
    auto routine = graph.make_request< quxlang::vm_procedure3_query >(*get_impl_inst);
    quxlang::vmir2::assembler asm_printer(routine);
    EXPECT_NE(asm_printer.to_string(routine).find("INTERFACE_INIT"), std::string::npos);
}

TEST(querygraph_queries, interface_slot_validation_rejects_bad_shapes)
{
    auto expect_bad_interface = [](std::string const& source)
    {
        auto bundle = make_single_main_source_bundle(source);
        auto graph = make_x64_graph(bundle);
        auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
        auto iface = quxlang::type_symbol(quxlang::subsymbol{main, "iface"});
        EXPECT_THROW(graph.make_request< quxlang::interface_slot_list_query >(iface), std::logic_error);
    };

    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x AUTO): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%...xs I32): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(@THIS I32): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x MissingType): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x I32): I32; .value FUNCTION(%y I32): I32; }");
}

TEST(querygraph_queries, interface_implementation_map_rejects_missing_slot)
{
    auto bundle = make_single_main_source_bundle(R"(
::iface INTERFACE { .value FUNCTION(): I32; }
::bad_impl IMPLEMENTATION(iface) { }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto bad_impl = quxlang::type_symbol(quxlang::subsymbol{main, "bad_impl"});

    EXPECT_THROW(graph.make_request< quxlang::implementation_function_map_query >(bad_impl), std::logic_error);
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

TEST(querygraph_queries, type_is_trivially_default_constructible_accepts_zero_constructible_shapes)
{
    auto bundle = make_single_main_source_bundle(R"(
::zero_enum ENUM [zero = 0 DEFAULT, one = 1];
::nonzero_enum ENUM [one = 1 DEFAULT, zero = 0];
::flags FLAGSET [read, write];
::trivial_record CLASS { .x VAR I32; .values VAR [3]I32; }
::blocked_record CLASS NO_IMPLICIT_DEFAULT_CONSTRUCTOR { .x VAR I32; }
::custom_record CLASS { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto f32 = quxlang::type_symbol(quxlang::float_type{32});
    quxlang::procedure_type procedure;
    procedure.signature.return_type = i32;
    auto procedure_type = quxlang::type_symbol(procedure);
    auto pointer_type = quxlang::type_symbol(quxlang::ptrref_type{.target = i32, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut});
    auto storage_type = quxlang::type_symbol(quxlang::storage{.storable_types = {i32}});
    auto i32_array = quxlang::type_symbol(quxlang::array_type{.element_type = i32, .element_count = quxlang::expression_numeric_literal{"4"}});
    auto custom_array = quxlang::type_symbol(quxlang::array_type{.element_type = quxlang::subsymbol{main, "custom_record"}, .element_count = quxlang::expression_numeric_literal{"2"}});

    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(i32));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::bool_type{}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(f32));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(pointer_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(procedure_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(storage_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(i32_array));

    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "flags"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "zero_enum"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "nonzero_enum"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "trivial_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "blocked_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "custom_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(custom_array));
}

TEST(querygraph_queries, global_init_type_classifies_default_trivial_globals)
{
    auto bundle = make_single_main_source_bundle(R"(
::custom_record CLASS { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::trivial_global VAR I32;
::trivial_array_global VAR [2]I32;
::explicit_global VAR I32 := 0;
::custom_global VAR custom_record;
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "trivial_global"}), quxlang::initialization_type::init_trivial);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "trivial_array_global"}), quxlang::initialization_type::init_trivial);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "explicit_global"}), quxlang::initialization_type::init_with_guard);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "custom_global"}), quxlang::initialization_type::init_with_guard);
}

TEST(querygraph_queries, global_get_reference_omits_initguard_for_trivial_globals)
{
    auto bundle = make_single_main_source_bundle(R"(
::custom_record CLASS { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::trivial_global VAR I32;
::custom_global VAR custom_record;
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto routine_text = [&](std::string const& name)
    {
        quxlang::type_symbol const global = quxlang::subsymbol{main, name};
        auto inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
            .initializee = quxlang::submember{global, "GET_REFERENCE"},
            .parameters = {},
            .adaptations = quxlang::allowed_adaptations::destination_rebinding,
        });
        EXPECT_TRUE(inst.has_value());
        quxlang::vmir2::functanoid_routine3 routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
        return quxlang::vmir2::assembler(routine).to_string(routine);
    };

    std::string const trivial_text = routine_text("trivial_global");
    EXPECT_NE(trivial_text.find("GET_GLOBAL_REF"), std::string::npos);
    EXPECT_EQ(trivial_text.find("GET_GLOBAL_STORAGE"), std::string::npos);
    EXPECT_EQ(trivial_text.find("STORAGE_PUN"), std::string::npos);
    EXPECT_EQ(trivial_text.find("INITGUARD_TRY_ACQUIRE"), std::string::npos);
    EXPECT_EQ(trivial_text.find("CALL"), std::string::npos);

    std::string const custom_text = routine_text("custom_global");
    EXPECT_NE(custom_text.find("INITGUARD_TRY_ACQUIRE"), std::string::npos);
}

TEST(querygraph_queries, static_classification_keywords_and_user_deserialize_constructor)
{
    auto bundle = make_single_main_source_bundle(R"(
::plain CLASS { .x VAR I32; }
::explicit_antestatal CLASS ANTESTATAL { .x VAR I32; }
::explicit_serialoid CLASS SERIALOID { .x VAR I32; }
::default_serialoid CLASS {
    .x VAR I32;
    .SERIALIZE FUNCTION(@OUTPUT_ITERATOR:output =>> BYTE): =>> BYTE { RETURN output; }
    .CONSTRUCTOR FUNCTION(@DESERIALIZE_INPUT_ITERATOR:input =>> BYTE) { }
}
::plain_static STATIC plain;
::serial_static STATIC default_serialoid := default_serialoid();
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto plain = quxlang::type_symbol(quxlang::subsymbol{main, "plain"});
    auto explicit_antestatal = quxlang::type_symbol(quxlang::subsymbol{main, "explicit_antestatal"});
    auto explicit_serialoid = quxlang::type_symbol(quxlang::subsymbol{main, "explicit_serialoid"});
    auto default_serialoid = quxlang::type_symbol(quxlang::subsymbol{main, "default_serialoid"});
    auto plain_static = quxlang::type_symbol(quxlang::subsymbol{main, "plain_static"});
    auto serial_static = quxlang::type_symbol(quxlang::subsymbol{main, "serial_static"});

    EXPECT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(plain));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(explicit_antestatal));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(explicit_serialoid));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_serialoid_query >(explicit_serialoid));

    EXPECT_TRUE(graph.make_request< quxlang::user_deserialize_exists_query >(default_serialoid));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(default_serialoid));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_serialoid_query >(default_serialoid));

    EXPECT_TRUE(graph.make_request< quxlang::global_is_antestatal_static_query >(plain_static));
    EXPECT_TRUE(graph.make_request< quxlang::global_is_serialoid_static_query >(serial_static));
}

TEST(querygraph_queries, type_is_stringlike_requires_class_tag)
{
    auto bundle = make_single_main_source_bundle(R"(
::plain CLASS { .x VAR I32; }
::textish CLASS STRINGLIKE { .x VAR I32; }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto plain = quxlang::type_symbol(quxlang::subsymbol{main, "plain"});
    auto textish = quxlang::type_symbol(quxlang::subsymbol{main, "textish"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});

    EXPECT_FALSE(graph.make_request< quxlang::type_is_stringlike_query >(plain));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_stringlike_query >(textish));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_stringlike_query >(i32));
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

TEST(querygraph_queries, nonstatic_static_global_is_rejected)
{
    auto bundle = make_single_main_source_bundle("::blocked CLASS NONSTATIC { .x VAR I32; } ::foo STATIC blocked;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    EXPECT_THROW(graph.make_request< quxlang::global_is_antestatal_static_query >(foo), std::logic_error);
    EXPECT_THROW(graph.make_request< quxlang::global_is_serialoid_static_query >(foo), std::logic_error);
}
