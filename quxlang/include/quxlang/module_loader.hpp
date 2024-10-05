// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_MODULE_LOADER_HEADER_GUARD
#define QUXLANG_MODULE_LOADER_HEADER_GUARD
#include "data/machine.hpp"

#include "data/target_configuration.hpp"

#include <filesystem>

namespace quxlang
{
    module_source load_module_source(std::filesystem::path const& path, output_info const& info);

} // namespace quxlang

#endif //MODULE_READER_HPP
