// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/queries/specs/output_llvm_input_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_llvm_input_spec > quxlang::output_llvm_input_impl(std::string)
{
    throw quxlang::compiler_bug("output_llvm_input_query is not implemented");
    co_return {};
}
