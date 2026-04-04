// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_DTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_DTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/have_nontrivial_member_dtor.hpp>
#include <quxlang/queries/class_builtin.hpp>
#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/class_trivially_destructible.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using have_nontrivial_member_dtor_spec = rpnx::query_handler_spec< have_nontrivial_member_dtor_query, rpnx::typelist< class_builtin_query, class_default_dtor_query, class_field_list_query, class_trivially_destructible_query > >;

    rpnx::querygraph::coroutine< have_nontrivial_member_dtor_spec > have_nontrivial_member_dtor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_HAVE_NONTRIVIAL_MEMBER_DTOR_SPEC_HEADER_GUARD
