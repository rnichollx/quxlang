// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_steppings_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_steppings_spec > quxlang::output_steppings_impl(std::string input)
{
    output_build_settings settings = co_await rpnx::querygraph::request< output_build_settings_query >(std::move(input));
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    if (target_config.steppings.has_value())
    {
        co_return *target_config.steppings;
    }

    std::vector< cpu_stepping_configuration > steppings;
    if (target_config.target_output_config.cpu_type == cpu::x86_64)
    {
        steppings = std::vector< cpu_stepping_configuration >{
            cpu_stepping_configuration{.attributes = {{"X64_FEATURES_V1", true}}},
            cpu_stepping_configuration{.attributes = {{"X64_FEATURES_V2", true}}},
            cpu_stepping_configuration{.attributes = {{"X64_FEATURES_V3", true}}},
            cpu_stepping_configuration{.attributes = {{"X64_FEATURES_V4", true}}},
        };
    }
    else if (target_config.target_output_config.cpu_type == cpu::arm_64 && target_config.target_output_config.os_type == os::macos)
    {
        steppings = std::vector< cpu_stepping_configuration >{
            cpu_stepping_configuration{
                .attributes = {{"ARM_FEATURES_APPLE_M1", true}},
                .tune = "ARM_TUNE_APPLE_M1",
            },
            cpu_stepping_configuration{
                .attributes = {{"ARM_FEATURES_APPLE_M2", true}},
                .tune = "ARM_TUNE_APPLE_M2",
            },
            cpu_stepping_configuration{
                .attributes = {{"ARM_FEATURES_APPLE_M4", true}},
                .tune = "ARM_TUNE_APPLE_M4",
            },
            cpu_stepping_configuration{
                .attributes = {{"ARM_FEATURES_APPLE_M5", true}},
                .tune = "ARM_TUNE_APPLE_M5",
            },
        };
    }
    else
    {
        steppings.emplace_back();
    }
    if (!build_type_has_multiple_steppings(settings.build_type))
    {
        steppings.resize(1);
    }
    co_return steppings;
}
