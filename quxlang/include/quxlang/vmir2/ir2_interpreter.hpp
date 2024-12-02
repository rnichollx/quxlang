//
// Created by Ryan Nicholl on 2024-12-01.
//

#ifndef RPNX_QUXLANG_IR2_INTERPRETER_HEADER
#define RPNX_QUXLANG_IR2_INTERPRETER_HEADER

#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "vmir2.hpp"
namespace quxlang
{
    namespace vmir2
    {
        class ir2_interpreter
        {
          private:
                std::unordered_map<type_symbol, class_layout> class_layouts;
                std::unordered_map<type_symbol, functanoid_routine2> functanoids;
          public:
            void add_class_layout(type_symbol name, class_layout layout);
            void add_functanoid(type_symbol addr, functanoid_routine2 func);
            void exec(type_symbol func);
        };
    } // namespace vmir2
} // namespace quxlang

#endif // RPNX_QUXLANG_IR2_INTERPRETER_HEADER
