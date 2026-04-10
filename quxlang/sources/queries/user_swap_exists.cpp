// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_swap_exists_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::user_swap_exists_spec > quxlang::user_swap_exists_impl(type_symbol input)
{
    auto ctor_symbol = submember{.of = input, .name = "OPERATOR<->"};

    auto input_str = quxlang::to_string(input);

    auto user_defined_ctor = co_await rpnx::querygraph::request< functum_user_overloads_query >(ctor_symbol);
    if (user_defined_ctor.empty())
    {
        co_return false;
    }

    co_return true;
}
