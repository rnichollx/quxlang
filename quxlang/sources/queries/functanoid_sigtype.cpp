// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/functanoid_sigtype_spec.hpp>
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::functanoid_sigtype_spec > quxlang::functanoid_sigtype_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    sigtype result;

    result.params = input.params;

    result.return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(input);

    co_return result;
}
