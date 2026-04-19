// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_ARGUMENT_ADAPTATION_TYPES_HEADER_GUARD
#define QUXLANG_DATA_ARGUMENT_ADAPTATION_TYPES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct argument_init_input
    {
        type_symbol from;
        type_symbol to;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(argument_init_input, from, to, adaptations);
    };

    struct argument_adaptation_better_fit_input
    {
        type_symbol from;
        type_symbol better_to;
        type_symbol worse_to;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(argument_adaptation_better_fit_input, from, better_to, worse_to, adaptations);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_ARGUMENT_ADAPTATION_TYPES_HEADER_GUARD
