// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_binaries_information_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binaries_information_spec > quxlang::output_binaries_information_impl(std::monostate)
{
    std::set< std::string > const output_names = co_await rpnx::querygraph::request< output_list_query >(std::monostate{});
    std::map< std::string, output_query_output > output;

    for (std::string const& output_name : output_names)
    {
        output.emplace(output_name, co_await rpnx::querygraph::request< output_binary_information_query >(output_name));
    }

    co_return output;
}
