// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_list_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_list_spec > quxlang::output_list_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    std::set< std::string > output;

    if (!target_config.outputs.has_value())
    {
        output.insert("default");
        co_return output;
    }

    for (auto const& output_entry : *target_config.outputs)
    {
        output.insert(output_entry.first);
    }

    co_return output;
}
