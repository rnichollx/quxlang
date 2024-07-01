//
// Created by Ryan Nicholl on 5/26/24.
//

#ifndef RPNX_QUXLANG_VMIR2_HEADER
#define RPNX_QUXLANG_VMIR2_HEADER

#include <cstdint>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/vm_expression.hpp>
#include <rpnx/metadata.hpp>
#include <rpnx/variant.hpp>
#include <string>
#include <vector>

namespace quxlang
{
    namespace vmir2
    {
        struct access_field;
        struct invoke;
        struct make_reference;

        using vm_instruction = rpnx::variant< access_field, invoke, make_reference >;

        using storage_index = std::size_t;

        struct invocation_args
        {
            std::vector< storage_index > positional;
            std::map< std::string, storage_index > named;

            inline auto size() const
            {
                return positional.size() + named.size();
            }

            RPNX_MEMBER_METADATA(invocation_args, positional, named);
        };

        struct access_field
        {
            storage_index base_index = 0;
            storage_index store_index = 0;
            storage_index offset = 0;

            RPNX_MEMBER_METADATA(access_field, base_index, store_index, offset);
        };

        struct invoke
        {
            type_symbol what;
            invocation_args args;

            RPNX_MEMBER_METADATA(invoke, what, args);
        };

        struct make_reference
        {
            type_symbol what;
            storage_index value_index;
            storage_index reference_index;

            RPNX_MEMBER_METADATA(make_reference, what, value_index, reference_index);
        };

        struct vm_slot
        {
            type_symbol type;
            std::optional< std::string > name;
            std::optional< std::string > literal_value;
        };

        struct vm_context
        {
            std::vector< vm_slot > slots;
        };

        struct functanoid_routine
        {
            std::vector< vm_slot > slots;
            std::vector< vm_instruction > instructions;
        };
    } // namespace vmir2

}; // namespace quxlang

#endif // RPNX_QUXLANG_VMIR2_HEADER
