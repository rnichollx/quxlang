// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_binaries_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binaries_spec > quxlang::output_binaries_impl(std::monostate)
{
    throw quxlang::compiler_bug("output_binaries_query is not implemented");
    co_return {};
}
