//
// Created by Ryan Nicholl on 2024-12-01.
//

#ifndef RPNX_QUXLANG_IR2_INTERPRETER_HEADER
#define RPNX_QUXLANG_IR2_INTERPRETER_HEADER

#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/res/constexpr.hpp"
#include "vmir2.hpp"
namespace quxlang
{
    namespace vmir2
    {
        class ir2_constexpr_interpreter
        {
          private:
            class ir2_constexpr_interpreter_impl;
            ir2_constexpr_interpreter_impl * implementation;

          public:
            ir2_constexpr_interpreter();
            ~ir2_constexpr_interpreter();
            // Adds a class layout (for accessing fields)
            void add_class_layout(type_symbol name, class_layout layout);
            void add_functanoid3(type_symbol addr, functanoid_routine3 func);

            std::set< type_symbol > const & missing_functanoids();
            void exec(type_symbol func);
            void exec3(type_symbol func);

            bool get_cr_bool();
            std::uint64_t get_cr_u64();

            constexpr_result get_cr_value();
        };
    } // namespace vmir2
} // namespace quxlang

#endif // RPNX_QUXLANG_IR2_INTERPRETER_HEADER
