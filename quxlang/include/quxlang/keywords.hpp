// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_KEYWORDS_HEADER_GUARD
#define QUXLANG_KEYWORDS_HEADER_GUARD

#include <set>
#include <string>
#include <string_view>

namespace quxlang
{
    namespace keywords
    {
        const std::string move_only = "MOVE_ONLY";
        const std::string not_copyable = "NOT_COPYABLE";
        const std::string no_implicit_default_constructor = "NO_IMPLICIT_DEFAULT_CONSTRUCTOR";
        const std::string no_implicit_constructors = "NO_IMPLICIT_CONSTRUCTORS";
        const std::string no_implicit_assignment = "NO_IMPLICIT_ASSIGNMENT";
        const std::string no_implicit_copy = "NO_IMPLICIT_COPY";
        const std::string antestatal = "ANTESTATAL";
        const std::string serialoid = "SERIALOID";
        const std::string nonstatic = "NONSTATIC";
        const std::string stringlike = "STRINGLIKE";
        const std::string never_valueless = "NEVER_VALUELESS";
        const std::string valueless_default = "VALUELESS_DEFAULT";
        const std::string no_default_copy = "NO_DEFAULT_COPY";
        const std::string no_default_move = "NO_DEFAULT_MOVE";
        const std::string no_default_assign = "NO_DEFAULT_ASSIGN";
        const std::string no_default_swap = "NO_DEFAULT_SWAP";


        std::set< std::string > const struct_keywords = {
            keywords::move_only,
            keywords::not_copyable,
            keywords::no_implicit_default_constructor,
            keywords::no_implicit_constructors,
            keywords::no_implicit_assignment,
            keywords::no_implicit_copy,
            keywords::antestatal,
            keywords::serialoid,
            keywords::nonstatic,
            keywords::stringlike,
        };

        /// Keywords accepted between a fusion declaration keyword and its body.
        std::set< std::string > const fusion_keywords = {
            keywords::never_valueless,
            keywords::valueless_default,
            keywords::no_default_copy,
            keywords::no_default_move,
            keywords::no_default_assign,
            keywords::no_default_swap,
        };

        inline std::set< std::string > get_subentity_keywords()
        {
            return {
                "CONSTRUCTOR",
                "DESTRUCTOR",
                "DEINIT",
                "OPERATOR",
                "SERIALIZE",
                "DESERIALIZE",
                "BEGIN",
                "END",
                "INDEX",
                "VALUE",
                "ENTRY",
                "INDEXES",
                "VALUES",
                "IV_PAIRS",
                "LESS",
                "EQUAL",
                "GREATER",
                "DEFAULT_ALLOCATOR",
                "ASSERT_FAIL",
                "CHECK_STACK",
                "PANIC",
                "INITGUARD_ABORT",
                "INITGUARD_COMPLETE",
                "INITGUARD_TRY_ACQUIRE",
                "THREAD_INITGUARD_TRY_ACQUIRE",
                "THREAD_DESTRUCTOR_REGISTER",
                "THREAD_RUNTIME_START",
                "THREAD_FINISH",
                "POST_DETECT",
                "UNIT_TEST_MAIN",
            };
        }

        static const std::set< std::string > subentity_keywords = get_subentity_keywords();

        std::set< std::string > const runtime_only_declared_symbols = {
            "ASSERT_FAIL",
            "CHECK_STACK",
            "PANIC",
            "DEFAULT_ALLOCATOR",
            "INITGUARD_ABORT",
            "INITGUARD_COMPLETE",
            "INITGUARD_TRY_ACQUIRE",
            "THREAD_INITGUARD_TRY_ACQUIRE",
            "THREAD_DESTRUCTOR_REGISTER",
            "THREAD_RUNTIME_START",
            "THREAD_FINISH",
            "POST_DETECT",
            "UNIT_TEST_MAIN",
            "PROGRAM_START",
        };

        /** Returns true when a compiler-provided object name exposes only constant references. */
        inline auto is_readonly_compiler_object_name(std::string_view name) -> bool
        {
            return name == "MAIN_FUNCTION_ARRAY" ||
                   name == "POST_DETECT_FUNCTION_ARRAY" ||
                   name == "STEPPING_COUNT" ||
                   name == "UNIT_TEST_COUNT" ||
                   name == "UNIT_TEST_NAMES" ||
                   name == "UNIT_TEST_PROC";
        }
    }
}

#endif // CLASS_KEYWORDS_HPP
