//
// Created by Ryan Nicholl on 12/15/2024.
//

#ifndef QUXLANG_EXCEPTION_HPP
#define QUXLANG_EXCEPTION_HPP

#include <exception>
#include <version>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif
#include <stdexcept>

namespace quxlang
{
#ifdef __cpp_lib_source_location
    using source_location = std::source_location;

#else
    class source_location
    {
      public:
        static source_location current()
        {
            return source_location();
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
        compiler_bug(std::string what_arg) : std::logic_error("Compiler Bug:" + what_arg)
        {
        }
    };

    class assert_failure : public compiler_bug
    {
      public:
        assert_failure(std::string what_arg, source_location loc = source_location::current()) : compiler_bug(std::string() + "Assert failure in " + loc.function_name() + " at " + loc.file_name() + ": " + what_arg)
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

    class syntax_error : public invalid_input_error
    {
      public:
        syntax_error(std::string what_arg) : invalid_input_error(what_arg)
        {
        }
    };

    class semantic_error : public invalid_input_error
    {
      public:
        semantic_error(std::string what_arg) : invalid_input_error(what_arg)
        {
        }
    };

    class recursion_error : public semantic_error
    {
      public:
        recursion_error(std::string what_arg) : semantic_error(what_arg)
        {
        }
    };

    class constexpr_logic_execution_error : public std::logic_error
    {
      public:
        constexpr_logic_execution_error(std::string what_arg) : std::logic_error("During constexpr evaluation: " + what_arg)
        {
        }
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
} // namespace quxlang
#endif // EXCEPTION_HPP
