// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "source_loader_internal.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>

#include <gtest/gtest.h>

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
