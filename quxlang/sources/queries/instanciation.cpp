// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/instanciation_spec.hpp>




rpnx::querygraph::coroutine< quxlang::instanciation_spec > quxlang::instanciation_impl(initialization_reference input)
{
    std::string dbg_input = to_string(input);
    type_symbol templexoid_symbol = input.initializee;

    auto kind = co_await rpnx::querygraph::query_request< symbol_type_query >(templexoid_symbol);

    if (kind == symbol_kind::functum)
    {
        co_return co_await rpnx::querygraph::query_request< functum_initialize_query >(input);
    }
    else if (kind == symbol_kind::function)
    {
        co_return co_await rpnx::querygraph::query_request< function_instanciation_query >(input);
    }
    else if (kind == symbol_kind::template_)
    {
        co_return co_await rpnx::querygraph::query_request< template_instanciation_query >(input);
    }
    else if (kind == symbol_kind::templex)
    {
        co_return co_await rpnx::querygraph::query_request< templex_initialize_query >(input);
    }
    else
    {
        co_return std::nullopt;
      //  throw std::logic_error("Cannot instanciate non-templexoid");
    }
}
