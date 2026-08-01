// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_optimized_llvm_spec.hpp>

#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::output_optimized_llvm_spec > quxlang::output_optimized_llvm_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));
    std::vector< rpnx::querygraph::request< llvm_postoptimize_query > > postoptimize_requests;
    postoptimize_requests.reserve(component_identities.size());
    for (llvm_output_query_input const& identity : component_identities)
    {
        postoptimize_requests.emplace_back(identity);
        co_yield rpnx::querygraph::dependency(postoptimize_requests.back());
    }

    std::string result;
    for (std::size_t index = 0; index < component_identities.size(); ++index)
    {
        llvm_backend::llvm_postoptimized_unit const& postoptimized = co_await postoptimize_requests.at(index);
        result += "; independently compiled component: " + llvm_output_component_name(component_identities.at(index)) +
            "\n" + postoptimized.llvm_ir_text + "\n";
    }
    co_return result;
}
