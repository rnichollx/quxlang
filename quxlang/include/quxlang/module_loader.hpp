// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_MODULE_READER_HPP
#define QUXLANG_MODULE_READER_HPP
#include "data/machine.hpp"

#include "data/target_configuration.hpp"

#include <filesystem>

namespace quxlang
{
    module_source load_module_source(std::filesystem::path const& path, output_info const& info);

} // namespace quxlang

#endif //MODULE_READER_HPP
