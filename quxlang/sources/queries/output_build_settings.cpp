// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/output_build_settings_spec.hpp>
rpnx::querygraph::coroutine< quxlang::output_build_settings_spec > quxlang::output_build_settings_impl(std::string input)
{
    co_await rpnx::querygraph::request< output_binary_information_query >(input);
    target_configuration const& target = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    output_config const& output = bundle.outputs.at(input);
    std::optional< quxlang::build_type > llvm_override;
    if (output.llvm_options.has_value())
    {
        llvm_override = output.llvm_options->build_type;
    }
    if (!llvm_override.has_value() && output.build_type.has_value() && target.llvm_options.build_type.has_value() && output.build_type != target.llvm_options.build_type)
    {
        throw semantic_compilation_error("Output '" + input + "' build_type conflicts with target LLVM build_type; specify backend_llvm_options.build_type on the output");
    }
    quxlang::build_type selected = output.build_type.value_or(target.build_type);
    co_return output_build_settings{selected, llvm_override.value_or(target.llvm_options.build_type.value_or(selected))};
}
