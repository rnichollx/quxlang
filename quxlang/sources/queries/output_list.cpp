// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_list_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_list_spec > quxlang::output_list_impl(std::monostate)
{
    throw quxlang::compiler_bug("output_list_query is not implemented");
    co_return {};
}
