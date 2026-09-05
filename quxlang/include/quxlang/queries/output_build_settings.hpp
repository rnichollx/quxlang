// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_OUTPUT_BUILD_SETTINGS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BUILD_SETTINGS_HEADER_GUARD
#include <quxlang/data/target_configuration.hpp>
namespace quxlang
{
    /** Concrete frontend and backend policies for one output. */
    struct output_build_settings
    {
        quxlang::build_type build_type = quxlang::build_type::release;
        quxlang::build_type llvm_build_type = quxlang::build_type::release;
        RPNX_MEMBER_METADATA(output_build_settings, build_type, llvm_build_type);
    };
    /** Resolves and validates the build policy of one configured output. */
    struct output_build_settings_query
    {
        static constexpr auto query_id = "output_build_settings";
        using input_type = std::string;
        using output_type = output_build_settings;
    };
} // namespace quxlang
#endif
