// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_CTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_CTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/have_nontrivial_member_ctor.hpp>
#include <quxlang/queries/class_builtin.hpp>
#include <quxlang/queries/class_default_ctor.hpp>
#include <quxlang/queries/class_field_list.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using have_nontrivial_member_ctor_spec = rpnx::query_handler_spec< have_nontrivial_member_ctor_query, rpnx::typelist< class_builtin_query, class_default_ctor_query, class_field_list_query, have_nontrivial_member_ctor_query > >;

    rpnx::querygraph::coroutine< have_nontrivial_member_ctor_spec > have_nontrivial_member_ctor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_CTOR_SPEC_HEADER_GUARD
