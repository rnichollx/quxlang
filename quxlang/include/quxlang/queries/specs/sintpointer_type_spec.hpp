// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SINTPOINTER_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SINTPOINTER_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/sintpointer_type.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using sintpointer_type_spec = rpnx::query_handler_spec< sintpointer_type_query, rpnx::typelist< machine_info_query > >;

    rpnx::querygraph::coroutine< sintpointer_type_spec > sintpointer_type_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SINTPOINTER_TYPE_SPEC_HEADER_GUARD
