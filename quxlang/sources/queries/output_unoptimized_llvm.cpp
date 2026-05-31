// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_unoptimized_llvm_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_unoptimized_llvm_spec > quxlang::output_unoptimized_llvm_impl(std::string)
{
    throw quxlang::compiler_bug("output_unoptimized_llvm_query is not implemented");
    co_return {};
}
