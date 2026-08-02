// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CPU_ATTRIBUTES_HPP
#define QUXLANG_CPU_ATTRIBUTES_HPP

#include <quxlang/data/machine.hpp>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace quxlang
{
    /// Contains the registered architectural feature and performance attribute names for one CPU.
    struct cpu_attribute_catalog
    {
        std::set< std::string > stable_features;
        std::set< std::string > stable_perf;
        std::set< std::string > stable_vendors;
        std::set< std::string > experimental_features;
        std::set< std::string > experimental_perf;
    };

    /// Canonical CPU attribute catalogs keyed by Quxlang CPU identifier.
    extern std::map< cpu, cpu_attribute_catalog > const cpu_attributes;

    /** Contains the CPU family and individual attributes represented by one aggregate attribute. */
    struct cpu_attribute_group
    {
        /// CPU family to which the aggregate applies.
        cpu cpu_type;
        /// Complete stable stems whose conjunction defines the aggregate.
        std::set< std::string > attributes;
    };

    /** Canonical aggregate CPU attributes keyed by their complete stable stems. */
    extern std::map< std::string, cpu_attribute_group > const cpu_attribute_groups;

    /// Backend-neutral CPU tuning models accepted by stepping configurations.
    enum class cpu_tuning_model
    {
        x86_amd_k6,
        x86_amd_k6_2,
        x86_amd_athlon,
        x86_amd_athlon_xp,
        x86_amd_geode,
        x64_amd_k8,
        x64_amd_k10,
        x64_amd_bobcat,
        x64_amd_jaguar,
        x64_amd_bulldozer,
        x64_amd_piledriver,
        x64_amd_steamroller,
        x64_amd_excavator,
        x64_amd_zen1,
        x64_amd_zen2,
        x64_amd_zen3,
        x64_amd_zen4,
        x64_amd_zen5,
        x64_intel_haswell,
        x64_intel_skylake,
        x64_intel_skylake_avx512,
        x64_intel_icelake_client,
        x64_intel_icelake_server,
        x64_intel_alderlake,
        x64_intel_sapphire_rapids,
        x64_intel_granite_rapids,
        arm_apple_m1,
        arm_apple_m2,
        arm_apple_m4,
        arm_apple_m5,
    };

    /// Associates one public tuning identifier with its CPU family and model.
    struct cpu_tuning_model_entry
    {
        cpu cpu_type;
        cpu_tuning_model model;
    };

    /// Canonical CPU tuning models keyed by their public Quxlang identifiers.
    extern std::map< std::string, cpu_tuning_model_entry > const cpu_tuning_models;

    /** Resolves one public, backend-neutral CPU tuning identifier. */
    auto parse_cpu_tuning_model(std::string_view name) -> std::optional< cpu_tuning_model_entry >;

    /** Returns whether a stable CPU attribute stem represents an aggregate group. */
    auto is_cpu_attribute_group(std::string_view stem) -> bool;

    /**
     * Resolves a stable canonical CPU attribute stem such as X64_FEATURE_AVX2.
     *
     * The returned string contains the category and attribute components, such
     * as FEATURE_AVX2. Experimental and unknown attributes are not resolved.
     */
    auto parse_cpu_attribute_stem(std::string_view stem) -> std::optional< std::pair< cpu, std::string > >;

    /// Returns the canonical stem for a previously resolved stable CPU attribute.
    auto format_cpu_attribute_stem(cpu cpu_type, std::string_view attribute) -> std::string;

    /// Returns true when name is a stable CPU attribute builtin ending in _ENABLED.
    auto is_cpu_attribute_enabled_name(std::string_view name) -> bool;
}

#endif // QUXLANG_CPU_ATTRIBUTES_HPP
