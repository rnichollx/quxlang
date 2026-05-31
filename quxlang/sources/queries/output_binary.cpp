// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_binary_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binary_spec > quxlang::output_binary_impl(std::string)
{
    throw quxlang::compiler_bug("output_binary_query is not implemented");
    co_return {};
}
