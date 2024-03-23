// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/compiler.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include "quxlang/source_loader.hpp"
#include "rpnx/value.hpp"

#include <filesystem>
#include <iostream>

#include <yaml-cpp/yaml.h>

int main(int argc, char** argv)
{
    bool verbose = true;
    if (argc < 3)
    {
        throw std::runtime_error("Usage: qxc <input directory> <output directory> [targets,...]");
    }

    std::filesystem::path input = argv[1];
    std::filesystem::path output = argv[2];

    quxlang::source_bundle input_srcs = quxlang::load_bundle_sources_for_targets(input, std::nullopt);

    for (auto const &[target_name, target_config] : input_srcs.targets)
    {
        if (verbose)
        {
            std::cout << "Compiling Target: " << target_name << std::endl;
        }

        quxlang::compiler c(input_srcs, target_name);

        for (auto const & [ output_name, output_config ] : target_config.outputs)
        {
            if (verbose)
            {
                std::cout << "Compiling: " << target_name << "/" << output_name << std::endl;
            }

            std::string main_module_name = output_config.module.value_or("::main@()");
            std::string main_function_name = output_config.main_functanoid.value_or("::main@()");

            auto mainfunc_sym = quxlang::parsers::parse_type_symbol(main_function_name);

            mainfunc_sym = quxlang::with_context(mainfunc_sym, quxlang::module_reference{main_module_name});

            auto proc = c.get_vm_procedure_from_canonical_functanoid(mainfunc_sym);

        }

    }



}