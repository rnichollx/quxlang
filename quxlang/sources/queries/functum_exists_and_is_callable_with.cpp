// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_exists_and_is_callable_with_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/debug.hpp"
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_exists_and_is_callable_with_spec > quxlang::functum_exists_and_is_callable_with_impl(initialization_reference input)
{
    auto ol = co_await rpnx::querygraph::query_request< functum_initialize_query >(input);

    co_return ol.has_value();
}
