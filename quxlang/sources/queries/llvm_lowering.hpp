// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_LLVM_LOWERING_HEADER_GUARD
#define QUXLANG_LLVM_LOWERING_HEADER_GUARD
#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Lowers owned bodies using borrowed semantic data without retaining a packet query result. */
    template < rpnx::querygraph::query_handler_spec_c HandlerSpec >
    auto lower_llvm_unit(llvm_output_query_input input) -> typename rpnx::querygraph::coroutine< HandlerSpec >::template cosubroutine< rpnx::cow< llvm_backend::llvm_preoptimized_unit > >;
} // namespace quxlang
#endif
