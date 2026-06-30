// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>
// This file implements an on-demand lexer for Quxlang
// We use an on-demand lexer to reduce memory bandwidth consumption
// TODO: This file is a WIP
#ifndef QUXLANG_LEXER_HPP
#define QUXLANG_LEXER_HPP
#include <cstdint>
#include <rpnx/macros.hpp>

RPNX_ENUM(quxlang::parsers, lexeme_kind, std::uint16_t,
         null_token,
         end_of_file,
         identifier,
         keyword_after,
         keyword_aligned_storage,
         keyword_arch_is_arm32,
         keyword_arch_is_arm64,
         keyword_arch_is_riscv64,
         keyword_arch_is_x64,
         keyword_arch_is_x86,
         keyword_arm,
         keyword_as,
         keyword_asm_procedure,
         keyword_assert,
         keyword_assume,
         keyword_at,
         keyword_auto,
         keyword_begin,
         keyword_bits,
         keyword_bool,
         keyword_break,
         keyword_builtin,
         keyword_byte,
         keyword_callable,
         keyword_callconv,
         keyword_ccall,
         keyword_checked,
         keyword_class,
         keyword_const,
         keyword_constexpr,
         keyword_constexpr_readable,
         keyword_constexpr_readwrite,
         keyword_constructor,
         keyword_continue,
         keyword_cstring_constant,
         keyword_data_constant,
         keyword_default,
         keyword_defaulted,
         keyword_default_from,
         keyword_default_value,
         keyword_deserialize,
         keyword_destroy,
         keyword_destructor,
         keyword_doc,
         keyword_if,
         keyword_else,
         keyword_enable_if,
         keyword_end,
         keyword_expect_compilation_failure,
         keyword_expect_fail,
         keyword_explicit,
         keyword_external,
         keyword_false,
         keyword_for,
         keyword_function,
         keyword_goto,
         keyword_i,
         keyword_ignored,
         keyword_import,
         keyword_include_if,
         keyword_initguard,
         keyword_initguard_lock,
         keyword_input_iter,
         keyword_is_integral,
         keyword_is_signed,
         keyword_kernel_bsd,
         keyword_kernel_linux,
         keyword_kernel_nt,
         keyword_kernel_xnu,
         keyword_label,
         keyword_linkname,
         keyword_loop,
         keyword_module,
         keyword_move_only,
         keyword_mut,
         keyword_namespace,
         keyword_native,
         keyword_new,
         keyword_noexcept,
         keyword_not_copyable,
         keyword_no_implicit_assignment,
         keyword_no_implicit_constructors,
         keyword_no_implicit_copy,
         keyword_no_implicit_default_constructor,
         keyword_no_message,
         keyword_nullptr,
         keyword_number,
         keyword_numeric_constant,
         keyword_numeric_literal,
         keyword_operator,
         keyword_option,
         keyword_os_bsd,
         keyword_os_linux,
         keyword_os_macos,
         keyword_os_windows,
         keyword_other,
         keyword_output_iterator,
         keyword_pack_arg,
         keyword_pack_arg_type,
         keyword_pack_size,
         keyword_partial,
         keyword_place,
         keyword_poison,
         keyword_procedure,
         keyword_procedure_ref,
         keyword_pun,
         keyword_return,
         keyword_rhs,
         keyword_runtime,
         keyword_same_types,
         keyword_serialize,
         keyword_sizeof,
         keyword_snapshot,
         keyword_static,
         keyword_static_choose,
         keyword_static_else,
         keyword_static_eval,
         keyword_static_if,
         keyword_static_test,
         keyword_static_var,
         keyword_static_while,
         keyword_stdcall,
         keyword_storage,
         keyword_string,
         keyword_string_constant,
         keyword_struct,
         keyword_sz,
         keyword_t,
         keyword_target,
         keyword_temp,
         keyword_template,
         keyword_this,
         keyword_true,
         keyword_tt,
         keyword_type,
         keyword_u,
         keyword_unimplemented,
         keyword_unspecified,
         keyword_unit_test,
         keyword_value,
         keyword_var,
         keyword_void,
         keyword_while,
         keyword_write
         );

namespace quxlang::parsers
{
    using parse_iterator = std::string::const_iterator;
    struct lexeme
    {
        std::string::const_iterator token_begin;
        std::string::const_iterator token_end;
        lexeme_kind kind;
    };


    struct parsing_context;

    class lexer
    {

        static constexpr std::size_t buffer_mask = 0b1111111;
        static constexpr std::size_t buffer_size = 128;
        std::array<lexeme, buffer_size> m_lexeme_buffer;
        std::size_t m_current_iter_pos;
        std::size_t m_parsed_count;

        std::string::const_iterator m_data_start;
        std::string::const_iterator m_data_end;

        lexeme & lexeme_at(ssize_t index)
        {
            return m_lexeme_buffer[(size_t(m_current_iter_pos) + static_cast< size_t >(index)) & buffer_mask];
        }

        lexeme const & lexeme_at(ssize_t index) const
        {
            return m_lexeme_buffer[(size_t(m_current_iter_pos) + static_cast< size_t >(index)) & buffer_mask];
        }

    public:
        lexeme const & peek_next()
        {
            if (m_parsed_count == 0)
            {
                scan_forward(64);
            }

            return lexeme_at(0);
        }

        std::string::const_iterator current_pos_iter() const
        {
            return lexeme_at(m_parsed_count).token_end;
        }

        void scan_forward(std::size_t size)
        {
            for (std::size_t i = 0; i < size; ++i)
            {
                scan_one();
            }
        }

        void scan_one()
        {
            auto it = current_pos_iter();



            while ( it != m_data_end && (*it == ' ' || *it == '\t' || *it == '\r' || *it == '\n'))
            {
                // skip whitespace
                it++;
            }

            if (it == m_data_end)
            {
                m_parsed_count++;
                lexeme_at(m_parsed_count) = lexeme{.token_begin = it, .token_end = it, .kind = lexeme_kind::end_of_file};
                return;
            }
        }

    };
}

#endif //QUXLANG_LEXER_HPP
