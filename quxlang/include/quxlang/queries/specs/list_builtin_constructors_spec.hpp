// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LIST_BUILTIN_CONSTRUCTORS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LIST_BUILTIN_CONSTRUCTORS_SPEC_HEADER_GUARD

#include <quxlang/queries/list_builtin_constructors.hpp>
#include <quxlang/queries/class_requires_gen_copy_ctor.hpp>
#include <quxlang/queries/class_requires_gen_default_ctor.hpp>
#include <quxlang/queries/class_requires_gen_move_ctor.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using list_builtin_constructors_spec = rpnx::query_handler_spec< list_builtin_constructors_query, rpnx::typelist< class_requires_gen_copy_ctor_query, class_requires_gen_default_ctor_query, class_requires_gen_move_ctor_query > >;

    rpnx::querygraph::coroutine< list_builtin_constructors_spec > list_builtin_constructors_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LIST_BUILTIN_CONSTRUCTORS_SPEC_HEADER_GUARD
