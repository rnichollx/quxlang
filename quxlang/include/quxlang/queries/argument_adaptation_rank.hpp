// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ARGUMENT_ADAPTATION_RANK_HEADER_GUARD
#define QUXLANG_QUERIES_ARGUMENT_ADAPTATION_RANK_HEADER_GUARD

#include <quxlang/data/argument_adaptation_types.hpp>
#include <optional>
#include <cstddef>


namespace quxlang
{
    struct argument_adaptation_rank_query
    {
        static constexpr auto query_id = "argument_adaptation_rank";
        using input_type = argument_init_input;
        using output_type = std::optional<std::size_t>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ARGUMENT_ADAPTATION_RANK_HEADER_GUARD
