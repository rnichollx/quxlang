// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_unoptimized_llvm_spec.hpp>

#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::output_unoptimized_llvm_spec > quxlang::output_unoptimized_llvm_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));
    std::vector< rpnx::querygraph::request< llvm_preoptimize_query > > preoptimize_requests;
    preoptimize_requests.reserve(component_identities.size());
    for (llvm_output_query_input const& identity : component_identities)
    {
        preoptimize_requests.emplace_back(identity);
        co_yield rpnx::querygraph::dependency(preoptimize_requests.back());
    }

    std::string result;
    for (std::size_t index = 0; index < component_identities.size(); ++index)
    {
        llvm_backend::llvm_preoptimized_unit const& preoptimized = co_await preoptimize_requests.at(index);
        result += "; independently compiled component: " + llvm_output_component_name(component_identities.at(index)) +
            "\n" + preoptimized.llvm_ir_text + "\n";
    }
    co_return result;
}
