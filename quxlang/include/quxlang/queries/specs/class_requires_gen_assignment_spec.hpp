// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_ASSIGNMENT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_ASSIGNMENT_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_assignment.hpp>
#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/have_nontrivial_member_ctor.hpp>
#include <quxlang/queries/user_assignment_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using class_requires_gen_assignment_spec = rpnx::query_handler_spec< class_requires_gen_assignment_query, rpnx::typelist< class_tags_query, have_nontrivial_member_ctor_query, user_assignment_exists_query > >;

    rpnx::querygraph::coroutine< class_requires_gen_assignment_spec > class_requires_gen_assignment_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_ASSIGNMENT_SPEC_HEADER_GUARD
