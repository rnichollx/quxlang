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
                "DEFAULT_ALLOCATOR",
                "ASSERT_FAIL",
                "INITGUARD_ABORT",
                "INITGUARD_COMPLETE",
                "INITGUARD_TRY_ACQUIRE",
                "UNIT_TESTING_PROGRAM_START",
            };
        }

        static const std::set< std::string > subentity_keywords = get_subentity_keywords();

        std::set< std::string > const runtime_only_declared_symbols = {
            "ASSERT_FAIL",
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
