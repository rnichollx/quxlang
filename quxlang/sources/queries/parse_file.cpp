// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/parse_file_spec.hpp>
#include <quxlang/macros.hpp>

#include <quxlang/parsers/parse_file.hpp>

#include <algorithm>
#include <exception>
#include <format>
#include <iterator>
#include <stdexcept>
#include <string>
#include <variant>

rpnx::querygraph::coroutine< quxlang::parse_file_spec > quxlang::parse_file_impl(source_file_name input)
{
    auto const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});

    auto const module_iter = bundle.module_sources.find(input.source_module);
    if (module_iter == bundle.module_sources.end())
    {
        throw quxlang::compiler_bug(std::format("parse_file_query received an unknown source module '{}'", input.source_module));
    }

    auto const file_iter = module_iter->second.files.find(input.relative_path);
    if (file_iter == module_iter->second.files.end())
    {
        throw quxlang::compiler_bug(std::format("parse_file_query received an unknown source file '{}/{}'", input.source_module, input.relative_path));
    }

    auto const& content = file_iter->second->contents;
    auto const& source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto const file_id_iter = source_file_index.file_to_id.find(input);
    if (file_id_iter == source_file_index.file_to_id.end())
    {
        throw quxlang::compiler_bug(std::format("parse_file_query could not resolve source file id for '{}/{}'", input.source_module, input.relative_path));
    }
    auto const file_id = file_id_iter->second;

    parsers::parsing_context ctx{
        .file_id = file_id,
        .source_locations_enabled = true,
        .iter_begin = content.begin(),
        .iter_pos = content.begin(),
        .iter_end = content.end(),
    };

    ast2_file_declaration file_ast;

    std::exception_ptr parse_error;
    std::string error_snippet;
    std::string error_message;
    auto capture_parse_error = [&](std::exception const& error) -> void
    {
        auto const distance_to_end = std::distance(ctx.iter_pos, ctx.iter_end);
        auto const snippet_size = std::min(distance_to_end, std::ptrdiff_t(100));
        auto const end_snippet_iter = ctx.iter_pos + snippet_size;

        error_snippet = std::string(ctx.iter_pos, end_snippet_iter);
        error_message = error.what();
        parse_error = std::current_exception();
    };

    try
    {
        file_ast = parsers::parse_file(ctx);
    }
    catch (compilation_error& error)
    {
        error.traceback.push_back(trace_frame{.trace_context = "parse file", .location = ctx.get_location_optional()});
        capture_parse_error(error);
    }
    catch (std::exception const& error)
    {
        capture_parse_error(error);
    }

    if (parse_error)
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("At:  {}", error_snippet);
            co_yield rpnx::querygraph::debug_message("Error: {}", error_message);
        }

        std::rethrow_exception(parse_error);
    }

    file_ast.filename = input.source_module + "/" + input.relative_path;
    co_return file_ast;
}
