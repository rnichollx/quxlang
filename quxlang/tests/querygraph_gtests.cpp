// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/source_bundle.hpp>

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
} // namespace

TEST(querygraph_queries, source_bundle_returns_injected_bundle)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config);

    auto resolved = graph.make_request< quxlang::source_bundle_query >(std::monostate{});

    ASSERT_TRUE(resolved.targets.contains("x64"));
    ASSERT_TRUE(resolved.targets.contains("arm64"));
    ASSERT_TRUE(resolved.module_sources.contains("main_x64"));
    ASSERT_EQ(resolved.module_sources.at("main_x64").files.at("main.qx").get().contents, "::main VAR I32;");
}

TEST(querygraph_queries, machine_info_returns_injected_output_info)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config);

    auto resolved = graph.make_request< quxlang::machine_info_query >(std::monostate{});

    ASSERT_EQ(resolved.cpu_type, quxlang::cpu::x86_64);
    ASSERT_EQ(resolved.os_type, quxlang::os::linux);
    ASSERT_EQ(resolved.binary_type, quxlang::binary::elf);
}

TEST(querygraph_queries, module_source_name_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph x64_graph(bundle, "x64", bundle.targets.at("x64").target_output_config);

    quxlang::compiler_querygraph arm64_graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config);

    ASSERT_EQ(x64_graph.make_request< quxlang::module_source_name_query >("main"), "main_x64");
    ASSERT_EQ(arm64_graph.make_request< quxlang::module_source_name_query >("main"), "main_arm64");
}

TEST(querygraph_queries, module_source_name_map_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config);

    auto resolved = graph.make_request< quxlang::module_source_name_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at("main"), "main_arm64");
    ASSERT_EQ(resolved.at("util"), "util_shared");
}

TEST(querygraph_queries, module_sources_returns_source_files_for_logical_module)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config);

    auto resolved = graph.make_request< quxlang::module_sources_query >("util");

    ASSERT_TRUE(resolved.files.contains("util.qx"));
    ASSERT_EQ(resolved.files.at("util.qx").get().contents, "::util VAR I32;");
}
