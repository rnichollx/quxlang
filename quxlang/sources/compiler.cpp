// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"



quxlang::compiler::compiler(cow<source_bundle> srcs, std::string target)
    : m_source_code(std::move(srcs)),
      m_configured_target(target)
{

}




