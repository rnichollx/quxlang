//
// Created by Ryan Nicholl on 10/5/2024.
//
#include "quxlang/builtin_classifier.hpp"


std::vector< quxlang::signature > quxlang::builtin_classifier::list_builtins(type_symbol func) {


    if (func.type_is<submember>())
    {
        auto & val = func.template get_as<submember>();
    }

    return {};
}