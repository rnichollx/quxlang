// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_SOURCE_LOCATION_HEADER_GUARD
#define QUXLANG_AST2_SOURCE_LOCATION_HEADER_GUARD
#include "rpnx/metadata.hpp"

namespace quxlang
{

    struct ast2_source_location
    {
        std::size_t file_id = {};
        std::size_t begin_index = {};
        std::size_t end_index = {};

        template < typename It >
        void set(It begin, It end)
        {
            // TODO: Do something here later maybe
        }

        RPNX_MEMBER_METADATA(ast2_source_location, file_id, begin_index, end_index);
    };
}

#endif //SOURCE_LOCATION_H
