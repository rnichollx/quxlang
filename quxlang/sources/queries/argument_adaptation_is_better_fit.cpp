// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/argument_adaptation_is_better_fit_spec.hpp>

rpnx::querygraph::coroutine< quxlang::argument_adaptation_is_better_fit_spec > quxlang::argument_adaptation_is_better_fit_impl(argument_adaptation_better_fit_input input)
{
    auto better_rank = co_await rpnx::querygraph::request< argument_adaptation_rank_query >(argument_init_input{
                                                                         .from = input.from,
                                                                         .to = input.better_to,
                                                                         .adaptations = input.adaptations,
                                                                     });
    if (!better_rank.has_value())
    {
        co_return false;
    }

    auto worse_rank = co_await rpnx::querygraph::request< argument_adaptation_rank_query >(argument_init_input{
                                                                        .from = input.from,
                                                                        .to = input.worse_to,
                                                                        .adaptations = input.adaptations,
                                                                    });
    if (!worse_rank.has_value())
    {
        co_return true;
    }

    co_return *better_rank < *worse_rank;
}
