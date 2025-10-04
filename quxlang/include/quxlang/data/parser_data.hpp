// Copyright (c) 2025 Ryan P. Nicholl <rnicholl@protonmail.com> http://rpnx.net/


#ifndef QUXLANG_DATA_PARSER_DATA_HEADER_GUARD
#define QUXLANG_DATA_PARSER_DATA_HEADER_GUARD
#include <string>

namespace quxlang
{

    using parser_iter = std::string::iterator;
    using parser_const_iter = std::string::const_iterator;

    struct parsing_context
    {
        int version_major = 0;
        int version_minor = 0;
        int version_patch = 0;

        std::string language = "EN";

        std::uint64_t file_id = 0;

        parser_const_iter file_begin = {};
        parser_const_iter file_end = {};
    };
}

#endif //PARSER_DATA_HPP
