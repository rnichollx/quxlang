// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_MOVE_CTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_MOVE_CTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_move_ctor.hpp>
#include <quxlang/queries/class_default_ctor.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/have_nontrivial_member_ctor.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/user_move_ctor_exists.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_requires_gen_move_ctor_spec
    {
        using query = class_requires_gen_move_ctor_query;
        using dependencies = rpnx::typelist< class_default_ctor_query, class_type_query, have_nontrivial_member_ctor_query, union_info_query, user_move_ctor_exists_query, variant_info_query >;
    };

    rpnx::querygraph::coroutine< class_requires_gen_move_ctor_spec > class_requires_gen_move_ctor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_MOVE_CTOR_SPEC_HEADER_GUARD
