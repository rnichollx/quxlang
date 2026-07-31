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
        std::set< std::string > experimental_features;
        std::set< std::string > experimental_perf;
    };

    /// Canonical CPU attribute catalogs keyed by Quxlang CPU identifier.
    extern std::map< cpu, cpu_attribute_catalog > const cpu_attributes;

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
