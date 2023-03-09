//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_LIR_TYPE_INDEX_HEADER
#define RPNX_RYANSCRIPT1031_LIR_TYPE_INDEX_HEADER

#include "symbol_address.hpp"
#include <cassert>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace rs1031
{
    using lir_type_id = std::size_t;
    using lir_field_id = std::size_t;

    struct lir_inherit_info
    {
        lir_type_id m_type;
        int m_privacy;
        bool m_virtual;
        std::optional< std::size_t > m_offset;
    };

    struct lir_field_info
    {
        lir_type_id m_type;
        std::optional< std::size_t > m_offset;
    };

    struct lir_type_info
    {
        symbol_address m_name;
        std::vector< lir_field_info > m_fields;
        std::vector< lir_inherit_info > m_inherits;
        std::optional< std::size_t > m_size;
        std::optional< std::size_t > m_alignment;
        bool m_sealed = false;
        bool m_finalized = false;
    };

    struct lir_machine_info
    {
        std::size_t m_pointer_size;
        std::size_t m_pointer_alignment;
    };

    struct lir_type_index
    {
        using lir_type_storage = std::vector< lir_type_info >;
        // using lir_type_storage = std::deque<lir_type_info>;

        // std::set< symbol_address > m_already_defined_types;
        std::map< symbol_address, lir_type_id > m_already_defined_types;

        lir_type_storage m_types;

        lir_machine_info m_machine_info;

      public:
        void setup_builtin_types()
        {
            for (int i = 1; i < 4; i++)
            {
                auto width = i * i * 8;
                // std::string iname = "I" + std::to_string(width);
                get_builtin_int(width);
            }
        }

        lir_type_id get_builtin_int(std::size_t width)
        {
            std::string iname = "I" + std::to_string(width);
            auto id = add_or_get_type({iname});
            m_types[id].m_size = width / 8;
            m_types[id].m_alignment = width / 8;
            m_types[id].m_sealed = true;
            m_types[id].m_finalized = true;
            return id;
        }

        lir_type_index(lir_machine_info machine_info)
            : m_machine_info{machine_info}
        {
            setup_builtin_types();
        }

        lir_type_id add_or_get_type(symbol_address addr)
        {
            auto it = m_already_defined_types.find(addr);
            if (it != m_already_defined_types.end())
            {
                return it->second;
            }
            else
            {
                return add_type(addr);
            }
        }

        lir_type_id add_type(symbol_address addr)
        {
            assert(m_already_defined_types.find(addr) == m_already_defined_types.end());
            m_types.emplace_back();
            m_types.back().m_name = addr;
            m_already_defined_types.insert({addr, m_types.size() - 1});
            return m_types.size() - 1;
        }

        lir_field_id type_add_field(lir_type_id ty, lir_type_id fieldtype)
        {
            assert(!m_types[ty].m_sealed);
            m_types[ty].m_fields.emplace_back();
            m_types[ty].m_fields.back().m_type = fieldtype;
            return m_types[ty].m_fields.size() - 1;
        }

        std::optional< std::size_t > get_type_size(lir_type_id id)
        {
            assert(m_types[id].m_sealed);
            return m_types[id].m_size;
        }

        std::optional< std::size_t > get_type_alignment(lir_type_id id)
        {
            assert(m_types[id].m_sealed);
            return m_types[id].m_alignment;
        }

        void seal_type(lir_type_id id)
        {
            assert(!m_types[id].m_sealed);
            m_types[id].m_sealed = true;
        }

        struct typeset_call_context
        {
        };

        void typeset_finalize(lir_type_id id, typeset_call_context ctx = typeset_call_context())
        {
            assert(m_types[id].m_sealed);
            if (m_types[id].m_finalized)
            {
                return;
            }

            std::size_t current_alignment = 1;

            for (lir_field_info const& field : m_types[id].m_fields)
            {
                typeset_finalize(field.m_type);
                auto field_alignment = get_type_alignment(field.m_type);
                assert(field_alignment.has_value());
                if (field_alignment > current_alignment)
                {
                    current_alignment = field_alignment.value();
                }
            }

            // Now we have the full alignment, do the naive approach of just setting the fields one after the other
            // in the order they are added

            std::size_t current_offset = 0;

            for (lir_field_info& field : m_types[id].m_fields)
            {
                std::size_t field_alignment = *get_type_alignment(field.m_type);
                std::size_t field_size = *get_type_size(field.m_type);

                std::size_t current_alignment_offset = current_offset % field_alignment;
                if (current_alignment_offset != 0)
                {
                    current_offset += field_alignment - current_alignment_offset;
                }
                field.m_offset = current_offset;

                current_offset += field_size;
            }

            m_types[id].m_size = current_offset;
        }
        std::string pretty_name(lir_type_id i)
        {
            return m_types[i].m_name.prettystring();
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_LIR_TYPE_INDEX_HEADER
