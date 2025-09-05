// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_TARGET_CONFIGURATION_HEADER_GUARD
#define QUXLANG_DATA_TARGET_CONFIGURATION_HEADER_GUARD

#include "machine.hpp"
#include "rpnx/cow.hpp"

#include <map>
#include <memory>
#include <optional>
#include <rpnx/metadata.hpp>
#include <string>
#include <rpnx/variant.hpp>

namespace quxlang
{
    struct source_file
    {
        std::string contents;

        RPNX_MEMBER_METADATA(source_file, contents);
    };

    struct module_source
    {
        // We use a shared pointer to the source file object because there are likely
        // multiple targets (e.g. x64, x86, arm, etc.) that will use the same source file.
        // Currently this isn't actually leveraged, (aggregation happens outside the graph
        // layers) but it's added so in the future we can load the file once in some sort
        // of ram cache.
        // TODO: Reuse the same source file object for multiple targets.
        std::map< std::string, rpnx::cow< source_file > > files;

        RPNX_MEMBER_METADATA(module_source, files);
    };

    /// module_configuration contains the configuration for a module within a given target.
    struct module_configuration
    {
        // The source maps a logical module to the source module directory.
        // For example, if the target module is "boost", the source might be "boost_1_75_0".
        // This allows multiple verisons of a module to be used in the same project
        // for different targets.
        std::string source;

        // import_mappings is a map from the imported name to the logical module name.
        // Key=ImportedName Value=LogicalModuleName
        // Import mappings allows external import renaming. This is not the same as the
        // internal import renaming. Suppose for example there are two libraries, library1
        // and library2, and they both depend on foolib, but different incompatible versions
        // of foolib. We could create a foolib1 and foolib2 logical module, and then use
        // import mappings to map foolib1 to the correct version of foolib for library1, and
        // foolib2 to the correct version of foolib for library2.
        // In this case, the key is the import name, and the value is the logical module name.
        // Example, if the module uses "IMPORT foolib;"  the mapping could be:
        // "foolib" -> "foolib1"
        std::map< std::string, std::string > import_mappings;

        std::map< std::string, std::string > option_values;

        RPNX_MEMBER_METADATA(module_configuration, source, import_mappings, option_values);
    };

    enum class output_kind
    {
        executable,
        shared_library,
        static_library,
        image,
    };

    struct output_config
    {
        output_kind type;
        std::optional<std::string> module;
        std::optional<std::string> main_functanoid;
    };


    struct target_configuration
    {
        std::map< std::string, module_configuration > module_configurations;
        // Key=LogicalModuleName Value=SourceModuleName
        //std::map< std::string, std::string > logical_module_mappings;
        output_info target_output_config;

        std::map< std::string, output_config > outputs;
    };


    struct source_bundle
    {
        std::map< std::string, target_configuration > targets;
        std::map< std::string, module_source > module_sources;

    };

} // namespace quxlang

#endif // QUXLANG_SOURCE_BUNDLE_HPP