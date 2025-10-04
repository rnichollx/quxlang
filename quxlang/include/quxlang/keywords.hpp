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
        const std::string no_default_constructor = "NO_DEFAULT_CONSTRUCTOR";
        const std::string no_implicit_constructors = "NO_IMPLICIT_CONSTRUCTORS";
        const std::string no_implicit_assignment = "NO_IMPLICIT_ASSIGNMENT";
        const std::string no_builtin_copy = "NO_BUILTIN_COPY";


        std::set< std::string > const class_keywords = {
            keywords::move_only,
            keywords::not_copyable,
            keywords::no_default_constructor,
            keywords::no_implicit_constructors,
            keywords::no_implicit_assignment,
            keywords::no_builtin_copy,
        };

        inline std::set< std::string > get_subentity_keywords()
        {
            return {"CONSTRUCTOR", "DESTRUCTOR", "OPERATOR", "SERIALIZE", "DESERIALIZE", "BEGIN", "END"};
        }

        static const std::set< std::string > subentity_keywords = get_subentity_keywords();
    }
}

#endif // CLASS_KEYWORDS_HPP
