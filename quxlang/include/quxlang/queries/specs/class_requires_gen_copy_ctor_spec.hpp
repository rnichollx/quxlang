// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_COPY_CTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_COPY_CTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_copy_ctor.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/user_copy_ctor_exists.hpp>
#include <quxlang/queries/variant_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_requires_gen_copy_ctor_spec
    {
        using query = class_requires_gen_copy_ctor_query;
        using dependencies = rpnx::typelist< class_type_query, struct_tags_query, union_info_query, user_copy_ctor_exists_query, variant_info_query >;
    };

    rpnx::querygraph::coroutine< class_requires_gen_copy_ctor_spec > class_requires_gen_copy_ctor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_COPY_CTOR_SPEC_HEADER_GUARD
