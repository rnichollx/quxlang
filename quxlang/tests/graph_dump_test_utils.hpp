// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_TESTS_GRAPH_DUMP_TEST_UTILS_HPP
#define QUXLANG_TESTS_GRAPH_DUMP_TEST_UTILS_HPP

#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>

#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace quxlang::tests
{
    inline auto configured_graph_dump_dir() -> std::optional< std::filesystem::path >
    {
        constexpr std::string_view graph_dump_dir_prefix = "--graph-dump-dir=";

        for (std::string const& argument : ::testing::internal::GetArgvs())
        {
            if (argument.starts_with(graph_dump_dir_prefix))
            {
                return std::filesystem::path(argument.substr(graph_dump_dir_prefix.size()));
            }
        }

        return std::nullopt;
    }

    inline auto current_test_graph_dump_path() -> std::optional< std::filesystem::path >
    {
        std::optional< std::filesystem::path > const output_dir = configured_graph_dump_dir();
        if (!output_dir.has_value())
        {
            return std::nullopt;
        }

        std::filesystem::create_directories(*output_dir);

        std::random_device random_device;
        std::uniform_int_distribution<unsigned int> distribution(0, 15);

        std::string file_stem;
        file_stem.reserve(32);
        for (std::size_t index = 0; index < 32; ++index)
        {
            unsigned int const digit = distribution(random_device);
            file_stem += static_cast<char>(digit < 10 ? ('0' + digit) : ('a' + (digit - 10)));
        }

        return *output_dir / (file_stem + ".qgd");
    }
} // namespace quxlang::tests

#endif // QUXLANG_TESTS_GRAPH_DUMP_TEST_UTILS_HPP
