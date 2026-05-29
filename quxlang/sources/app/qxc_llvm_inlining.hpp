// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_APP_QXC_LLVM_INLINING_HEADER_GUARD
#define QUXLANG_SOURCES_APP_QXC_LLVM_INLINING_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <cstddef>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace quxlang::qxc_detail
{
    inline constexpr std::size_t max_potential_llvm_inlining_depth = 2;

    using llvm_inlining_dependency_graph = std::map< type_symbol, std::set< type_symbol > >;

    /**
     * Collects helper routines that may be emitted as available_externally bodies from one root, bounded by dependency depth.
     */
    inline auto collect_potentially_inlinable_functanoids(
        llvm_inlining_dependency_graph const& dependency_graph,
        type_symbol const& root_symbol,
        std::size_t const max_depth = max_potential_llvm_inlining_depth) -> std::set< type_symbol >
    {
        std::set< type_symbol > result;
        std::vector< std::pair< type_symbol, std::size_t > > pending;
        pending.push_back(std::make_pair(root_symbol, std::size_t(0)));

        while (!pending.empty())
        {
            std::pair< type_symbol, std::size_t > current = std::move(pending.back());
            pending.pop_back();

            std::map< type_symbol, std::set< type_symbol > >::const_iterator deps_iter = dependency_graph.find(current.first);
            if (deps_iter == dependency_graph.end() || current.second >= max_depth)
            {
                continue;
            }

            for (type_symbol const& dependency : deps_iter->second)
            {
                if (!result.insert(dependency).second)
                {
                    continue;
                }
                pending.push_back(std::make_pair(dependency, current.second + 1));
            }
        }

        result.erase(root_symbol);
        return result;
    }
}

#endif // QUXLANG_SOURCES_APP_QXC_LLVM_INLINING_HEADER_GUARD
