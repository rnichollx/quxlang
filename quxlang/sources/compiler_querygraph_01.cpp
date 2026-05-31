// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "compiler_querygraph_internal.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_option_strings_map.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/target_configuration.hpp>

#include <map>
#include <string>
#include <utility>

quxlang::compiler_querygraph::compiler_querygraph(source_bundle const& bundle, std::string configured_target, machine_target_info const& machine_info,
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
    m_graph.register_handler_singleton< target_configuration_query >(target_config);
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
    detail::register_compiler_querygraph_handlers_10(*this);
    detail::register_compiler_querygraph_handlers_11(*this);
    detail::register_compiler_querygraph_handlers_12(*this);
    detail::register_compiler_querygraph_handlers_13(*this);
    detail::register_compiler_querygraph_handlers_14(*this);
    detail::register_compiler_querygraph_handlers_15(*this);
    detail::register_compiler_querygraph_handlers_16(*this);
    detail::register_compiler_querygraph_handlers_17(*this);
    detail::register_compiler_querygraph_handlers_18(*this);

    m_graph.bind_handlers();
}
