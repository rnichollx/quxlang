//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/ast/file_ast.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/filelist.hpp"
#include "rylang/res/file_ast_resolver.hpp"
#include "rylang/res/file_content_resolver.hpp"
#include "rylang/res/filelist_resolver.hpp"

namespace rylang
{
    class compiler
    {
        template <typename T>
        using index = rpnx::index< compiler, T >;

        filelist m_file_list;
        filelist_resolver m_filelist_resolver;
        index< file_content_resolver > m_file_contents_index;
        index< file_ast_resolver > m_file_ast_index;

        rpnx::single_thread_graph_solver< compiler > m_solver;

        template < typename T >
        using out = rpnx::output_ptr< compiler, T >;

      public:
        compiler(int argc, char** argv);

        filelist get_file_list();

        out< std::string > file_contents(std::string const& filename);
        out< file_ast > file_ast(std::string const& filename);

        std::string get_file_contents(std::string const& filename)
        {
            auto node = file_contents(filename);
            m_solver.solve(this, node);
            return node->get();
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
