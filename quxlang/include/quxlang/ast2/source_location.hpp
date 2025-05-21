//
// Created by rnicholl on 5/21/25.
//

#ifndef QUXLANG_SOURCE_LOCATION_HPP
#define QUXLANG_SOURCE_LOCATION_HPP
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
