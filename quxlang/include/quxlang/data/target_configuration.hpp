// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_TARGET_CONFIGURATION_HEADER_GUARD
#define QUXLANG_DATA_TARGET_CONFIGURATION_HEADER_GUARD

#include "machine.hpp"
#include "rpnx/cow.hpp"

#include <map>
#include <memory>
#include <optional>
#include <cstdint>
#include <rpnx/macros.hpp>
#include <string>
#include <rpnx/variant.hpp>

RPNX_ENUM(quxlang, backend_kind, std::uint8_t, llvm);
RPNX_ENUM(quxlang, backend_llvm_mode, std::uint8_t, optimize, debug);
/// Controls how UNIMPLEMENTED statements are handled when VMIR is generated.
RPNX_ENUM(quxlang, unimplemented_mode, std::uint8_t, trap, error);

namespace quxlang
{
    struct source_file_name
    {
        std::string source_module;
        std::string relative_path;

        RPNX_MEMBER_METADATA(source_file_name, source_module, relative_path);
    };

    struct source_file_index
    {
        std::map< source_file_name, std::uint64_t > file_to_id;
        std::map< std::uint64_t, source_file_name > id_to_file;

        RPNX_MEMBER_METADATA(source_file_index, file_to_id, id_to_file);
    };

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

    /// backend_llvm_options contains the LLVM-specific backend options for one target.
    struct backend_llvm_options
    {
        backend_llvm_mode mode = backend_llvm_mode::optimize;

        RPNX_MEMBER_METADATA(backend_llvm_options, mode);
    };

    enum class output_kind
    {
        executable,
        shared_library,
        static_library,
        image,
        unit_test_suite,
    };

    /// output_config contains the configuration for one named target output.
    struct output_config
    {
        output_kind type;
        std::optional< std::string > module;
        std::optional< std::string > main_functanoid;
        std::optional< backend_llvm_options > llvm_options;

        RPNX_MEMBER_METADATA(output_config, type, module, main_functanoid, llvm_options);
    };

    /// target_configuration contains all compile options for one configured qxc target.
    struct target_configuration
    {
        std::map< std::string, module_configuration > module_configurations;
        // Key=LogicalModuleName Value=SourceModuleName
        //std::map< std::string, std::string > logical_module_mappings;
        machine_target_info target_output_config;
        backend_kind backend = backend_kind::llvm;
        backend_llvm_options llvm_options;
        /// How codegen handles a reached UNIMPLEMENTED statement for this target.
        quxlang::unimplemented_mode unimplemented_mode = quxlang::unimplemented_mode::trap;
        bool run_static_tests = true;

        std::optional< std::map< std::string, output_config > > outputs;

        RPNX_MEMBER_METADATA(target_configuration, module_configurations, target_output_config, backend, llvm_options, unimplemented_mode, run_static_tests, outputs);
    };


    struct source_bundle
    {
        std::map< std::string, target_configuration > targets;
        std::map< std::string, module_source > module_sources;

        RPNX_MEMBER_METADATA(source_bundle, targets, module_sources);
    };

} // namespace quxlang

#endif // QUXLANG_SOURCE_BUNDLE_HPP
