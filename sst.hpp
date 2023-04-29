//
// Created by Ryan Nicholl on 2/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_SST_HEADER
#define RPNX_RYANSCRIPT1031_SST_HEADER
#include "ast.hpp"
#include <map>

namespace rs1031
{
    template < typename T >
    bool sst_resolved(T const&);

    struct sst_null_object
    {
        inline std::string to_string()
        {
            return "sst_null_object{}";
        }
    };

    template <>
    bool sst_resolved< sst_null_object >(sst_null_object const&)
    {
        return false;
    }

    struct sst_class;
    struct sst_function;

    struct sst_type_reference : value< std::variant< sst_null_object, ast_type_ref, static_lookup_sequence > >
    {
    };

    struct sst_module_declaration : value< std::variant< sst_null_object, sst_class, sst_function > >
    {
    };

    struct sst_member_variable
    {
        std::string name;
        sst_type_reference type;
    };

    struct sst_member_function
    {
    };

    struct sst_class_member : value< std::variant< sst_null_object, sst_member_variable, sst_member_function > >
    {
    };

    struct sst_class
    {
        ast_class m_ast;
        std::vector< std::function< void() > > resolve_size_queue;
        std::optional< std::size_t > size;
        std::size_t unresolved_member_variable_count = 0;
        std::vector< sst_class_member > members;
        std::optional< lir_type_id > m_lir_type_index;
    };

    inline bool sst_resolved(sst_type_reference const& ref)
    {
        return false;
        // todo
        // return std::holds_alternative< sst_class* >(ref.get());
    }

    struct sst_procedure_arg
    {
        std::string name;
        sst_type_reference type;
    };

    struct sst_procedure
    {
        std::size_t priority = 0;
        std::vector< sst_procedure_arg > args;
    };

    struct sst_function
    {
        std::string name;
        std::vector< std::shared_ptr< sst_procedure > > overload_list;
    };

    struct sst_module
    {
        std::map< std::vector< std::string >, sst_module_declaration > stuff;
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_SST_HEADER
