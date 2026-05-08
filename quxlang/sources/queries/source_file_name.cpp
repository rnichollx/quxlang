// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/source_file_name_spec.hpp>

#include <stdexcept>

rpnx::querygraph::coroutine< quxlang::source_file_name_spec > quxlang::source_file_name_impl(std::uint64_t input)
{
    auto const index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto const iter = index.id_to_file.find(input);
    if (iter == index.id_to_file.end())
    {
        throw quxlang::compiler_bug("source_file_name_query received an unknown source file id");
    }
    co_return iter->second;
}
