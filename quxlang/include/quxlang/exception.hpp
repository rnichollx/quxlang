// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_EXCEPTION_HEADER_GUARD
#define QUXLANG_EXCEPTION_HEADER_GUARD

#include <rpnx/macros.hpp>

#include <exception>
#include <string>
#include <utility>
#include <version>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif
#include <stdexcept>

namespace quxlang
{
#ifdef __cpp_lib_source_location
    using cxx_source_location = std::source_location;
#else
    class cxx_source_location
    {
      public:
        static cxx_source_location current()
        {
            return cxx_source_location();
        }

        std::string file_name() const
        {
            return "?";
        }

        std::string function_name() const
        {
            return "?";
        }

        int line() const
        {
            return -1;
        }
    };

#endif

    class compiler_bug : public std::logic_error
    {
      public:
        compiler_bug(std::string what_arg) : std::logic_error("Compiler Bug: " + what_arg)
        {
        }
    };

    class assert_failure : public compiler_bug
    {
      public:
        assert_failure(std::string what_arg, cxx_source_location loc = cxx_source_location::current()) : compiler_bug(std::string() + "Assert failure in " + loc.function_name() + " at " + loc.file_name() + ": " + what_arg)
        {
        }
    };

    class invalid_input_error : public std::logic_error
    {
      public:
        invalid_input_error(std::string what_arg) : std::logic_error(what_arg)
        {
        }
    };

    /**
     * Runtime failure raised while evaluating code in the constexpr interpreter.
     */
    class constexpr_runtime_error : public std::logic_error
    {
      public:
        std::string message;

        RPNX_MEMBER_METADATA(constexpr_runtime_error, message);

        constexpr_runtime_error() : std::logic_error("During constexpr evaluation: "), message("During constexpr evaluation: ")
        {
        }

        constexpr_runtime_error(std::string what_arg)
            : std::logic_error(std::string("During constexpr evaluation: ") + what_arg), message(std::string("During constexpr evaluation: ") + std::move(what_arg))
        {
        }

        auto what() const noexcept -> char const* override
        {
            return message.c_str();
        }
    };

    using constexpr_logic_execution_error = constexpr_runtime_error;
    using constexpr_assert_failure = constexpr_runtime_error;

    /** Runtime failure raised when constexpr execution reaches a PANIC terminator. */
    class constexpr_panic_failure : public constexpr_runtime_error
    {
      public:
        using constexpr_runtime_error::constexpr_runtime_error;
    };

    class invalid_instruction_error : public std::logic_error
    {
      public:
        invalid_instruction_error(std::string what_arg) : std::logic_error(what_arg)
        {
        }
    };

    class invalid_instruction_transition_error : public std::logic_error
    {
      public:
        invalid_instruction_transition_error(std::string what_arg) : std::logic_error(what_arg)
        {
        }
    };

    class invalid_goto_error : public std::logic_error
    {
      public:
        invalid_goto_error(std::string what_arg) : std::logic_error(what_arg)
        {
        }
    };
} // namespace quxlang
#endif // EXCEPTION_HPP
