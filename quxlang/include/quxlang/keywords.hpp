// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_KEYWORDS_HEADER_GUARD
#define QUXLANG_KEYWORDS_HEADER_GUARD

#include <set>
#include <string>

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
                "OPERATOR",
                "SERIALIZE",
                "DESERIALIZE",
                "BEGIN",
                "END",
                "LESS",
                "EQUAL",
                "GREATER",
                "DEFAULT_ALLOCATOR",
                "ASSERT_FAIL",
                "PANIC",
                "INITGUARD_ABORT",
                "INITGUARD_COMPLETE",
                "INITGUARD_TRY_ACQUIRE",
                "UNIT_TESTING_PROGRAM_START",
            };
        }

        static const std::set< std::string > subentity_keywords = get_subentity_keywords();

        std::set< std::string > const runtime_only_declared_symbols = {
            "ASSERT_FAIL",
            "PANIC",
            "DEFAULT_ALLOCATOR",
            "INITGUARD_ABORT",
            "INITGUARD_COMPLETE",
            "INITGUARD_TRY_ACQUIRE",
            "PROGRAM_START",
            "UNIT_TESTING_PROGRAM_START",
        };
    }
}

#endif // CLASS_KEYWORDS_HPP
