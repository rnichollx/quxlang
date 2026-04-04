// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_assignment_exists_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::user_assignment_exists_spec > quxlang::user_assignment_exists_impl(type_symbol input)
{
    auto user_assign_en = co_await rpnx::querygraph::query_request< functum_user_overloads_query >(submember{
        .of = input,
        .name = "OPERATOR:=",
    });

    co_return !user_assign_en.empty();
}
