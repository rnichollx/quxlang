// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_IR2_CONSTEXPR_INTERPRETER_HEADER_GUARD
#define QUXLANG_VMIR2_IR2_CONSTEXPR_INTERPRETER_HEADER_GUARD

#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/constexpr.hpp"
#include "quxlang/data/constexpr_types.hpp"
#include "quxlang/data/antestatal.hpp"
#include "vmir2.hpp"
#include <map>
#include <optional>
#include <set>
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
            void set_constexpr_result_global_symbol(std::optional< type_symbol > symbol);
            void add_constexpr_antestatal_global(type_symbol symbol, type_symbol type, antestatal_value value);

            std::set< type_symbol > const & missing_functanoids();
            std::set< type_symbol > const & missing_antestatal_globals();
            void exec(type_symbol func);
            void exec3(type_symbol func);

            bool get_cr_bool();
            std::uint64_t get_cr_u64();

            constexpr_result get_cr_value();
            antestatal_value get_cr_antestatal_value();
        };
    } // namespace vmir2
} // namespace quxlang

#endif // RPNX_QUXLANG_IR2_INTERPRETER_HEADER
