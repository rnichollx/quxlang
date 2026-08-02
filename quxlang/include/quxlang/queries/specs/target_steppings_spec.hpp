// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TARGET_STEPPINGS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TARGET_STEPPINGS_SPEC_HEADER_GUARD

#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_steppings.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph specification for resolving the active target's CPU steppings. */
    struct target_steppings_spec
    {
        using query = target_steppings_query;
        using dependencies = rpnx::typelist< target_configuration_query >;
    };

    /** Resolves configured target steppings or supplies the machine target's default stepping set. */
    rpnx::querygraph::coroutine< target_steppings_spec > target_steppings_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TARGET_STEPPINGS_SPEC_HEADER_GUARD
