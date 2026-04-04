// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_RANK_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_RANK_SPEC_HEADER_GUARD

#include <quxlang/queries/argument_adaptation_rank.hpp>
#include <quxlang/queries/argument_initialize_by_class_conversion.hpp>
#include <quxlang/queries/bindable.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using argument_adaptation_rank_spec = rpnx::query_handler_spec< argument_adaptation_rank_query, rpnx::typelist< argument_initialize_by_class_conversion_query, bindable_query > >;

    rpnx::querygraph::coroutine< argument_adaptation_rank_spec > argument_adaptation_rank_impl(argument_init_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ARGUMENT_ADAPTATION_RANK_SPEC_HEADER_GUARD
