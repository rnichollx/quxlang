// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functum_user_overloads_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/debug.hpp"
#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_user_overloads_spec > quxlang::functum_user_overloads_impl(type_symbol input)
{
    auto const& map = co_await rpnx::querygraph::query_request< functum_map_user_formal_ensigs_query >(input);

    std::set< temploid_ensig > results;

    for (auto const& [ensig, index] : map)
    {
        results.insert(ensig);
    }

    co_return results;
}
