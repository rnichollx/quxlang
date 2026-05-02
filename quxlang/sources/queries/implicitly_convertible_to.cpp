// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/implicitly_convertible_to_spec.hpp>

rpnx::querygraph::coroutine< quxlang::implicitly_convertible_to_spec > quxlang::implicitly_convertible_to_impl(implicitly_convertible_to_input input)
{
    auto adapted = co_await rpnx::querygraph::request< ensig_argument_initialize_query >(argument_init_input{
                                                                      .from = input.from,
                                                                      .to = input.to,
                                                                      .adaptations = allowed_adaptations::destination_rebinding,
                                                                  });
    co_return adapted.has_value();
}
