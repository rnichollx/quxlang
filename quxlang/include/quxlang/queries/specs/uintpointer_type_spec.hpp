// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_UINTPOINTER_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_UINTPOINTER_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/uintpointer_type.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using uintpointer_type_spec = rpnx::querygraph::query_handler_spec< uintpointer_type_query, rpnx::typelist< machine_info_query > >;

    rpnx::querygraph::coroutine< uintpointer_type_spec > uintpointer_type_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_UINTPOINTER_TYPE_SPEC_HEADER_GUARD
