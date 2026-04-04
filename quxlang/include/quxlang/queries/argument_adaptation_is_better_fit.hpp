// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ARGUMENT_ADAPTATION_IS_BETTER_FIT_HEADER_GUARD
#define QUXLANG_QUERIES_ARGUMENT_ADAPTATION_IS_BETTER_FIT_HEADER_GUARD

#include <quxlang/data/argument_adaptation_types.hpp>

namespace quxlang
{
    struct argument_adaptation_is_better_fit_query
    {
        static constexpr auto query_id = "argument_adaptation_is_better_fit";
        using input_type = argument_adaptation_better_fit_input;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ARGUMENT_ADAPTATION_IS_BETTER_FIT_HEADER_GUARD
