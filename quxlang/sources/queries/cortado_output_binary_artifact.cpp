// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/cortado-backend.hpp>
#include <quxlang/queries/specs/cortado_output_binary_artifact_spec.hpp>

rpnx::querygraph::coroutine< quxlang::cortado_output_binary_artifact_spec > quxlang::cortado_output_binary_artifact_impl(std::string input)
{
    cortado_backend::cortado_compilable_unit const& unit = co_await rpnx::querygraph::request< output_cortado_input_query >(input);
    co_return cortado_backend::emit_jar(unit);
}
