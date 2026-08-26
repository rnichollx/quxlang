// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/llvm_compiled_output_spec.hpp>

#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::llvm_compiled_output_spec > quxlang::llvm_compiled_output_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));

    std::vector< rpnx::querygraph::request< llvm_preoptimize_query > > preoptimize_requests;
    std::vector< rpnx::querygraph::request< llvm_postoptimize_query > > postoptimize_requests;
    std::vector< rpnx::querygraph::request< llvm_post_codegen_query > > post_codegen_requests;
    preoptimize_requests.reserve(component_identities.size());
    postoptimize_requests.reserve(component_identities.size());
    post_codegen_requests.reserve(component_identities.size());
    for (llvm_output_query_input const& identity : component_identities)
    {
        preoptimize_requests.emplace_back(identity);
        postoptimize_requests.emplace_back(identity);
        post_codegen_requests.emplace_back(identity);
        co_yield rpnx::querygraph::dependency(preoptimize_requests.back());
        co_yield rpnx::querygraph::dependency(postoptimize_requests.back());
        co_yield rpnx::querygraph::dependency(post_codegen_requests.back());
    }

    llvm_compiled_output result;
    result.objects.reserve(post_codegen_requests.size());
    for (std::size_t index = 0; index < post_codegen_requests.size(); ++index)
    {
        auto preopt = co_await preoptimize_requests.at(index);
        auto postopt =co_await postoptimize_requests.at(index);
        auto postcg = co_await post_codegen_requests.at(index);
        result.objects.push_back(llvm_output_object{
            .identity = component_identities.at(index),
            .preoptimized = std::move(preopt),
            .postoptimized = std::move(postopt),
            .post_codegen = std::move(postcg),
        });
    }
    co_return result;
}
