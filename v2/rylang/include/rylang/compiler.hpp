//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/filelist.hpp"
#include "rylang/res/file_content_resolver.hpp"
#include "rylang/res/filelist_resolver.hpp"

namespace rylang
{
    class compiler
    {
        filelist m_file_list;
        filelist_resolver m_filelist_resolver;
        rpnx::index< compiler, file_content_resolver > m_file_contents_index;

        rpnx::single_thread_graph_solver< compiler > m_solver;

      public:
        compiler(int argc, char** argv);

        filelist get_file_list();

        rpnx::output_ptr< compiler, std::string > file_contents(std::string const& filename);

        std::string get_file_contents(std::string const& filename)
        {
            auto node = file_contents(filename);
            m_solver.solve(this, node);
            return node->get();
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
