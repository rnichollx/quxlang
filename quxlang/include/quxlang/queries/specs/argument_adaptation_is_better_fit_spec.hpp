// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_IS_BETTER_FIT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_IS_BETTER_FIT_SPEC_HEADER_GUARD

#include <quxlang/queries/argument_adaptation_is_better_fit.hpp>
#include <quxlang/queries/argument_adaptation_rank.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct argument_adaptation_is_better_fit_spec
    {
        using query = argument_adaptation_is_better_fit_query;
        using dependencies = rpnx::typelist< argument_adaptation_rank_query >;
    };

    rpnx::querygraph::coroutine< argument_adaptation_is_better_fit_spec > argument_adaptation_is_better_fit_impl(argument_adaptation_better_fit_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_IS_BETTER_FIT_SPEC_HEADER_GUARD
