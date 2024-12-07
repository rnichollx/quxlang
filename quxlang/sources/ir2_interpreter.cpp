//
// Created by Ryan Nicholl on 2024-12-01.
//

#include "quxlang/vmir2/ir2_interpreter.hpp"

void quxlang::vmir2::ir2_interpreter::add_class_layout(quxlang::type_symbol name, quxlang::class_layout layout)
{
    class_layouts[name] = layout;
}
void quxlang::vmir2::ir2_interpreter::add_functanoid(quxlang::type_symbol addr, quxlang::vmir2::functanoid_routine2 func)
{
    functanoids[addr] = func;
}
