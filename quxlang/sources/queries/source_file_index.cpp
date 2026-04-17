// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/source_file_index_spec.hpp>

rpnx::querygraph::coroutine< quxlang::source_file_index_spec > quxlang::source_file_index_impl(std::monostate)
{
    auto const bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});

    source_file_index result;
    std::uint64_t next_id = 0;

    for (auto const& [source_module, module] : bundle.module_sources)
    {
        for (auto const& [relative_path, file] : module.files)
        {
            (void)file;
            source_file_name name{.source_module = source_module, .relative_path = relative_path};
            result.id_to_file.emplace(next_id, name);
            result.file_to_id.emplace(std::move(name), next_id);
            ++next_id;
        }
    }

    co_return result;
}
