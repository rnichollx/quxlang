//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "dependency_resolver_chain.hpp"
#include "lir_type_index.hpp"
#include <memory>

namespace rs1031
{

    class compiler
    {
        lir_type_index m_type_index;

        std::shared_ptr< resolvable< void > > m_done_scanning_files;
        std::map< symbol_address, std::shared_ptr< resolvable< symbol_address > > > m_dealias_index;

      public:
        compiler(lir_machine_info machine_info)
            : m_type_index(std::move(machine_info))
            , m_done_scanning_files(std::make_shared< resolvable< void > >(
                  []()
                  {
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
                    [addr]()
                    {
                        // TODO: Implement this
                        return addr;
                    },
                    std::vector< std::shared_ptr< resolvable_base > >{}, 1);
                m_dealias_index[addr] = new_resolv;
                return new_resolv;
            }
        }
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
