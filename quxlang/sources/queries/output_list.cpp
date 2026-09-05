// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_list_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_list_spec > quxlang::output_list_impl(std::monostate)
{
    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    std::string target = co_await rpnx::querygraph::request< configured_target_query >(std::monostate{});
    std::set< std::string > output;

    for (std::pair< std::string const, output_config > const& entry : bundle.outputs)
    {
        if (entry.second.target == target)
        {
            output.insert(entry.first);
        }
    }

    co_return output;
}
