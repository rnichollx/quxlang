// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/target_steppings_spec.hpp>

rpnx::querygraph::coroutine< quxlang::target_steppings_spec > quxlang::target_steppings_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    if (target_config.steppings.has_value())
    {
        co_return *target_config.steppings;
    }

    if (target_config.target_output_config.cpu_type == cpu::arm_64 &&
        target_config.target_output_config.os_type == os::macos)
    {
        co_return std::vector< cpu_stepping_configuration >{
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

    co_return std::vector< cpu_stepping_configuration >{cpu_stepping_configuration{}};
}
