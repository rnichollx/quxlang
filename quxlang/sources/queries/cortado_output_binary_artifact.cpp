// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/cortado-backend.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/cortado_output_binary_artifact_spec.hpp>

rpnx::querygraph::coroutine< quxlang::cortado_output_binary_artifact_spec > quxlang::cortado_output_binary_artifact_impl(std::string input)
{
    cortado_backend::cortado_compilable_unit const& unit = co_await rpnx::querygraph::request< output_cortado_input_query >(input);
    try
    {
        co_return cortado_backend::emit_jar(unit);
    }
    catch (compilation_error const&)
    {
        throw;
    }
    catch (std::exception const& error)
    {
        throw semantic_compilation_error("Cortado classfile generation failed: " + std::string(error.what()));
    }
}
