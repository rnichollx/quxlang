// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/convertible_by_call_spec.hpp>

rpnx::querygraph::coroutine< quxlang::convertible_by_call_spec > quxlang::convertible_by_call_impl(implicitly_convertible_to_input input)
{
    co_return co_await rpnx::querygraph::request< argument_initialize_by_class_conversion_query >(argument_init_input{
                                                        .from = input.from,
                                                        .to = input.to,
                                                        .adaptations = allowed_adaptations::destination_rebinding,
                                                    });
}
