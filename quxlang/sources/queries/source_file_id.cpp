// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/source_file_id_spec.hpp>

#include <stdexcept>

rpnx::querygraph::coroutine< quxlang::source_file_id_spec > quxlang::source_file_id_impl(source_file_name input)
{
    auto const index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto const iter = index.file_to_id.find(input);
    if (iter == index.file_to_id.end())
    {
        throw quxlang::compiler_bug("source_file_id_query received an unknown source file name");
    }
    co_return iter->second;
}
