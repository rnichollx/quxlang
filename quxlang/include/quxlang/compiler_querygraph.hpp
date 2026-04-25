// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD
#define QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD

#include <quxlang/data/machine.hpp>
#include <quxlang/data/target_configuration.hpp>
#include <rpnx/querygraph/querygraph.hpp>

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>

namespace quxlang
{
    class compiler_querygraph
    {
      public:
        compiler_querygraph(source_bundle const& bundle, std::string configured_target, output_info const& machine_info,
                            std::optional< std::filesystem::path > dump_output_path = std::nullopt);
        ~compiler_querygraph();

        template < typename Query >
        auto make_request(typename Query::input_type input) -> typename Query::output_type
        {
            auto result = m_graph.make_request< Query >(std::move(input));
            write_dump_file();
            return result;
        }

        auto raw_graph() -> decltype(auto)
        {
            return (m_graph);
        }

      private:
        void write_dump_file();

        rpnx::querygraph::graph m_graph;
        std::optional< std::filesystem::path > m_dump_output_path;
        bool m_has_reported_dump_output_path = false;
    };
} // namespace quxlang

#endif // QUXLANG_COMPILER_QUERYGRAPH_HEADER_GUARD
