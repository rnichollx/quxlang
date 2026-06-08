// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/run_static_tests_spec.hpp>

rpnx::querygraph::coroutine< quxlang::run_static_tests_spec > quxlang::run_static_tests_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    if (!target_config.run_static_tests)
    {
        co_return std::monostate{};
    }

    // To get better performance, yield dependencies so they can be processed in separate threads
    for (std::pair< std::string const, module_configuration > const& module_entry : target_config.module_configurations)
    {
        type_symbol const module_symbol = absolute_module_reference{.module_name = module_entry.first};
        co_yield rpnx::querygraph::dependency< list_static_tests_query >(module_symbol);
    }

    // Now, for each module, we should yield dependencies on each test
    for (std::pair< std::string const, module_configuration > const& module_entry : target_config.module_configurations)
    {
        type_symbol const module_symbol = absolute_module_reference{.module_name = module_entry.first};
        std::set< type_symbol > const & tests = co_await rpnx::querygraph::request< list_static_tests_query >(module_symbol);
        for (type_symbol const& test : tests)
        {
            co_yield rpnx::querygraph::dependency< run_static_test_query >(test);
        }
    }

    // finally, we actually gather the results of each test by repeating the loop with co_await instead of co_yield
    for (std::pair< std::string const, module_configuration > const& module_entry : target_config.module_configurations)
    {
        type_symbol const module_symbol = absolute_module_reference{.module_name = module_entry.first};
        std::set< type_symbol > const & tests = co_await rpnx::querygraph::request< list_static_tests_query >(module_symbol);
        for (type_symbol const& test : tests)
        {
            co_await rpnx::querygraph::request< run_static_test_query >(test);
        }
    }

    co_return std::monostate{};
}
