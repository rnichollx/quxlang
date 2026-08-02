// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "source_loader_internal.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <string>
#include <type_traits>


TEST(source_loader, reproducibility_error_is_not_a_compilation_error)
{
    static_assert(std::is_base_of_v< std::logic_error, quxlang::reproducibility_error >);
    static_assert(!std::is_base_of_v< quxlang::compilation_error, quxlang::reproducibility_error >);
}

TEST(source_loader, rejects_carriage_returns)
{
    EXPECT_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", "::main VAR I32;\r\n"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_byte_order_marks)
{
    std::string const source = "\xef\xbb\xbf::main VAR I32;\n";
    EXPECT_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", source), quxlang::reproducibility_error);
}

TEST(source_loader, accepts_portable_source_contents)
{
    EXPECT_NO_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", "::main VAR I32;\n"));
}

TEST(source_loader, rejects_paths_differing_only_in_capitalization)
{
    quxlang::detail::source_path_validator validator;
    validator.add("modules/main/sources/Main.qxs");
    EXPECT_THROW(validator.add("modules/main/sources/main.qxs"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_characters_illegal_on_a_major_filesystem)
{
    quxlang::detail::source_path_validator validator;
    EXPECT_THROW(validator.add("modules/main/sources/bad?.qxs"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_filename_components_over_portable_limit)
{
    quxlang::detail::source_path_validator validator;
    std::string const long_filename(252, 'a');
    EXPECT_THROW(validator.add(std::filesystem::path("modules/main/sources") / (long_filename + ".qxs")), quxlang::reproducibility_error);
}

TEST(source_loader, parses_ordered_cpu_stepping_attribute_forms)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_FEATURE_SSE2
- attributes:
    X64_FEATURE_AVX2: true
    X64_PERF_FAST_GATHER: false
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(2));
    EXPECT_FALSE(steppings.at(0).tune.has_value());
    EXPECT_FALSE(steppings.at(1).tune.has_value());
    EXPECT_EQ(steppings.at(0).attributes, (std::map< std::string, bool >{{"X64_FEATURE_SSE2", true}}));
    EXPECT_EQ(
        steppings.at(1).attributes,
        (std::map< std::string, bool >{{"X64_FEATURE_AVX2", true}, {"X64_PERF_FAST_GATHER", false}}));
}

TEST(source_loader, parses_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_VENDOR_AMD
  tune: X64_TUNE_AMD_ZEN4
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(1));
    ASSERT_TRUE(steppings.front().tune.has_value());
    EXPECT_EQ(*steppings.front().tune, "X64_TUNE_AMD_ZEN4");
}

TEST(source_loader, parses_historical_x86_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X86_VENDOR_AMD
  tune: X86_TUNE_AMD_ATHLON_XP
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_32, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(1));
    EXPECT_EQ(steppings.front().attributes, (std::map< std::string, bool >{{"X86_VENDOR_AMD", true}}));
    ASSERT_TRUE(steppings.front().tune.has_value());
    EXPECT_EQ(*steppings.front().tune, "X86_TUNE_AMD_ATHLON_XP");
}

TEST(source_loader, rejects_unknown_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes: []
  tune: X64_TUNE_AMD_NOT_REGISTERED
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, rejects_cpu_stepping_tuning_model_for_another_cpu)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes: []
  tune: X64_TUNE_INTEL_HASWELL
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::arm_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, preserves_cpu_attribute_group_constraints)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_FEATURES_V1
- attributes:
    X64_FEATURES_V2: false
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(2));
    EXPECT_EQ(steppings.at(0).attributes, (std::map< std::string, bool >{{"X64_FEATURES_V1", true}}));
    EXPECT_EQ(steppings.at(1).attributes, (std::map< std::string, bool >{{"X64_FEATURES_V2", false}}));
}

TEST(source_loader, rejects_cpu_stepping_attribute_for_another_cpu)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - ARM64_FEATURE_ADVANCED_SIMD
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, rejects_disabled_attribute_at_stepping_zero)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    X64_FEATURE_SSE2: false
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}
