// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_binary_artifact_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binary_artifact_spec > quxlang::output_binary_artifact_impl(std::string input)
{
    backend_kind const backend = co_await rpnx::querygraph::request< target_backend_query >(std::monostate{});

    if (backend == backend_kind::llvm)
    {
        co_return co_await rpnx::querygraph::request< llvm_output_binary_artifact_query >(std::move(input));
    }

    throw compiler_bug("Unsupported target backend");
}
