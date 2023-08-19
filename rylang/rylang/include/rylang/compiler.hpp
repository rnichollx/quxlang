//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/module_ast_precursor1.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/lookup_chain.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/filelist.hpp"
#include "rylang/res/class_list_resolver.hpp"
#include "rylang/res/file_ast_resolver.hpp"
#include "rylang/res/file_content_resolver.hpp"
#include "rylang/res/filelist_resolver.hpp"
#include <mutex>
#include <shared_mutex>

namespace rylang
{
    class compiler
    {
        friend class filelist_resolver;
        friend class class_list_resolver;
        friend class file_ast_resolver;
        friend class file_content_resolver;
        friend class classes_per_file_resolver;
        friend class type_id_assignment_resolver;
        friend class class_id_ast_resolver;

        template < typename T >
        using index = rpnx::index< compiler, T >;

        filelist m_file_list;
        filelist_resolver m_filelist_resolver;
        class_list_resolver m_class_list_resolver;
        index< file_content_resolver > m_file_contents_index;
        index< file_ast_resolver > m_file_ast_index;

        rpnx::single_thread_graph_solver< compiler > m_solver;


        std::shared_mutex m_mutex;

        std::size_t m_type_id_next = 1;

        std::size_t assign_type_id()
        {
            std::unique_lock< std::shared_mutex > lock(m_mutex);
            return m_type_id_next++;
        }

      public:


        template < typename T >
        using out = rpnx::output_ptr< compiler, T >;


        compiler(int argc, char** argv);

        filelist get_file_list();

        out< std::string > file_contents(std::string const& filename);
        out< file_ast > lk_file_ast(std::string const& filename);
        out< class_ast > lk_class_ast(symbol_id id);
        out< symbol_id > lk_global_lookup(std::string const& name);
        out< symbol_id > lk_lookup_full_chain( lookup_chain const& chain);
        out< symbol_id > lk_lookup_relative( symbol_id, lookup_singular const& part);
        out< module_ast_precursor1 > lk_module_ast_precursor1(symbol_id id);

        std::string get_file_contents(std::string const& filename)
        {
            auto node = file_contents(filename);
            m_solver.solve(this, node);
            return node->get();
        }
        file_ast get_file_ast(std::string const& filename)
        {
            auto node = lk_file_ast(filename);
            m_solver.solve(this, node);
            return node->get();
        }
        class_list get_class_list()
        {
            auto node = &m_class_list_resolver;
            m_solver.solve(this, node);
            return node->get();
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
