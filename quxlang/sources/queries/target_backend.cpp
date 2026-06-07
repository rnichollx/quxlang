// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/target_backend_spec.hpp>

rpnx::querygraph::coroutine< quxlang::target_backend_spec > quxlang::target_backend_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    co_return target_config.backend;
}
