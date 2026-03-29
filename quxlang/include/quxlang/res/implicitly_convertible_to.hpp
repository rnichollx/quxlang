// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_IMPLICITLY_CONVERTIBLE_TO_HEADER_GUARD
#define QUXLANG_RES_IMPLICITLY_CONVERTIBLE_TO_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/res/resolver.hpp"
#include "rpnx/resolver_utilities.hpp"

namespace quxlang
{
    struct implicitly_convertible_to_query
    {
        type_symbol from;
        type_symbol to;

        RPNX_MEMBER_METADATA(implicitly_convertible_to_query, from, to);
    };

    QUX_CO_RESOLVER(implicitly_convertible_to, implicitly_convertible_to_query, bool);

    struct argument_init_query
    {
        type_symbol from;
        type_symbol to;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(argument_init_query, from, to, adaptations);
    };

    QUX_CO_RESOLVER(ensig_argument_initialize, argument_init_query, std::optional<type_symbol>);
    QUX_CO_RESOLVER(argument_initialize_by_intrinsic, argument_init_query, std::optional<type_symbol>);
    QUX_CO_RESOLVER(argument_initialize_by_template, argument_init_query, std::optional<type_symbol>);
    QUX_CO_RESOLVER(argument_initialize_by_class_conversion, argument_init_query, std::optional<type_symbol>);

    QUX_CO_RESOLVER(argument_adaptation_rank, argument_init_query, std::optional<std::size_t>);

    struct argument_adaptation_better_fit_query
    {
        type_symbol from;
        type_symbol better_to;
        type_symbol worse_to;
        allowed_adaptations adaptations = allowed_adaptations::destination_rebinding;

        RPNX_MEMBER_METADATA(argument_adaptation_better_fit_query, from, better_to, worse_to, adaptations);
    };

    QUX_CO_RESOLVER(argument_adaptation_is_better_fit, argument_adaptation_better_fit_query, bool);

    QUX_CO_RESOLVER(bindable, implicitly_convertible_to_query, bool);

    QUX_CO_RESOLVER(convertible_by_call, implicitly_convertible_to_query, std::optional<type_symbol>);

    QUX_CO_RESOLVER(bindable_by_reference_requalification, implicitly_convertible_to_query, bool);
    QUX_CO_RESOLVER(bindable_by_temporary_materialization, implicitly_convertible_to_query, bool);
    QUX_CO_RESOLVER(bindable_by_reference_objectization, implicitly_convertible_to_query, bool);
} // namespace quxlang

#endif // QUXLANG_implicitly_convertible_to_RESOLVER_HEADER_GUARD
