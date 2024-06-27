#include "quxlang/compiler.hpp"



quxlang::compiler::compiler(cow<source_bundle> srcs, std::string target)
    : m_source_code(std::move(srcs)),
      m_configured_target(target)
{

}

rpnx::output_ptr< quxlang::compiler, std::string > quxlang::compiler::file_contents(std::string const& filename)
{
    return m_file_contents_index.lookup(filename);
}


