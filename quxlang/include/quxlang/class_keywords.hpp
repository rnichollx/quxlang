// Copyright (c) 2025 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef CLASS_KEYWORDS_HPP
#define CLASS_KEYWORDS_HPP

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
    }



    std::set< std::string > const class_keywords = {
        keywords::move_only,
        keywords::not_copyable,
        keywords::no_default_constructor,
        keywords::no_implicit_constructors,
        keywords::no_implicit_assignment,
        keywords::no_builtin_copy,
    };
}

#endif // CLASS_KEYWORDS_HPP
