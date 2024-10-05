// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BUILTIN_CLASSIFIER_HEADER_GUARD
#define QUXLANG_BUILTIN_CLASSIFIER_HEADER_GUARD
#include "data/machine.hpp"
#include "data/type_symbol.hpp"

namespace quxlang
{

    class builtin_classifier
    {
         output_info machine_info_;
    public:
        builtin_classifier(output_info const & m) : machine_info_(m) {}

        std::vector<signature> list_builtins(type_symbol func);

        std::vector<signature> list_constructor_builtins(type_symbol of_type);

    };
}


#endif //BUILTIN_CLASSIFIER_H
