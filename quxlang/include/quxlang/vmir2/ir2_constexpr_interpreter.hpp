// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_IR2_CONSTEXPR_INTERPRETER_HEADER_GUARD
#define QUXLANG_VMIR2_IR2_CONSTEXPR_INTERPRETER_HEADER_GUARD

#include "quxlang/data/struct_layout.hpp"
#include <quxlang/data/basic_types.hpp>
#include "quxlang/data/constexpr_types.hpp"
#include "vmir2.hpp"
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
namespace quxlang
{
    namespace vmir2
    {
        struct source_index;

        class ir2_constexpr_interpreter
        {
          private:
            class ir2_constexpr_interpreter_impl;
            ir2_constexpr_interpreter_impl * implementation;

          public:
            ir2_constexpr_interpreter();
            ~ir2_constexpr_interpreter();
            // Adds a struct layout (for accessing fields)
            void add_struct_layout(type_symbol name, struct_layout layout);
            /// Adds a nominal integer-like value type and its exact in-storage bit width.
            void add_nominal_integer_type(type_symbol name, std::uint64_t bits);
            void add_functanoid3(type_symbol addr, functanoid_routine3 func);
            /// Registers an ASM procedure invocable that may be formed but cannot be called during constexpr evaluation.
            void add_constexpr_asm_procedure(type_symbol symbol);
            void set_source_index(source_index source_index);
            /// Sets a callback that receives constexpr interpreter debug lines when debug message emission is enabled.
            void set_debug_line_handler(std::function< void(std::string) > handler);
            void set_constexpr_result_global_symbol(std::optional< type_symbol > symbol);
            void add_constexpr_antestatal_global(type_symbol symbol, type_symbol type, antestatal_value value);
            /// Adds an antestatal root with explicit mutability for constexpr localdata/static evaluation.
            void add_constexpr_antestatal_global(type_symbol symbol, type_symbol type, antestatal_value value, bool is_mutable);
            /// Adds a global storage root whose active object is recursively zero-initialized.
            void add_zero_initialized_global(type_symbol symbol, type_symbol type);

            std::set< type_symbol > const & missing_invokables();
            std::set< type_symbol > const & missing_antestatal_globals();
            void exec(type_symbol func);
            void exec3(type_symbol func);

            bool get_cr_bool();
            std::uint64_t get_cr_u64();

            constexpr_result get_cr_value();
            antestatal_value get_cr_antestatal_value();
            /// Returns every antestatal result materialized by constexpr_set_result2, keyed by result ID.
            std::map< std::uint64_t, antestatal_value > get_cr_antestatal_values();
            std::map< std::uint64_t, constexpr_serialoid > get_cr_serialoid_values();
        };
    } // namespace vmir2
} // namespace quxlang

#endif // RPNX_QUXLANG_IR2_INTERPRETER_HEADER
