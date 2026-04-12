// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/type_is_antestatal.hpp>
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

TEST(querygraph_queries, module_sources_returns_source_files_for_logical_module)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::module_sources_query >("util");

    ASSERT_TRUE(resolved.files.contains("util.qx"));
    ASSERT_EQ(resolved.files.at("util.qx").get().contents, "::util VAR I32;");
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
