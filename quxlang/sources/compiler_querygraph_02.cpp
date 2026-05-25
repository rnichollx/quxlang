// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/exception.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <fstream>
#include <format>
#include <iostream>
#include <string>

auto rpnx::querygraph::debug_traits< quxlang::vmir2::functanoid_routine3 >::to_debug_string(quxlang::vmir2::functanoid_routine3 const& value) -> std::string
{
    quxlang::vmir2::assembler assembler(value);
    return assembler.to_string(value);
}

namespace
{
    auto write_marshaled_graph_dump(rpnx::querygraph::graph& graph, std::filesystem::path const& output_path) -> void
    {
        auto const marshaled_dump = graph.marshall();
        std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw quxlang::compilation_error(std::format("Failed to open dump file for writing: {}", output_path.string()));
        }

        if (!marshaled_dump.empty())
        {
            out.write(reinterpret_cast< char const* >(marshaled_dump.data()), static_cast< std::streamsize >(marshaled_dump.size()));
        }
        if (!out)
        {
            throw quxlang::compilation_error(std::format("Failed to write dump file: {}", output_path.string()));
        }
    }
} // namespace

quxlang::compiler_querygraph::~compiler_querygraph() = default;

void quxlang::compiler_querygraph::write_dump_file()
{
    if (!m_dump_output_path.has_value())
    {
        return;
    }

    write_marshaled_graph_dump(m_graph, *m_dump_output_path);
    if (!m_has_reported_dump_output_path)
    {
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "Quxlang graph dump written to: " << m_dump_output_path->string() << std::endl;
        }
        m_has_reported_dump_output_path = true;
    }
}
