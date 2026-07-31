// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/target_steppings_spec.hpp>

rpnx::querygraph::coroutine< quxlang::target_steppings_spec > quxlang::target_steppings_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    if (target_config.steppings.has_value())
    {
        co_return *target_config.steppings;
    }

    co_return std::vector< cpu_stepping_configuration >{cpu_stepping_configuration{}};
}
