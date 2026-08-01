// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/exception.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/llvm_compiler_builtin_manifest_spec.hpp>

#include <map>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::llvm_compiler_builtin_manifest_spec > quxlang::llvm_compiler_builtin_manifest_impl(std::string input)
{
    std::vector< llvm_output_query_input > const& component_identities =
        co_await rpnx::querygraph::request< llvm_output_component_identities_query >(std::move(input));
    std::vector< rpnx::querygraph::request< output_llvm_input_query > > packet_requests;
    packet_requests.reserve(component_identities.size());
    for (llvm_output_query_input const& identity : component_identities)
    {
        packet_requests.emplace_back(identity);
        co_yield rpnx::querygraph::dependency(packet_requests.back());
    }

    std::map< type_symbol, type_symbol > result;
    for (rpnx::querygraph::request< output_llvm_input_query >& packet_request : packet_requests)
    {
        llvm_backend::llvm_compilable_unit const& packet = co_await packet_request;
        for (std::pair< type_symbol const, type_symbol > const& object_type : packet.object_reference_types)
        {
            if (!object_type.first.type_is< builtin_symbol >())
            {
                continue;
            }
            initialization_type const& init_type =
                co_await rpnx::querygraph::request< global_init_type_query >(object_type.first);
            if (init_type != initialization_type::init_compiler_builtin)
            {
                continue;
            }

            std::map< type_symbol, type_symbol >::iterator existing = result.find(object_type.first);
            if (existing == result.end())
            {
                result.emplace(object_type.first, object_type.second);
                continue;
            }
            if (existing->second != object_type.second)
            {
                throw compiler_bug(
                    "Compiler-builtin object has inconsistent LLVM component types: " +
                    to_string(object_type.first));
            }
        }
    }
    co_return result;
}
