// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_CTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_CTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_default_ctor.hpp>
#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/have_nontrivial_member_ctor.hpp>
#include <quxlang/queries/user_default_ctor_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using class_requires_gen_default_ctor_spec = rpnx::query_handler_spec< class_requires_gen_default_ctor_query, rpnx::typelist< class_tags_query, have_nontrivial_member_ctor_query, user_default_ctor_exists_query > >;

    rpnx::querygraph::coroutine< class_requires_gen_default_ctor_spec > class_requires_gen_default_ctor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_DEFAULT_CTOR_SPEC_HEADER_GUARD
