//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "dependency_resolver_chain.hpp"
#include "lir_type_index.hpp"
#include "semantic_generator.hpp"
#include <memory>

namespace rs1031
{

    class compiler
    {
        lir_type_index m_type_index;
        collector c;
        semantic_generator sg;

        std::shared_ptr< resolvable< void > > m_done_scanning_files;
        std::map< symbol_address, std::shared_ptr< resolvable< symbol_address > > > m_dealias_index;

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

      public:
        compiler(lir_machine_info machine_info)
            : m_type_index(std::move(machine_info))
            , m_done_scanning_files(std::make_shared< resolvable< void > >(
                  []()
                  {
                      return true;
                  },
                  std::vector< std::shared_ptr< resolvable_base > >{}, 1))
        {
        }

        std::shared_ptr< resolvable< symbol_address > > resolv_dealias_symbol_address(symbol_address addr)
        {
            auto it = m_dealias_index.find(addr);
            if (it != m_dealias_index.end())
            {
                return it->second;
            }
            else
            {
                auto new_resolv = std::make_shared< resolvable< symbol_address > >(
                    [addr](std::optional< symbol_address >& out) -> bool
                    {
                        // TODO: Implement this correctly
                        out = addr;
                        return true;
                    },
                    std::vector< std::shared_ptr< resolvable_base > >{m_done_scanning_files});
                m_dealias_index[addr] = new_resolv;
                return new_resolv;
            }
        }
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
