// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_SWAP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_SWAP_SPEC_HEADER_GUARD

#include <quxlang/queries/class_requires_gen_swap.hpp>
#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/user_swap_exists.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_requires_gen_swap_spec
    {
        using query = class_requires_gen_swap_query;
        using dependencies = rpnx::typelist< class_tags_query, user_swap_exists_query >;
    };

    rpnx::querygraph::coroutine< class_requires_gen_swap_spec > class_requires_gen_swap_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_REQUIRES_GEN_SWAP_SPEC_HEADER_GUARD
