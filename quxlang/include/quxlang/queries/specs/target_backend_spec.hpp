// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TARGET_BACKEND_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TARGET_BACKEND_SPEC_HEADER_GUARD

#include <quxlang/queries/target_backend.hpp>
#include <quxlang/queries/target_configuration.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct target_backend_spec
    {
        using query = target_backend_query;
        using dependencies = rpnx::typelist< target_configuration_query >;
    };

    rpnx::querygraph::coroutine< target_backend_spec > target_backend_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TARGET_BACKEND_SPEC_HEADER_GUARD
