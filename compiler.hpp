//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "dependency_resolver_chain.hpp"
#include "lir_type_index.hpp"
#include "map_alg.hpp"
#include "semantic_generator.hpp"
#include <memory>

namespace rs1031
{

    class compiler
    {
        lir_type_index m_type_index;
        collector c;
        semantic_generator sg;

        template < typename K, typename V >
        using index = std::map< K, dep_func_ptr< V > >;

        std::shared_ptr< dependency_func_node< void > > m_done_scanning_files;
        std::map< symbol_address, std::shared_ptr< dependency_func_node< symbol_address > > > m_dealias_index;

        // Resolve typeid -> name
        index< symbol_address, lir_type_id > m_typeid_index;

        // Resolve typeid -> type information
        index< lir_type_id, std::size_t > m_size_index;
        index< lir_type_id, std::size_t > m_alignment_index;

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
            , m_done_scanning_files(std::make_shared< dependency_func_node< void > >(
                  []()
                  {
                      return true;
                  },
                  std::vector< std::shared_ptr< dependency_base > >{}, 1))
        {
        }

        std::shared_ptr< dependency_func_node< symbol_address > > step_dealias_symbol_address(symbol_address addr)
        {
            // todo : implement
            return nullptr;
        }

        dep_func_ptr< std::size_t > step_calculate_size(symbol_address addr)
        {
            // return access_or_create(m_size_index, addr,
            //                         [&]()
            //                          {
            //                              return this->create_step_calculate_size(addr);
            //                          });
            // TODO: Implement
            return nullptr;
        }

        template < typename T >
        using alg_func = typename dependency_func_node< T >::resolver_function_type;

        // This structure represents a list of field declarations that refer to types by typeid.
        using lir_field_typeid_list = std::vector< lir_field_type_decl >;

        dep_func_ptr< lir_field_typeid_list > step_resolve_member_field_type_list(lir_type_id addr);

        dep_func_ptr< std::size_t > create_step_calculate_size(lir_type_id addr)
        {

            dep_res_func< std::size_t > res_func = [this, addr](std::optional< std::size_t >& out, dep_func_ptr< std::size_t > that) -> bool
            {
                auto result = std::dynamic_pointer_cast< dependency_func_node< std::size_t > >(that);

                assert(result != nullptr);
                assert(result->is_maybe_resolvable());

                auto member_list = this->step_resolve_member_field_type_list(addr);

                if (!member_list->is_resolved())
                {
                    result->add_dependency(member_list);
                    return false;
                }

                auto rmember_list = member_list->get();
            };

            return make_dep_func< std::size_t >(res_func);
        }
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
