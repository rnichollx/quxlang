// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::functanoid_sigtype_spec > quxlang::functanoid_sigtype_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    sigtype result;

    auto concrete_params = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(input);
    result.params = invotype_from_instatype(concrete_params);

    result.return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(input);

    co_return result;
}
