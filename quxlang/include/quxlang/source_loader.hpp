// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_SOURCE_LOADER_HEADER_GUARD
#define QUXLANG_SOURCE_LOADER_HEADER_GUARD


#include "data/machine.hpp"
#include "data/target_configuration.hpp"

#include <filesystem>
#include <set>
#include <optional>
#include <string>

namespace quxlang
{
    source_bundle load_bundle_sources_for_targets(std::filesystem::path const& path, std::optional< std::set< std::string > > configured_targets);
}

#endif //SOURCE_LOADER_HPP
