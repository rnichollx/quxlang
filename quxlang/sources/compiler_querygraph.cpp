// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_option_strings_map.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/querygraph_traits.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <fstream>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <utility>

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

quxlang::compiler_querygraph::compiler_querygraph(source_bundle const& bundle, std::string configured_target, output_info const& machine_info,
                                                  std::optional< std::filesystem::path > dump_output_path)
    : m_dump_output_path(std::move(dump_output_path))
{
    std::map< std::string, std::string > module_source_name_map;
    std::map< std::string, std::map< std::string, std::string > > module_option_strings_map;
    auto const& target_config = bundle.targets.at(configured_target);
    for (auto const& [logical_name, module_config] : target_config.module_configurations)
    {
        module_source_name_map.emplace(logical_name, module_config.source);
        module_option_strings_map.emplace(logical_name, module_config.option_values);
    }

    m_graph.register_canonical_error< compilation_error >();
    m_graph.register_canonical_error< constexpr_runtime_error >();

    m_graph.register_handler_singleton< source_bundle_query >(bundle);
    m_graph.register_handler_singleton< machine_info_query >(machine_info);
    m_graph.register_handler_singleton< module_source_name_map_query >(std::move(module_source_name_map));
    m_graph.register_handler_singleton< module_option_strings_map_query >(std::move(module_option_strings_map));

    detail::register_compiler_querygraph_handlers_1(*this);
    detail::register_compiler_querygraph_handlers_2(*this);
    detail::register_compiler_querygraph_handlers_3(*this);
    detail::register_compiler_querygraph_handlers_4(*this);
    detail::register_compiler_querygraph_handlers_5(*this);
    detail::register_compiler_querygraph_handlers_6(*this);
    detail::register_compiler_querygraph_handlers_7(*this);
    detail::register_compiler_querygraph_handlers_8(*this);
    detail::register_compiler_querygraph_handlers_9(*this);

    m_graph.bind_handlers();
}

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
