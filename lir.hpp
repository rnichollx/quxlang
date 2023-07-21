//
// Created by Ryan Nicholl on 5/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_LIR_HEADER
#define RPNX_RYANSCRIPT1031_LIR_HEADER

#include "lookup_sequence.hpp"
#include <vector>

namespace rs1031
{
    using lir_symbol_id = std::size_t;
    using lir_type_id = std::size_t;
    using lir_field_id = std::size_t;

    struct lir_type_lookup_sequence
    {
        // TODO: Make this beter.
        static_lookup_sequence where;
    };

    struct lir_field_lookuponly_information
    {
        std::string m_name;
        lir_type_lookup_sequence m_typestring;
    };

    struct lir_field_typeonly_information
    {
        std::string m_name;
        lir_type_id m_type;
    };

    struct lir_field_full_information
    {
        lir_type_id m_type;
        std::string m_name;
        std::size_t m_offset;
    };

    struct lir_type_fields_full_information
    {
        std::vector< lir_field_full_information > m_fields;
        std::size_t m_size = 0;
        std::size_t m_align = 1;
    };

    struct lir_module_sources
    {
        std::map< std::string, std::string > m_sources;
    };

    using lir_typed_field_declaration_list = std::vector< lir_field_typeonly_information >;
    using lir_untyped_field_declaration_list = std::vector< lir_field_lookuponly_information >;
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_LIR_HEADER
