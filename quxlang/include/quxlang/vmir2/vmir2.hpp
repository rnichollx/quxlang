//
// Created by Ryan Nicholl on 5/26/24.
//

#ifndef RPNX_QUXLANG_VMIR2_HEADER
#define RPNX_QUXLANG_VMIR2_HEADER

#include <cstdint>
#include <quxlang/data/type_symbol.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>
#include <quxlang/data/vm_expression.hpp>

namespace quxlang
{
    namespace vmir2
    {
        struct access_field
        {
            std::size_t base_index;
            std::size_t store_index;
            std::size_t offset;

            RPNX_MEMBER_METADATA(access_field, base_index, store_index, offset);
        };

        struct invoke
        {
            type_symbol what;
            vm_invocation_args args;

            RPNX_MEMBER_METADATA(invoke, what, args);
        };
    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2_HEADER
